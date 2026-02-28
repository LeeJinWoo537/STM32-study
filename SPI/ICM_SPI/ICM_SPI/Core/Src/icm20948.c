/**
  ******************************************************************************
  * @file    icm20948.c
  * @brief   ICM20948 9-axis IMU driver (SPI) - Accelerometer, Gyroscope, Magnetometer
  ******************************************************************************
  */

#include "icm20948.h"
#include <string.h>

/* Scale factors: Accel ±4g => 8192 LSB/g, Gyro ±250dps => 131 LSB/(deg/s), Mag 0.15 uT/LSB */
#define ACCEL_SCALE    (1.0f / 8192.0f)   /* 4g full scale */
#define GYRO_SCALE     (1.0f / 131.0f)    /* 250 dps (GYRO_CONFIG_1=0x01) */
#define MAG_SCALE      0.15f              /* uT per LSB */

/* 자이로: 극강 스무딩 (읽기 불안정 시 그래프만 0 근처로 안정화) */
#define GYRO_LPF_ALPHA      0.999f     /* 99.9% 이전 + 0.1% 신규 ≈ 수백 샘플 평균 */
#define GYRO_LPF_ALPHA_SPIKE 0.9995f
#define GYRO_SPIKE_THRESH_DPS  50.0f
#define GYRO_CLAMP_DPS    250.0f

/* 자이로 바이어스 보정: 정지 상태 샘플 평균을 오프셋으로 저장 후 매번 차감 */
#define GYRO_CAL_SKIP      80         /* 전원 직후 N샘플은 제외 (센서 안정화) */
#define GYRO_CAL_SAMPLES   400        /* 보정에 사용할 샘플 수 (스킵 이후) */
#define GYRO_DEADZONE_LSB  150        /* 정지 시 잔여 오차 무시: |raw| < 이 값이면 0 (약 1.1 dps) */

static HAL_StatusTypeDef ICM20948_SelectBank(ICM20948_Handle_t *hicm, uint8_t bank);
static HAL_StatusTypeDef ICM20948_ReadReg(ICM20948_Handle_t *hicm, uint8_t reg, uint8_t *data);
static HAL_StatusTypeDef ICM20948_ReadRegs(ICM20948_Handle_t *hicm, uint8_t reg, uint8_t *data, uint16_t len);
static HAL_StatusTypeDef ICM20948_WriteReg(ICM20948_Handle_t *hicm, uint8_t reg, uint8_t data);

/* 자이로 바이어스 보정용 (LSB 단위, ReadAccelGyro에서 사용) */
static uint16_t gyro_cal_count = 0;
static float gyro_sum_x = 0.0f, gyro_sum_y = 0.0f, gyro_sum_z = 0.0f;
static float gyro_bias_x = 0.0f, gyro_bias_y = 0.0f, gyro_bias_z = 0.0f;
static uint8_t gyro_cal_done = 0;

static HAL_StatusTypeDef ICM20948_SelectBank(ICM20948_Handle_t *hicm, uint8_t bank)
{
  if (hicm->bank == bank)
    return HAL_OK;
  HAL_StatusTypeDef ret = ICM20948_WriteReg(hicm, ICM20948_REG_BANK_SEL, bank << 4);
  if (ret == HAL_OK)
    hicm->bank = bank;
  return ret;
}

static HAL_StatusTypeDef ICM20948_ReadReg(ICM20948_Handle_t *hicm, uint8_t reg, uint8_t *data)
{
  return ICM20948_ReadRegs(hicm, reg, data, 1);
}

static HAL_StatusTypeDef ICM20948_ReadRegs(ICM20948_Handle_t *hicm, uint8_t reg, uint8_t *data, uint16_t len)
{
  uint8_t cmd = reg | 0x80;
  HAL_GPIO_WritePin(hicm->cs_port, hicm->cs_pin, GPIO_PIN_RESET);
  HAL_StatusTypeDef ret = HAL_SPI_Transmit(hicm->hspi, &cmd, 1, 100);
  if (ret != HAL_OK) {
    HAL_GPIO_WritePin(hicm->cs_port, hicm->cs_pin, GPIO_PIN_SET);
    return ret;
  }
  ret = HAL_SPI_Receive(hicm->hspi, data, len, 100);
  HAL_GPIO_WritePin(hicm->cs_port, hicm->cs_pin, GPIO_PIN_SET);
  return ret;
}

