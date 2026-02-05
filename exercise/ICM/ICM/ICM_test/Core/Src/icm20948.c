/**
  ******************************************************************************
  * @file    icm20948.c
  * @brief   ICM20948 9-axis IMU driver implementation
  *
  * 구성:
  * - 뱅크 선택/레지스터 읽쓰기 헬퍼
  * - Init: 0x68/0x69 시도, WHO_AM_I, 리셋, Bank2 설정, I2C 마스터·AK09916 초기화
  * - ReadAccel/ReadGyro/ReadMag: raw + 물리값, 필터(EMA 또는 MA) 적용
  ******************************************************************************
  */

#include "icm20948.h"

#define I2C_TIMEOUT_MS  100

/* ==================== I2C 헬퍼 (뱅크 선택, 레지스터 읽/쓰) ==================== */
static HAL_StatusTypeDef ICM20948_SelectBank(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t bank)
{
  uint8_t reg = ICM20948_REG_BANK_SEL;
  return HAL_I2C_Mem_Write(hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, &bank, 1, I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef ICM20948_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t reg, uint8_t value)
{
  return HAL_I2C_Mem_Write(hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef ICM20948_ReadRegs(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
  return HAL_I2C_Mem_Read(hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, I2C_TIMEOUT_MS);
}

/* ==================== 필터: 1단계 EMA (지수이동평균) ====================
 * out = alpha*prev + (1-alpha)*in. prev는 호출 후 갱신됨.
 */
static void FilterAxis(float *out, float *prev, float in, float alpha)
{
  float y = alpha * (*prev) + (1.0f - alpha) * in;
  *prev = y;
  *out = y;
}

/* ==================== 필터: 이동평균(MA) 1축 ====================
 * axis: 0~2 가속도 x,y,z / 3~5 자이로 x,y,z / 6~8 마그 x,y,z
 * 버퍼가 가득 차기 전까지는 누적 평균, 이후에는 원형 버퍼로 최근 N개 평균.
 */
static void ApplyMA(ICM20948_HandleTypeDef *hicm, uint8_t axis, float in, float *out)
{
  uint8_t n = hicm->ma_window;
  uint8_t *len = &hicm->ma_len[axis];
  uint8_t *idx = &hicm->ma_idx[axis];
  float *sum = &hicm->ma_sum[axis];
  float (*buf)[ICM20948_MA_MAX_LEN] = hicm->ma_buf;

  if (*len < n) {
    buf[axis][*len] = in;
    *sum += in;
    *len += 1;
    *out = *sum / (float)(*len);
  } else {
    float old = buf[axis][*idx];
    *sum -= old;
    buf[axis][*idx] = in;
    *sum += in;
    *out = *sum / (float)n;
    *idx = (*idx + 1) % n;
  }
}

void ICM20948_SetFilter(ICM20948_HandleTypeDef *hicm, uint8_t enable, float alpha)
{
  if (hicm == NULL) return;
  hicm->filter_enabled = enable ? 1 : 0;
  hicm->filter_mode = ICM20948_FILTER_EMA;
  if (alpha < 0.01f) alpha = 0.01f;
  if (alpha > 0.99f) alpha = 0.99f;
  hicm->filter_alpha = alpha;
  hicm->filter_initialized = 0;
}

void ICM20948_SetFilterMovingAvg(ICM20948_HandleTypeDef *hicm, uint8_t enable, uint8_t window)
{
  if (hicm == NULL) return;
  hicm->filter_enabled = enable ? 1 : 0;
  hicm->filter_mode = ICM20948_FILTER_MA;
  if (window < 4U) window = 4U;
  if (window > ICM20948_MA_MAX_LEN) window = ICM20948_MA_MAX_LEN;
  hicm->ma_window = window;
  for (int i = 0; i < 9; i++) {
    hicm->ma_len[i] = 0;
    hicm->ma_idx[i] = 0;
    hicm->ma_sum[i] = 0.0f;
  }
}

void ICM20948_ResetFilter(ICM20948_HandleTypeDef *hicm)
{
  if (hicm == NULL) return;
  hicm->filter_initialized = 0;
  for (int i = 0; i < 9; i++) {
    hicm->ma_len[i] = 0;
    hicm->ma_idx[i] = 0;
    hicm->ma_sum[i] = 0.0f;
  }
}

/* ==================== 초기화 ====================
 * 1) 0x68로 뱅크0 선택 -> 실패 시 0x69 시도
 * 2) WHO_AM_I 확인 (0xEA)
 * 3) 리셋 -> PLL 클럭, 축 활성화
 * 4) Bank2: 자이로 500dps, 가속도 4g
 * 5) I2C 마스터 활성화, Bank3에서 AK09916 리셋·연속모드(100Hz) 설정
 */
HAL_StatusTypeDef ICM20948_Init(ICM20948_HandleTypeDef *hicm, I2C_HandleTypeDef *hi2c)
{
  HAL_StatusTypeDef status;
  uint8_t whoami;
  uint8_t val;

  if (hicm == NULL || hi2c == NULL)
    return HAL_ERROR;

  HAL_Delay(5);  /* power-on / line stabilization */
  hicm->hi2c = hi2c;
  hicm->accel_scale = ICM20948_ACCEL_SCALE_4G;
  hicm->gyro_scale = ICM20948_GYRO_SCALE_500;
  hicm->filter_enabled = 0;
  hicm->filter_mode = ICM20948_FILTER_EMA;
  hicm->filter_initialized = 0;
  for (int i = 0; i < 9; i++) {
    hicm->ma_len[i] = 0;
    hicm->ma_idx[i] = 0;
    hicm->ma_sum[i] = 0.0f;
  }

  /* Try 0x68 first, then 0x69 (AD0 high on many breakouts) */
  hicm->addr = ICM20948_I2C_ADDR_0;
  status = ICM20948_SelectBank(hi2c, hicm->addr, ICM20948_BANK_0);
  if (status != HAL_OK) {
    hicm->addr = ICM20948_I2C_ADDR_1;
    status = ICM20948_SelectBank(hi2c, hicm->addr, ICM20948_BANK_0);
  }
  if (status != HAL_OK) return status;

  status = ICM20948_ReadRegs(hi2c, hicm->addr, ICM20948_WHO_AM_I_REG, &whoami, 1);
  if (status != HAL_OK) return status;
  if (whoami != ICM20948_WHO_AM_I) {
    /* Retry with other address */
    if (hicm->addr == ICM20948_I2C_ADDR_0)
      hicm->addr = ICM20948_I2C_ADDR_1;
    else
      return HAL_ERROR;
    status = ICM20948_SelectBank(hi2c, hicm->addr, ICM20948_BANK_0);
    if (status != HAL_OK) return status;
    status = ICM20948_ReadRegs(hi2c, hicm->addr, ICM20948_WHO_AM_I_REG, &whoami, 1);
    if (status != HAL_OK || whoami != ICM20948_WHO_AM_I)
      return HAL_ERROR;
  }

  /* Reset device */
  val = ICM20948_DEVICE_RESET;
  status = ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_PWR_MGMT_1, val);
  if (status != HAL_OK) return status;
  HAL_Delay(10);

  /* Wake up, use PLL clock */
  val = ICM20948_CLKSEL_PLL;
  status = ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_PWR_MGMT_1, val);
  if (status != HAL_OK) return status;
  HAL_Delay(1);

  /* Enable all accel & gyro axes (PWR_MGMT_2: 0x00 = enable all) */
  val = 0x00;
  status = ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_PWR_MGMT_2, val);
  if (status != HAL_OK) return status;

  /* Bank 2: Gyro config 500dps, Accel config 4g */
  status = ICM20948_SelectBank(hi2c, hicm->addr, ICM20948_BANK_2);
  if (status != HAL_OK) return status;
  val = 0x01U; /* GYRO_FS_500_DPS, GYRO_DLPF bypass */
  ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_GYRO_CONFIG_1, val);
  val = 0x01U; /* ACCEL_FS_4G, ACCEL_DLPF bypass */
  ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_ACCEL_CONFIG, val);
  status = ICM20948_SelectBank(hi2c, hicm->addr, ICM20948_BANK_0);
  if (status != HAL_OK) return status;

  /* Enable I2C master (for internal magnetometer) */
  val = ICM20948_I2C_MST_EN;
  status = ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_USER_CTRL, val);
  if (status != HAL_OK) return status;

  /* Bank 3: I2C master clock ~400kHz */
  status = ICM20948_SelectBank(hi2c, hicm->addr, ICM20948_BANK_3);
  if (status != HAL_OK) return status;
  val = 0x07U; /* I2C_MST_CLK 345.6kHz */
  ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_I2C_MST_CTRL, val);

  /* Reset AK09916 via I2C master: write 0x01 to AK09916 CNTL3 (0x32) */
  val = AK09916_I2C_ADDR & 0x7FU; /* write */
  ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_ADDR, val);
  ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_REG, AK09916_CNTL3);
  ICM20948_WriteReg(hi2c, hicm->addr, 0x06, AK09916_CNTL3_SRST); /* SLV0_DO = 0x01 */
  ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, 0x81); /* 1 byte write, enable */
  HAL_Delay(10);
  ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, 0);   /* disable SLV0 */

  /* Set AK09916 continuous mode 100Hz: write 0x08 to CNTL2 (0x31) */
  ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_ADDR, AK09916_I2C_ADDR & 0x7FU);
  ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_REG, AK09916_CNTL2);
  ICM20948_WriteReg(hi2c, hicm->addr, 0x06, AK09916_CNTL2_CONTINUOUS);
  ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, 0x81);
  HAL_Delay(10);
  ICM20948_WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, 0);

  /* Back to Bank 0 for data reads */
  status = ICM20948_SelectBank(hi2c, hicm->addr, ICM20948_BANK_0);
  return status;
}

