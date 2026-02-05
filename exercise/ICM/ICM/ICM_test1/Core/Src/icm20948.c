/**
  ******************************************************************************
  * @file    icm20948.c
  * @brief   ICM20948 driver — Init, Read Accel/Gyro/Mag (no filter)
  ******************************************************************************
  */

#include "icm20948.h"

#define I2C_TIMEOUT_MS  100  /* I2C 통신 대기 시간(ms) */

/* 지금부터 읽/쓸 레지스터가 어느 Bank(층)인지 선택. 0x7F에 bank 값 씀 */
static HAL_StatusTypeDef SelectBank(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t bank)
{
  uint8_t reg = ICM20948_REG_BANK_SEL;
  return HAL_I2C_Mem_Write(hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, &bank, 1, I2C_TIMEOUT_MS);
}

/* 현재 선택된 Bank 안의 레지스터 reg에 1바이트 val 씀 */
static HAL_StatusTypeDef WriteReg(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t reg, uint8_t val)
{
  return HAL_I2C_Mem_Write(hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, I2C_TIMEOUT_MS);
}

/* 현재 선택된 Bank 안의 레지스터 reg부터 len바이트 읽어서 buf에 넣음 */
static HAL_StatusTypeDef ReadRegs(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
  return HAL_I2C_Mem_Read(hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, I2C_TIMEOUT_MS);
}

/* ICM20948 + 내부 마그(AK09916) 초기화. 주소 0x68→0x69 시도, WHO_AM_I 확인, 리셋·클럭·축·Bank2 설정·I2C마스터·마그 리셋·연속모드 후 Bank0 복귀 */
HAL_StatusTypeDef ICM20948_Init(ICM20948_HandleTypeDef *hicm, I2C_HandleTypeDef *hi2c)
{
  HAL_StatusTypeDef status;
  uint8_t whoami, val;

  if (hicm == NULL || hi2c == NULL) return HAL_ERROR;

  hicm->hi2c = hi2c;
  hicm->accel_scale = ICM20948_ACCEL_SCALE_4G;
  hicm->gyro_scale = ICM20948_GYRO_SCALE_500;
  HAL_Delay(5);

  /* 0x68 먼저 시도, 실패하면 0x69로 재시도 (AD0 핀에 따라 주소가 다름) */
  hicm->addr = ICM20948_I2C_ADDR_0;
  status = SelectBank(hi2c, hicm->addr, ICM20948_BANK_0);
  if (status != HAL_OK) {
    hicm->addr = ICM20948_I2C_ADDR_1;
    status = SelectBank(hi2c, hicm->addr, ICM20948_BANK_0);
  }
  if (status != HAL_OK) return status;

  /* Bank0 WHO_AM_I(0x00) 읽어서 0xEA인지 확인. 아니면 다른 주소로 한 번 더 시도 */
  status = ReadRegs(hi2c, hicm->addr, ICM20948_WHO_AM_I_REG, &whoami, 1);
  if (status != HAL_OK) return status;
  if (whoami != ICM20948_WHO_AM_I) {
    if (hicm->addr == ICM20948_I2C_ADDR_0) hicm->addr = ICM20948_I2C_ADDR_1;
    else return HAL_ERROR;
    status = SelectBank(hi2c, hicm->addr, ICM20948_BANK_0);
    if (status != HAL_OK) return status;
    status = ReadRegs(hi2c, hicm->addr, ICM20948_WHO_AM_I_REG, &whoami, 1);
    if (status != HAL_OK || whoami != ICM20948_WHO_AM_I) return HAL_ERROR;
  }

  /* PWR_MGMT_1: 리셋 후 PLL 클럭 선택·슬립 해제, PWR_MGMT_2: 축 전부 켜기 */
  val = ICM20948_DEVICE_RESET;
  WriteReg(hi2c, hicm->addr, ICM20948_PWR_MGMT_1, val);
  HAL_Delay(10);
  WriteReg(hi2c, hicm->addr, ICM20948_PWR_MGMT_1, ICM20948_CLKSEL_PLL);
  HAL_Delay(1);
  WriteReg(hi2c, hicm->addr, ICM20948_PWR_MGMT_2, 0x00);

  /* Bank2: 자이로 500dps, 가속도 4g 설정 */
  status = SelectBank(hi2c, hicm->addr, ICM20948_BANK_2);
  if (status != HAL_OK) return status;
  WriteReg(hi2c, hicm->addr, ICM20948_GYRO_CONFIG_1, 0x01U);
  WriteReg(hi2c, hicm->addr, ICM20948_ACCEL_CONFIG, 0x01U);
  status = SelectBank(hi2c, hicm->addr, ICM20948_BANK_0);
  if (status != HAL_OK) return status;

  /* Bank0: I2C 마스터 활성화 (마그 접근용). Bank3: 마스터 클럭 설정 */
  WriteReg(hi2c, hicm->addr, ICM20948_USER_CTRL, ICM20948_I2C_MST_EN);
  status = SelectBank(hi2c, hicm->addr, ICM20948_BANK_3);
  if (status != HAL_OK) return status;
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_MST_CTRL, 0x07U);

  /* SLV0로 AK09916에 CNTL3(0x32)에 0x01 써서 소프트 리셋 */
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_ADDR, AK09916_I2C_ADDR & 0x7FU);
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_REG, AK09916_CNTL3);
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_DO, AK09916_CNTL3_SRST);
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, 0x81);
  HAL_Delay(10);
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, 0);

  /* SLV0로 AK09916 CNTL2(0x31)에 0x08 써서 연속 측정 모드 */
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_ADDR, AK09916_I2C_ADDR & 0x7FU);
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_REG, AK09916_CNTL2);
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_DO, AK09916_CNTL2_CONTINUOUS);
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, 0x81);
  HAL_Delay(10);
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, 0);

  return SelectBank(hi2c, hicm->addr, ICM20948_BANK_0);
}