static HAL_StatusTypeDef ICM20948_WriteReg(ICM20948_Handle_t *hicm, uint8_t reg, uint8_t data)
{
  uint8_t buf[2] = { reg, data };
  HAL_GPIO_WritePin(hicm->cs_port, hicm->cs_pin, GPIO_PIN_RESET);
  HAL_StatusTypeDef ret = HAL_SPI_Transmit(hicm->hspi, buf, 2, 100);
  HAL_GPIO_WritePin(hicm->cs_port, hicm->cs_pin, GPIO_PIN_SET);
  return ret;
}

HAL_StatusTypeDef ICM20948_Init(ICM20948_Handle_t *hicm, SPI_HandleTypeDef *hspi,
                                GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
  uint8_t whoami;
  if (hicm == NULL || hspi == NULL || cs_port == NULL)
    return HAL_ERROR;

  hicm->hspi = hspi;
  hicm->cs_port = cs_port;
  hicm->cs_pin = cs_pin;
  hicm->bank = 0xFF;

  /* Configure CS pin as GPIO output (in case Cube set it as NSS AF) */
  GPIO_InitTypeDef g = {0};
  g.Pin = cs_pin;
  g.Mode = GPIO_MODE_OUTPUT_PP;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init((GPIO_TypeDef *)cs_port, &g);
  HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);

  HAL_Delay(10);

  /* Select Bank 0 and check WHO_AM_I */
  if (ICM20948_SelectBank(hicm, ICM20948_BANK_0) != HAL_OK)
    return HAL_ERROR;
  if (ICM20948_ReadReg(hicm, ICM20948_WHO_AM_I, &whoami) != HAL_OK)
    return HAL_ERROR;
  if (whoami != ICM20948_WHO_AM_I_VALUE)
    return HAL_ERROR;

  /* Reset and wake */
  ICM20948_WriteReg(hicm, ICM20948_PWR_MGMT_1, 0x80);
  HAL_Delay(50);
  ICM20948_WriteReg(hicm, ICM20948_PWR_MGMT_1, 0x01);
  HAL_Delay(10);
  ICM20948_WriteReg(hicm, ICM20948_PWR_MGMT_2, 0x00);

  /* Bank 2: Gyro 250dps + DLPF 5.7Hz (정지 시 노이즈 감소), Accel 4g */
  if (ICM20948_SelectBank(hicm, ICM20948_BANK_2) != HAL_OK)
    return HAL_ERROR;
  ICM20948_WriteReg(hicm, ICM20948_GYRO_CONFIG_1, 0x31);   /* 250dps, DLPF enable, DLPFCFG=6 (5.7Hz BW) */
  ICM20948_WriteReg(hicm, ICM20948_ACCEL_CONFIG, 0x00);    /* ±4g */
  ICM20948_SelectBank(hicm, ICM20948_BANK_0);

  /* Enable I2C master for magnetometer */
  ICM20948_WriteReg(hicm, ICM20948_USER_CTRL, 0x20);       /* I2C_MST_EN */
  HAL_Delay(10);
  ICM20948_WriteReg(hicm, ICM20948_LP_CONFIG, 0x20);       /* I2C_MST_CYCLE: 매 샘플마다 I2C 마스터 실행 */

  /* Bank 3: I2C master 400kHz, configure SLV0 to read AK09916 */
  if (ICM20948_SelectBank(hicm, ICM20948_BANK_3) != HAL_OK)
    return HAL_ERROR;
  ICM20948_WriteReg(hicm, ICM20948_I2C_MST_CTRL, 0x0F);    /* I2C 400kHz + MULT_MST_EN + 등 (데이터시트 권장) */
  /* Optional: verify AK09916 present (read WIA2) — 트리거 후에만 I2C 실행됨 */
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_ADDR, AK09916_I2C_ADDR | 0x80);
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_REG, AK09916_WIA2);
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_CTRL, 0x81);
  ICM20948_SelectBank(hicm, ICM20948_BANK_0);
  { uint8_t trig[13]; (void)ICM20948_ReadRegs(hicm, ICM20948_ACCEL_XOUT_H - 1, trig, 13); }
  HAL_Delay(5);
  {
    uint8_t wia2;
    if (ICM20948_ReadReg(hicm, ICM20948_EXT_SLV_SENS_DATA_00, &wia2) == HAL_OK &&
        wia2 == AK09916_WIA2_VALUE) {
      /* AK09916 OK */
    }
    /* If wia2 != 0x09, mag may still work on some boards; continue init */
  }
  if (ICM20948_SelectBank(hicm, ICM20948_BANK_3) != HAL_OK)
    return HAL_OK;
  ICM20948_WriteReg(hicm, ICM20948_I2C_MST_DELAY_CTRL, 0x01);   /* DELAY_ES_SHADOW */
  /* AK09916 소프트 리셋 — 트리거해야 I2C 쓰기 실행됨 */
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_ADDR, AK09916_I2C_ADDR);
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_REG, AK09916_CNTL3);
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_DO, 0x01);         /* CNTL3 = 리셋 */
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_CTRL, 0x81);
  ICM20948_SelectBank(hicm, ICM20948_BANK_0);
  { uint8_t trig[13]; (void)ICM20948_ReadRegs(hicm, ICM20948_ACCEL_XOUT_H - 1, trig, 13); }
  HAL_Delay(20);
  /* AK09916 연속 10Hz 설정 — 트리거해야 I2C 쓰기 실행됨 (ReadMagnetometer에서 단일측정으로 덮어씀) */
  if (ICM20948_SelectBank(hicm, ICM20948_BANK_3) != HAL_OK)
    return HAL_OK;
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_ADDR, AK09916_I2C_ADDR);
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_REG, AK09916_CNTL2);
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_DO, AK09916_CNTL2_CONT_10HZ);
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_CTRL, 0x81);
  ICM20948_SelectBank(hicm, ICM20948_BANK_0);
  { uint8_t trig[13]; (void)ICM20948_ReadRegs(hicm, ICM20948_ACCEL_XOUT_H - 1, trig, 13); }
  HAL_Delay(150);   /* 연속 모드 + 첫 샘플 대기 */
  if (ICM20948_SelectBank(hicm, ICM20948_BANK_3) != HAL_OK)
    return HAL_OK;
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_ADDR, AK09916_I2C_ADDR | 0x80);
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_REG, AK09916_HXL);
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_CTRL, 0x88);
  ICM20948_SelectBank(hicm, ICM20948_BANK_0);

  /* 자이로 바이어스 보정: Init 시점에 보드를 가만히 둔 상태로 가정, 샘플 수집 후 bias 저장 (데이터 전송 전에 완료) */
  if (ICM20948_SelectBank(hicm, ICM20948_BANK_0) != HAL_OK)
    return HAL_OK;
  HAL_Delay(50);
  {
    uint8_t buf[3];
    float sx = 0.0f, sy = 0.0f, sz = 0.0f;
    uint16_t i;
    for (i = 0; i < GYRO_CAL_SKIP; i++) {
      ICM20948_ReadRegs(hicm, ICM20948_GYRO_XOUT_H - 1, buf, 3);
      ICM20948_ReadRegs(hicm, ICM20948_GYRO_YOUT_H - 1, buf, 3);
      ICM20948_ReadRegs(hicm, ICM20948_GYRO_ZOUT_H - 1, buf, 3);
      HAL_Delay(2);
    }
    for (i = 0; i < GYRO_CAL_SAMPLES; i++) {
      ICM20948_ReadRegs(hicm, ICM20948_GYRO_XOUT_H - 1, buf, 3);
      sx += (float)(int16_t)((buf[1] << 8) | buf[2]);
      ICM20948_ReadRegs(hicm, ICM20948_GYRO_YOUT_H - 1, buf, 3);
      sy += (float)(int16_t)((buf[1] << 8) | buf[2]);
      ICM20948_ReadRegs(hicm, ICM20948_GYRO_ZOUT_H - 1, buf, 3);
      sz += (float)(int16_t)((buf[1] << 8) | buf[2]);
      HAL_Delay(2);
    }
    gyro_bias_x = sx / (float)GYRO_CAL_SAMPLES;
    gyro_bias_y = sy / (float)GYRO_CAL_SAMPLES;
    gyro_bias_z = sz / (float)GYRO_CAL_SAMPLES;
    gyro_cal_done = 1;
  }

  return HAL_OK;
}