/* ==================== 가속도 읽기 ====================
 * Bank0 0x2D~0x32 6바이트 읽어 raw(g 포인터 NULL이면 생략)와 g 단위로 변환.
 * 필터 켜져 있으면 EMA 2단 또는 MA 적용 후 g에 반영.
 */
HAL_StatusTypeDef ICM20948_ReadAccel(ICM20948_HandleTypeDef *hicm, ICM20948_AxisRaw_t *raw, ICM20948_Axis_t *g)
{
  uint8_t buf[6];
  HAL_StatusTypeDef status;

  if (hicm == NULL) return HAL_ERROR;
  status = ICM20948_SelectBank(hicm->hi2c, hicm->addr, ICM20948_BANK_0);
  if (status != HAL_OK) return status;
  status = ICM20948_ReadRegs(hicm->hi2c, hicm->addr, ICM20948_ACCEL_XOUT_H, buf, 6);
  if (status != HAL_OK) return status;

  if (raw) {
    raw->x = (int16_t)((uint16_t)buf[0] << 8 | buf[1]);
    raw->y = (int16_t)((uint16_t)buf[2] << 8 | buf[3]);
    raw->z = (int16_t)((uint16_t)buf[4] << 8 | buf[5]);
  }
  if (g) {
    g->x = (float)((int16_t)((uint16_t)buf[0] << 8 | buf[1])) / hicm->accel_scale;
    g->y = (float)((int16_t)((uint16_t)buf[2] << 8 | buf[3])) / hicm->accel_scale;
    g->z = (float)((int16_t)((uint16_t)buf[4] << 8 | buf[5])) / hicm->accel_scale;
    if (hicm->filter_enabled) {
      if (hicm->filter_mode == ICM20948_FILTER_MA) {
        ApplyMA(hicm, 0, g->x, &g->x);
        ApplyMA(hicm, 1, g->y, &g->y);
        ApplyMA(hicm, 2, g->z, &g->z);
      } else {
        float a = hicm->filter_alpha;
        if (!(hicm->filter_initialized & 0x01U)) {
          hicm->filter_accel.x = g->x; hicm->filter_accel.y = g->y; hicm->filter_accel.z = g->z;
          hicm->filter_initialized |= 0x01U;
        } else {
          FilterAxis(&g->x, &hicm->filter_accel.x, g->x, a);
          FilterAxis(&g->y, &hicm->filter_accel.y, g->y, a);
          FilterAxis(&g->z, &hicm->filter_accel.z, g->z, a);
        }
        if (!(hicm->filter_initialized & 0x10U)) {
          hicm->filter_accel2.x = g->x; hicm->filter_accel2.y = g->y; hicm->filter_accel2.z = g->z;
          hicm->filter_initialized |= 0x10U;
        } else {
          FilterAxis(&g->x, &hicm->filter_accel2.x, g->x, a);
          FilterAxis(&g->y, &hicm->filter_accel2.y, g->y, a);
          FilterAxis(&g->z, &hicm->filter_accel2.z, g->z, a);
        }
      }
    }
  }
  return HAL_OK;
}