/* Bank0에서 가속도 0x2D~0x32 6바이트 읽어서 raw와 g 단위로 채움. 필터 없음 */
HAL_StatusTypeDef ICM20948_ReadAccel(ICM20948_HandleTypeDef *hicm, ICM20948_AxisRaw_t *raw, ICM20948_Axis_t *g)
{
  uint8_t buf[6];
  HAL_StatusTypeDef status;

  if (hicm == NULL) return HAL_ERROR;
  status = SelectBank(hicm->hi2c, hicm->addr, ICM20948_BANK_0);
  if (status != HAL_OK) return status;
  status = ReadRegs(hicm->hi2c, hicm->addr, ICM20948_ACCEL_XOUT_H, buf, 6);
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
  }
  return HAL_OK;
}

/* Bank0에서 자이로 0x33~0x38 6바이트 읽어서 raw와 dps(도/초) 단위로 채움 */
HAL_StatusTypeDef ICM20948_ReadGyro(ICM20948_HandleTypeDef *hicm, ICM20948_AxisRaw_t *raw, ICM20948_Axis_t *dps)
{
  uint8_t buf[6];
  HAL_StatusTypeDef status;

  if (hicm == NULL) return HAL_ERROR;
  status = SelectBank(hicm->hi2c, hicm->addr, ICM20948_BANK_0);
  if (status != HAL_OK) return status;
  status = ReadRegs(hicm->hi2c, hicm->addr, ICM20948_GYRO_XOUT_H, buf, 6);
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
  }
  return HAL_OK;
}

/* Bank3에서 SLV0로 AK09916 마그 8바이트 읽기 설정 → Bank0 0x3B에서 결과 읽어서 raw와 uT로 채움 */
HAL_StatusTypeDef ICM20948_ReadMag(ICM20948_HandleTypeDef *hicm, ICM20948_AxisRaw_t *raw, ICM20948_Axis_t *ut)
{
  HAL_StatusTypeDef status;
  uint8_t buf[8];

  if (hicm == NULL) return HAL_ERROR;

  status = SelectBank(hicm->hi2c, hicm->addr, ICM20948_BANK_3);
  if (status != HAL_OK) return status;
  WriteReg(hicm->hi2c, hicm->addr, ICM20948_I2C_SLV0_ADDR, AK09916_I2C_ADDR | 0x80U);
  WriteReg(hicm->hi2c, hicm->addr, ICM20948_I2C_SLV0_REG, AK09916_ST1);
  WriteReg(hicm->hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, 0x87);

  status = SelectBank(hicm->hi2c, hicm->addr, ICM20948_BANK_0);
  if (status != HAL_OK) return status;
  HAL_Delay(2);

  status = ReadRegs(hicm->hi2c, hicm->addr, ICM20948_EXT_SLV_SENS_DATA_00, buf, 8);
  if (status != HAL_OK) return status;

  if (raw) {
    raw->x = (int16_t)((uint16_t)buf[2] << 8 | buf[1]);
    raw->y = (int16_t)((uint16_t)buf[4] << 8 | buf[3]);
    raw->z = (int16_t)((uint16_t)buf[6] << 8 | buf[5]);
  }
  if (ut) {
    ut->x = (float)((int16_t)((uint16_t)buf[2] << 8 | buf[1])) * AK09916_MAG_SCALE;
    ut->y = (float)((int16_t)((uint16_t)buf[4] << 8 | buf[3])) * AK09916_MAG_SCALE;
    ut->z = (float)((int16_t)((uint16_t)buf[6] << 8 | buf[5])) * AK09916_MAG_SCALE;
  }
  return HAL_OK;
}

/* 가속도·자이로·마그 순서로 한 번에 읽기. 셋 다 성공 시 HAL_OK */
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