HAL_StatusTypeDef ICM20948_ReadAccelGyro(ICM20948_Handle_t *hicm, ICM20948_Data_t *data)
{
  uint8_t buf[7];
  if (ICM20948_SelectBank(hicm, ICM20948_BANK_0) != HAL_OK)
    return HAL_ERROR;

  /* 가속도: 0x2C부터 7바이트 읽기 → 수신 7바이트 = 0x2D..0x33, buf[0..5] = ACCEL_XOUT_H..ACCEL_ZOUT_L */
  if (ICM20948_ReadRegs(hicm, ICM20948_ACCEL_XOUT_H - 1, buf, 7) != HAL_OK)
    return HAL_ERROR;
  data->accel_x = (float)(int16_t)((buf[0] << 8) | buf[1]);
  data->accel_y = (float)(int16_t)((buf[2] << 8) | buf[3]);
  data->accel_z = (float)(int16_t)((buf[4] << 8) | buf[5]);

  /* 자이로: 축마다 3바이트씩 따로 읽기. 주소 (GYRO_*OUT_H - 1)로 읽으면 첫 바이트는 이전 레지스터(0x32=ACCEL_ZOUT_L 등)이므로 buf[1]=H, buf[2]=L 사용 */
  if (ICM20948_ReadRegs(hicm, ICM20948_GYRO_XOUT_H - 1, buf, 3) != HAL_OK)
    return HAL_ERROR;
  data->gyro_x = (float)(int16_t)((buf[1] << 8) | buf[2]);
  if (ICM20948_ReadRegs(hicm, ICM20948_GYRO_YOUT_H - 1, buf, 3) != HAL_OK)
    return HAL_ERROR;
  data->gyro_y = (float)(int16_t)((buf[1] << 8) | buf[2]);
  if (ICM20948_ReadRegs(hicm, ICM20948_GYRO_ZOUT_H - 1, buf, 3) != HAL_OK)
    return HAL_ERROR;
  data->gyro_z = (float)(int16_t)((buf[1] << 8) | buf[2]);

  /* 자이로 바이어스 보정: GYRO_CAL_SKIP 이후 샘플만 모아 평균을 bias로 저장, 이후 매번 차감 */
  if (!gyro_cal_done) {
    gyro_cal_count++;
    if (gyro_cal_count > GYRO_CAL_SKIP) {
      gyro_sum_x += data->gyro_x;
      gyro_sum_y += data->gyro_y;
      gyro_sum_z += data->gyro_z;
      if ((gyro_cal_count - GYRO_CAL_SKIP) >= GYRO_CAL_SAMPLES) {
        float n = (float)GYRO_CAL_SAMPLES;
        gyro_bias_x = gyro_sum_x / n;
        gyro_bias_y = gyro_sum_y / n;
        gyro_bias_z = gyro_sum_z / n;
        gyro_cal_done = 1;
      }
    }
  }
  if (gyro_cal_done) {
    data->gyro_x -= gyro_bias_x;
    data->gyro_y -= gyro_bias_y;
    data->gyro_z -= gyro_bias_z;
    /* 정지 시 잔여 오차 제거: 아주 작은 값은 0으로 (가만히 있을 때 0에 수렴) */
    if (data->gyro_x > -GYRO_DEADZONE_LSB && data->gyro_x < GYRO_DEADZONE_LSB) data->gyro_x = 0.0f;
    if (data->gyro_y > -GYRO_DEADZONE_LSB && data->gyro_y < GYRO_DEADZONE_LSB) data->gyro_y = 0.0f;
    if (data->gyro_z > -GYRO_DEADZONE_LSB && data->gyro_z < GYRO_DEADZONE_LSB) data->gyro_z = 0.0f;
  }

  return HAL_OK;
}