/* ==================== 자이로 읽기 ====================
 * Bank0 0x33~0x38 6바이트. raw, dps(dps 단위). 필터 동일.
 */
HAL_StatusTypeDef ICM20948_ReadGyro(ICM20948_HandleTypeDef *hicm, ICM20948_AxisRaw_t *raw, ICM20948_Axis_t *dps)
{
  uint8_t buf[6];
  HAL_StatusTypeDef status;

  if (hicm == NULL) return HAL_ERROR;
  status = ICM20948_SelectBank(hicm->hi2c, hicm->addr, ICM20948_BANK_0);
  if (status != HAL_OK) return status;
  status = ICM20948_ReadRegs(hicm->hi2c, hicm->addr, ICM20948_GYRO_XOUT_H, buf, 6);
  if (status != HAL_OK) return status;

  if (raw) {
    raw->x = (int16_t)((uint16_t)buf[0] << 8 | buf[1]);
    raw->y = (int16_t)((uint16_t)buf[2] << 8 | buf[3]);
    raw->z = (int16_t)((uint16_t)buf[4] << 8 | buf[5]);
  }
  if (dps) {
    dps->x = (float)((int16_t)((uint16_t)buf[0] << 8 | buf[1])) / hicm->gyro_scale;
    dps->y = (float)((int16_t)((uint16_t)buf[2] << 8 | buf[3])) / hicm->gyro_scale;
    dps->z = (float)((int16_t)((uint16_t)buf[4] << 8 | buf[5])) / hicm->gyro_scale;
    if (hicm->filter_enabled) {
      if (hicm->filter_mode == ICM20948_FILTER_MA) {
        ApplyMA(hicm, 3, dps->x, &dps->x);
        ApplyMA(hicm, 4, dps->y, &dps->y);
        ApplyMA(hicm, 5, dps->z, &dps->z);
      } else {
        float a = hicm->filter_alpha;
        if (!(hicm->filter_initialized & 0x02U)) {
          hicm->filter_gyro.x = dps->x; hicm->filter_gyro.y = dps->y; hicm->filter_gyro.z = dps->z;
          hicm->filter_initialized |= 0x02U;
        } else {
          FilterAxis(&dps->x, &hicm->filter_gyro.x, dps->x, a);
          FilterAxis(&dps->y, &hicm->filter_gyro.y, dps->y, a);
          FilterAxis(&dps->z, &hicm->filter_gyro.z, dps->z, a);
        }
        if (!(hicm->filter_initialized & 0x20U)) {
          hicm->filter_gyro2.x = dps->x; hicm->filter_gyro2.y = dps->y; hicm->filter_gyro2.z = dps->z;
          hicm->filter_initialized |= 0x20U;
        } else {
          FilterAxis(&dps->x, &hicm->filter_gyro2.x, dps->x, a);
          FilterAxis(&dps->y, &hicm->filter_gyro2.y, dps->y, a);
          FilterAxis(&dps->z, &hicm->filter_gyro2.z, dps->z, a);
        }
      }
    }
  }
  return HAL_OK;
}