HAL_StatusTypeDef ICM20948_ReadMagnetometer(ICM20948_Handle_t *hicm, ICM20948_Data_t *data)
{
  uint8_t buf[8];
  uint8_t trig[13];
  /* 매번 단일 측정(single-shot)으로 새로 측정 트리거 후 읽기. 연속 모드에 의존하지 않음 */
  if (ICM20948_SelectBank(hicm, ICM20948_BANK_3) != HAL_OK)
    return HAL_ERROR;
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_ADDR, AK09916_I2C_ADDR);
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_REG, AK09916_CNTL2);
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_DO, AK09916_CNTL2_SINGLE);
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_CTRL, 0x81);
  if (ICM20948_SelectBank(hicm, ICM20948_BANK_0) != HAL_OK)
    return HAL_ERROR;
  (void)ICM20948_ReadRegs(hicm, ICM20948_ACCEL_XOUT_H - 1, trig, 13);  /* 트리거 → CNTL2=0x01 쓰기 실행 */
  HAL_Delay(20);   /* AK09916 단일 측정 완료 대기 (데이터시트 ~10ms) */

  if (ICM20948_SelectBank(hicm, ICM20948_BANK_3) != HAL_OK)
    return HAL_ERROR;
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_ADDR, AK09916_I2C_ADDR | 0x80);
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_REG, AK09916_HXL);
  ICM20948_WriteReg(hicm, ICM20948_I2C_SLV0_CTRL, 0x88);
  if (ICM20948_SelectBank(hicm, ICM20948_BANK_0) != HAL_OK)
    return HAL_ERROR;
  (void)ICM20948_ReadRegs(hicm, ICM20948_ACCEL_XOUT_H - 1, trig, 13);  /* 트리거 → HXL 8바이트 읽기 실행 */
  HAL_Delay(50);   /* I2C 8바이트 읽기 완료·EXT_SLV 갱신 대기 */
  if (ICM20948_ReadRegs(hicm, ICM20948_EXT_SLV_SENS_DATA_00, buf, 8) != HAL_OK)
    return HAL_ERROR;

  /* HXL,HXH, HYL,HYH, HZL,HZH, (0x17), ST2 — 8바이트 읽기로 ST2까지 읽어야 다음 측정이 갱신됨. buf[7]=ST2 */
  (void)buf[7];   /* ST2 읽기 완료로 읽기 사이클 종료 → 다음 데이터 준비 */
  int16_t mx = (int16_t)((buf[1] << 8) | buf[0]);
  int16_t my = (int16_t)((buf[3] << 8) | buf[2]);
  int16_t mz = (int16_t)((buf[5] << 8) | buf[4]);

  data->mag_x = (float)mx * MAG_SCALE;
  data->mag_y = (float)my * MAG_SCALE;
  data->mag_z = (float)mz * MAG_SCALE;

  return HAL_OK;
}