/* ==================== 마그 읽기 (AK09916) ====================
 * ICM20948 내부 I2C 마스터로 AK09916 레지스터 0x10(ST1)부터 8바이트 읽기 설정
 * -> Bank0 복귀 후 EXT_SLV_SENS_DATA_00에서 8바이트 읽어 HX,HY,HZ 추출, uT로 변환.
 * 필터 켜져 있으면 EMA 2단 또는 MA 적용.
 */
HAL_StatusTypeDef ICM20948_ReadMag(ICM20948_HandleTypeDef *hicm, ICM20948_AxisRaw_t *raw, ICM20948_Axis_t *ut)
{
  HAL_StatusTypeDef status;
  uint8_t slv0_addr = (AK09916_I2C_ADDR | 0x80U);  /* read */
  uint8_t slv0_reg  = AK09916_ST1;
  uint8_t slv0_ctrl = 0x87U;  /* 8 bytes read, enable */

  if (hicm == NULL) return HAL_ERROR;

  /* Select Bank 3, configure SLV0 to read 8 bytes from AK09916 ST1 (0x10) */
  status = ICM20948_SelectBank(hicm->hi2c, hicm->addr, ICM20948_BANK_3);
  if (status != HAL_OK) return status;
  ICM20948_WriteReg(hicm->hi2c, hicm->addr, ICM20948_I2C_SLV0_ADDR, slv0_addr);
  ICM20948_WriteReg(hicm->hi2c, hicm->addr, ICM20948_I2C_SLV0_REG, slv0_reg);
  ICM20948_WriteReg(hicm->hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, slv0_ctrl);

  /* Back to Bank 0, wait for I2C master to complete */
  status = ICM20948_SelectBank(hicm->hi2c, hicm->addr, ICM20948_BANK_0);
  if (status != HAL_OK) return status;
  HAL_Delay(2);

  uint8_t buf[8];
  status = ICM20948_ReadRegs(hicm->hi2c, hicm->addr, ICM20948_EXT_SLV_SENS_DATA_00, buf, 8);
  if (status != HAL_OK) return status;

  /* buf[0]=ST1, buf[1-2]=HX, buf[3-4]=HY, buf[5-6]=HZ, buf[7]=ST2. AK09916 is 16-bit, unit 0.15uT */
  if (raw) {
    raw->x = (int16_t)((uint16_t)buf[2] << 8 | buf[1]);
    raw->y = (int16_t)((uint16_t)buf[4] << 8 | buf[3]);
    raw->z = (int16_t)((uint16_t)buf[6] << 8 | buf[5]);
  }
  if (ut) {
    ut->x = (float)((int16_t)((uint16_t)buf[2] << 8 | buf[1])) * AK09916_MAG_SCALE;
    ut->y = (float)((int16_t)((uint16_t)buf[4] << 8 | buf[3])) * AK09916_MAG_SCALE;
    ut->z = (float)((int16_t)((uint16_t)buf[6] << 8 | buf[5])) * AK09916_MAG_SCALE;
    if (hicm->filter_enabled) {
      if (hicm->filter_mode == ICM20948_FILTER_MA) {
        ApplyMA(hicm, 6, ut->x, &ut->x);
        ApplyMA(hicm, 7, ut->y, &ut->y);
        ApplyMA(hicm, 8, ut->z, &ut->z);
      } else {
        float a = hicm->filter_alpha;
        if (!(hicm->filter_initialized & 0x04U)) {
          hicm->filter_mag.x = ut->x; hicm->filter_mag.y = ut->y; hicm->filter_mag.z = ut->z;
          hicm->filter_initialized |= 0x04U;
        } else {
          FilterAxis(&ut->x, &hicm->filter_mag.x, ut->x, a);
          FilterAxis(&ut->y, &hicm->filter_mag.y, ut->y, a);
          FilterAxis(&ut->z, &hicm->filter_mag.z, ut->z, a);
        }
        if (!(hicm->filter_initialized & 0x40U)) {
          hicm->filter_mag2.x = ut->x; hicm->filter_mag2.y = ut->y; hicm->filter_mag2.z = ut->z;
          hicm->filter_initialized |= 0x40U;
        } else {
          FilterAxis(&ut->x, &hicm->filter_mag2.x, ut->x, a);
          FilterAxis(&ut->y, &hicm->filter_mag2.y, ut->y, a);
          FilterAxis(&ut->z, &hicm->filter_mag2.z, ut->z, a);
        }
      }
    }
  }
  return HAL_OK;
}

/* ==================== 3축 한 번에 읽기 ==================== */
HAL_StatusTypeDef ICM20948_ReadAll(ICM20948_HandleTypeDef *hicm,
  ICM20948_AxisRaw_t *accel_raw, ICM20948_Axis_t *accel_g,
  ICM20948_AxisRaw_t *gyro_raw, ICM20948_Axis_t *gyro_dps,
  ICM20948_AxisRaw_t *mag_raw, ICM20948_Axis_t *mag_ut)
{
  HAL_StatusTypeDef s;
  s = ICM20948_ReadAccel(hicm, accel_raw, accel_g);
  if (s != HAL_OK) return s;
  s = ICM20948_ReadGyro(hicm, gyro_raw, gyro_dps);
  if (s != HAL_OK) return s;
  s = ICM20948_ReadMag(hicm, mag_raw, mag_ut);
  return s;
}