void ICM20948_ScaleData(ICM20948_Data_t *data)
{
  data->accel_x *= ACCEL_SCALE;
  data->accel_y *= ACCEL_SCALE;
  data->accel_z *= ACCEL_SCALE;
  data->gyro_x  *= GYRO_SCALE;
  data->gyro_y  *= GYRO_SCALE;
  data->gyro_z  *= GYRO_SCALE;

  /* 자이로만 극강 LPF (노이즈가 커도 출력은 0 근처로 수렴) */
  static float gyro_prev_x = 0.0f, gyro_prev_y = 0.0f, gyro_prev_z = 0.0f;
  static uint8_t gyro_lpf_init = 0;
  float ax, ay, az;
  if (!gyro_lpf_init) {
    gyro_prev_x = 0.0f;
    gyro_prev_y = 0.0f;
    gyro_prev_z = 0.0f;
    gyro_lpf_init = 1;
  }
  {
    ax = GYRO_LPF_ALPHA;
    ay = GYRO_LPF_ALPHA;
    az = GYRO_LPF_ALPHA;
    if (data->gyro_x - gyro_prev_x > GYRO_SPIKE_THRESH_DPS || gyro_prev_x - data->gyro_x > GYRO_SPIKE_THRESH_DPS)
      ax = GYRO_LPF_ALPHA_SPIKE;
    if (data->gyro_y - gyro_prev_y > GYRO_SPIKE_THRESH_DPS || gyro_prev_y - data->gyro_y > GYRO_SPIKE_THRESH_DPS)
      ay = GYRO_LPF_ALPHA_SPIKE;
    if (data->gyro_z - gyro_prev_z > GYRO_SPIKE_THRESH_DPS || gyro_prev_z - data->gyro_z > GYRO_SPIKE_THRESH_DPS)
      az = GYRO_LPF_ALPHA_SPIKE;
    data->gyro_x = ax * gyro_prev_x + (1.0f - ax) * data->gyro_x;
    data->gyro_y = ay * gyro_prev_y + (1.0f - ay) * data->gyro_y;
    data->gyro_z = az * gyro_prev_z + (1.0f - az) * data->gyro_z;
  }
  /* 비정상 과대값 방지 (풀스케일 250 dps) */
  if (data->gyro_x > GYRO_CLAMP_DPS)  data->gyro_x = GYRO_CLAMP_DPS;
  if (data->gyro_x < -GYRO_CLAMP_DPS) data->gyro_x = -GYRO_CLAMP_DPS;
  if (data->gyro_y > GYRO_CLAMP_DPS)  data->gyro_y = GYRO_CLAMP_DPS;
  if (data->gyro_y < -GYRO_CLAMP_DPS) data->gyro_y = -GYRO_CLAMP_DPS;
  if (data->gyro_z > GYRO_CLAMP_DPS)  data->gyro_z = GYRO_CLAMP_DPS;
  if (data->gyro_z < -GYRO_CLAMP_DPS) data->gyro_z = -GYRO_CLAMP_DPS;
  gyro_prev_x = data->gyro_x;
  gyro_prev_y = data->gyro_y;
  gyro_prev_z = data->gyro_z;

  /* mag already scaled in ReadMagnetometer */
}
