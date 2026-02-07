# 🧩 **STM32 ICM20948 코드 분석!**

### **ICM20948 헤더파일**
```c
#ifndef __ICM20948_H
#define __ICM20948_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* I2C 슬레이브 주소. AD0=LOW면 0x68, HIGH면 0x69. HAL은 7bit<<1 해서 8bit로 넘김. Init에서 둘 다 시도함 */
#define ICM20948_I2C_ADDR_0       (0x68U << 1)
#define ICM20948_I2C_ADDR_1       (0x69U << 1)
/* WHO_AM_I 레지스터에서 읽으면 나와야 하는 칩 고유 ID 값 (0xEA = ICM20948) */
#define ICM20948_WHO_AM_I         0xEAU	  /* 센서 내부 레지스터 주소 반환 값 */

/* 뱅크 선택: 레지스터 접근 전에 0x7F에 쓸 값. 이걸 써야 그 다음 주소가 어느 Bank인지 정해짐 */
#define ICM20948_REG_BANK_SEL     0x7FU	  /* Bank 레지스터 주소를 설정할 때 사용 Bank를 바꾸려고 하면 이게 꼭 필요함 */
#define ICM20948_BANK_0           0x00U   /* 0층: WHO_AM_I, 전원, 가속도/자이로 데이터, 마그 수신 버퍼 등 */
#define ICM20948_BANK_2           0x20U   /* 2층: 자이로/가속도 스케일·필터 설정 */
#define ICM20948_BANK_3           0x30U   /* 3층: 내부 마그(AK09916) 접근용 I2C 마스터·SLV0 설정 */

/* Bank 0 레지스터 주소들 */
#define ICM20948_WHO_AM_I_REG     0x00U   /* 칩 ID 읽기 (0xEA 반환) */
#define ICM20948_USER_CTRL        0x03U   /* I2C 마스터 활성화 등 사용자 제어 */
#define ICM20948_PWR_MGMT_1       0x06U   /* 전원 관리1: 리셋, 슬립, 클럭 소스 선택 */
#define ICM20948_PWR_MGMT_2       0x07U   /* 전원 관리2: 가속도/자이로 축별 온/오프 */
#define ICM20948_ACCEL_XOUT_H     0x2DU   /* 가속도 X high. 0x2D~0x32까지 X,Y,Z 각 2바이트 */
#define ICM20948_GYRO_XOUT_H      0x33U   /* 자이로 X high. 0x33~0x38까지 X,Y,Z 각 2바이트 */
#define ICM20948_EXT_SLV_SENS_DATA_00  0x3BU  /* I2C 마스터로 읽은 마그 데이터가 여기 들어옴 */

/* Bank 2 레지스터: 자이로/가속도 설정 */
#define ICM20948_GYRO_CONFIG_1    0x01U   /* 자이로 측정 범위(FSR), DLPF 설정 */
#define ICM20948_ACCEL_CONFIG     0x14U   /* 가속도 측정 범위(FSR), DLPF 설정 */

/* Bank 3 레지스터: 내부 I2C 마스터로 AK09916(마그) 접근할 때 씀 */
#define ICM20948_I2C_MST_CTRL     0x01U   /* I2C 마스터 클럭 속도 설정 */
#define ICM20948_I2C_SLV0_ADDR    0x03U   /* 슬레이브0 I2C 주소 (읽기비트 포함) */
#define ICM20948_I2C_SLV0_REG     0x04U   /* 슬레이브0에서 읽/쓸 레지스터 주소 */
#define ICM20948_I2C_SLV0_CTRL    0x05U   /* 슬레이브0 동작 제어 (바이트 수, 읽기/쓰기, 활성화) */
#define ICM20948_I2C_SLV0_DO      0x06U   /* 슬레이브0 쓰기 시 보낼 데이터 1바이트 */

/* USER_CTRL 등에서 쓸 비트: I2C 마스터 켜기 / 리셋 / PLL 클럭 선택 */
#define ICM20948_I2C_MST_EN       (1U << 5)
#define ICM20948_DEVICE_RESET     (1U << 7)
#define ICM20948_CLKSEL_PLL       0x01U

/* AK09916: ICM20948 안에 들어있는 자력계. I2C 주소 0x0C, 내부 I2C 마스터로만 접근 */
#define AK09916_I2C_ADDR          0x0CU   /* 마그 칩 I2C 주소 (7bit) */
#define AK09916_ST1               0x10U   /* 마그 데이터 시작 레지스터 (ST1~HZ 8바이트 읽음) */
#define AK09916_CNTL2             0x31U   /* 연속/단발 모드 설정 */
#define AK09916_CNTL3             0x32U   /* 소프트 리셋 */
#define AK09916_CNTL2_CONTINUOUS  0x08U   /* CNTL2에 쓸 값: 연속 측정 모드 */
#define AK09916_CNTL3_SRST        0x01U   /* CNTL3에 쓸 값: 리셋 실행 */
#define AK09916_MAG_SCALE         0.15f   /* raw값 * 이 수 = 마이크로테슬라(uT) */

/* 물리량 변환용 스케일: 4g 범위 가속도, 500dps 자이로 기준 LSB당 값 */
#define ICM20948_ACCEL_SCALE_4G   8192.0f
#define ICM20948_GYRO_SCALE_500   65.5f

/* 축별 값: float는 g/dps/uT 같은 물리 단위, int16_t는 센서 raw */
typedef struct { float x, y, z; } ICM20948_Axis_t;
typedef struct { int16_t x, y, z; } ICM20948_AxisRaw_t;

/* 드라이버 핸들: I2C 핸들, 실제 사용 중인 주소, 가속도/자이로 스케일 보관 */
typedef struct {
  I2C_HandleTypeDef *hi2c;
  uint8_t addr;
  float accel_scale;
  float gyro_scale;
} ICM20948_HandleTypeDef;

/* ICM20948 + 내부 마그(AK09916) 초기화. 0x68/0x69 시도, WHO_AM_I 확인, 리셋·클럭·축·마그 설정 */
HAL_StatusTypeDef ICM20948_Init(ICM20948_HandleTypeDef *hicm, I2C_HandleTypeDef *hi2c);
/* 가속도 읽기. raw=원시값, g=중력 단위. NULL이면 해당 출력 생략 */
HAL_StatusTypeDef ICM20948_ReadAccel(ICM20948_HandleTypeDef *hicm, ICM20948_AxisRaw_t *raw, ICM20948_Axis_t *g);
/* 자이로 읽기. raw=원시값, dps=도/초 단위 */
HAL_StatusTypeDef ICM20948_ReadGyro(ICM20948_HandleTypeDef *hicm, ICM20948_AxisRaw_t *raw, ICM20948_Axis_t *dps);
/* 마그 읽기. ICM20948 내부 I2C 마스터로 AK09916에서 읽어서 raw, uT로 반환 */
HAL_StatusTypeDef ICM20948_ReadMag(ICM20948_HandleTypeDef *hicm, ICM20948_AxisRaw_t *raw, ICM20948_Axis_t *ut);
/* 가속도·자이로·마그 한 번에 읽기 */
HAL_StatusTypeDef ICM20948_ReadAll(ICM20948_HandleTypeDef *hicm,
  ICM20948_AxisRaw_t *accel_raw, ICM20948_Axis_t *accel_g,
  ICM20948_AxisRaw_t *gyro_raw, ICM20948_Axis_t *gyro_dps,
  ICM20948_AxisRaw_t *mag_raw, ICM20948_Axis_t *mag_ut);

#ifdef __cplusplus
}
#endif
```

### **ICM20948 C코드**
```c
#include "icm20948.h"

#define I2C_TIMEOUT_MS  100  /* I2C 통신 대기 시간(ms) */

/* 지금부터 읽/쓸 레지스터가 어느 Bank(층)인지 선택. 0x7F에 bank 값 씀 */
static HAL_StatusTypeDef SelectBank(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t bank)
{
  uint8_t reg = ICM20948_REG_BANK_SEL;              // 0x7FU	/* 센서 내부 레지스터 주소 */
  return HAL_I2C_Mem_Write(hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, &bank, 1, I2C_TIMEOUT_MS);
}

/* 현재 선택된 Bank 안의 레지스터 reg에 1바이트 val 씀 */
static HAL_StatusTypeDef WriteReg(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t reg, uint8_t val)
{
  return HAL_I2C_Mem_Write(hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, I2C_TIMEOUT_MS);  /* reg에 val 1바이트 쓰기 */
}

/* 현재 선택된 Bank 안의 레지스터 reg부터 len바이트 읽어서 buf에 넣음 */
static HAL_StatusTypeDef ReadRegs(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
  return HAL_I2C_Mem_Read(hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, I2C_TIMEOUT_MS);  /* reg부터 len바이트 읽어 buf에 저장 */
}

/* ICM20948 + 내부 마그(AK09916) 초기화. 주소 0x68→0x69 시도, WHO_AM_I 확인, 리셋·클럭·축·Bank2 설정·I2C마스터·마그 리셋·연속모드 후 Bank0 복귀 */
HAL_StatusTypeDef ICM20948_Init(ICM20948_HandleTypeDef *hicm, I2C_HandleTypeDef *hi2c)  // ICM20948_HandleTypeDef = ICM 헤더파일에 있음, I2C_HandleTypeDef = ioc에서 I2C설정하면 생성되는 것!
{
  HAL_StatusTypeDef status;
  uint8_t whoami, val;

  if (hicm == NULL || hi2c == NULL) return HAL_ERROR;       /* 값을 못받으면 에러 처리 */

  hicm->hi2c = hi2c;                    /* I2C 객체 */
  hicm->accel_scale = ICM20948_ACCEL_SCALE_4G;      
  hicm->gyro_scale = ICM20948_GYRO_SCALE_500;
  HAL_Delay(5);         /* 0.005초 */

  /* 0x68 먼저 시도, 실패하면 0x69로 재시도 (AD0 핀에 따라 주소가 다름) */
  hicm->addr = ICM20948_I2C_ADDR_0;         /* I2C 20948센서 주소 */
  status = SelectBank(hi2c, hicm->addr, ICM20948_BANK_0);       /* I2C객체, I2C 20948센서주소, 0x00U Bank0번째 주소 */
  if (status != HAL_OK) {               /* 위에 값이 제대로 읽히면 참 */
    hicm->addr = ICM20948_I2C_ADDR_1;       /* AD0이 VCC에 연결이 되어있으면 실행 */
    status = SelectBank(hi2c, hicm->addr, ICM20948_BANK_0);
  }
  if (status != HAL_OK) return status;

  /* Bank0 WHO_AM_I(0x00) 읽어서 0xEA인지 확인. 아니면 다른 주소로 한 번 더 시도 */
  status = ReadRegs(hi2c, hicm->addr, ICM20948_WHO_AM_I_REG, &whoami, 1);       /* 이거 whoami 이거 그냥 저장인가?? 변수만 생성하고 그다음 뭘 넣은게 없던데 버퍼 저장용인가? */
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
  val = ICM20948_DEVICE_RESET;          /* 이거 Device Reset이 뭔지 그리고 왜 1 << 7인지 */
  WriteReg(hi2c, hicm->addr, ICM20948_PWR_MGMT_1, val);   /* 리셋 명령 전송 */
  HAL_Delay(10);                       /* 리셋 완료 대기 */
  WriteReg(hi2c, hicm->addr, ICM20948_PWR_MGMT_1, ICM20948_CLKSEL_PLL);  /* PLL 클럭 선택, 슬립 해제 */
  HAL_Delay(1);
  WriteReg(hi2c, hicm->addr, ICM20948_PWR_MGMT_2, 0x00);   /* 가속도·자이로 6축 모두 켜기 */

  /* Bank2: 자이로 500dps, 가속도 4g 설정 */
  status = SelectBank(hi2c, hicm->addr, ICM20948_BANK_2);
  if (status != HAL_OK) return status;
  WriteReg(hi2c, hicm->addr, ICM20948_GYRO_CONFIG_1, 0x01U);   /* 자이로 500dps, DLPF */
  WriteReg(hi2c, hicm->addr, ICM20948_ACCEL_CONFIG, 0x01U);    /* 가속도 4g, DLPF */
  status = SelectBank(hi2c, hicm->addr, ICM20948_BANK_0);
  if (status != HAL_OK) return status;

  /* Bank0: I2C 마스터 활성화 (마그 접근용). Bank3: 마스터 클럭 설정 */
  WriteReg(hi2c, hicm->addr, ICM20948_USER_CTRL, ICM20948_I2C_MST_EN);   /* 내부 I2C 마스터 켜기 */
  status = SelectBank(hi2c, hicm->addr, ICM20948_BANK_3);
  if (status != HAL_OK) return status;
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_MST_CTRL, 0x07U);    /* I2C 마스터 클럭 분주 */

  /* SLV0로 AK09916에 CNTL3(0x32)에 0x01 써서 소프트 리셋 */
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_ADDR, AK09916_I2C_ADDR & 0x7FU);  /* 마그 주소 0x0C (쓰기) */
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_REG, AK09916_CNTL3);
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_DO, AK09916_CNTL3_SRST);
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, 0x81);    /* 1바이트 쓰기 + SLV0 활성화 */
  HAL_Delay(10);                       /* 마그 리셋 대기 */
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, 0);      /* SLV0 끄기 */

  /* SLV0로 AK09916 CNTL2(0x31)에 0x08 써서 연속 측정 모드 */
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_ADDR, AK09916_I2C_ADDR & 0x7FU);
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_REG, AK09916_CNTL2);
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_DO, AK09916_CNTL2_CONTINUOUS);   /* 0x08 = 연속 모드 */
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, 0x81);
  HAL_Delay(10);
  WriteReg(hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, 0);      /* 설정 후 SLV0 비활성화 */

  return SelectBank(hi2c, hicm->addr, ICM20948_BANK_0);        /* 사용 후 Bank0으로 복귀 */
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

  if (raw) {			// raw = 센서 값
    raw->x = (int16_t)((uint16_t)buf[0] << 8 | buf[1]);
    raw->y = (int16_t)((uint16_t)buf[2] << 8 | buf[3]);
    raw->z = (int16_t)((uint16_t)buf[4] << 8 | buf[5]);
  }
  if (g) {			// g = 단위
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
  status = ReadRegs(hicm->hi2c, hicm->addr, ICM20948_GYRO_XOUT_H, buf, 6);   /* 0x33부터 6바이트 (X,Y,Z 각 2바이트) */
  if (status != HAL_OK) return status;

  if (raw) {                           /* raw = 자이로 원시값. NULL이면 안 씀 */
    raw->x = (int16_t)((uint16_t)buf[0] << 8 | buf[1]);
    raw->y = (int16_t)((uint16_t)buf[2] << 8 | buf[3]);
    raw->z = (int16_t)((uint16_t)buf[4] << 8 | buf[5]);
  }
  if (dps) {                           /* dps = 도/초 단위. scale로 나누면 °/s */
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
  uint8_t buf[8];                      /* ST1, HXL,HXH, HYL,HYH, HZL,HZH, ST2 순서 8바이트 */

  if (hicm == NULL) return HAL_ERROR;

  status = SelectBank(hicm->hi2c, hicm->addr, ICM20948_BANK_3);   /* 마그 설정은 Bank3 */
  if (status != HAL_OK) return status;
  WriteReg(hicm->hi2c, hicm->addr, ICM20948_I2C_SLV0_ADDR, AK09916_I2C_ADDR | 0x80U);  /* 0x80 = 읽기. AK09916에서 읽기 */
  WriteReg(hicm->hi2c, hicm->addr, ICM20948_I2C_SLV0_REG, AK09916_ST1);                 /* 읽기 시작 주소 ST1(0x10) */
  WriteReg(hicm->hi2c, hicm->addr, ICM20948_I2C_SLV0_CTRL, 0x87);   /* 8바이트 읽기 + SLV0 활성화 */

  status = SelectBank(hicm->hi2c, hicm->addr, ICM20948_BANK_0);
  if (status != HAL_OK) return status;
  HAL_Delay(2);                        /* I2C 마스터가 마그에서 읽어올 시간 대기 */

  status = ReadRegs(hicm->hi2c, hicm->addr, ICM20948_EXT_SLV_SENS_DATA_00, buf, 8);   /* 결과는 0x3B에 쌓임 */
  if (status != HAL_OK) return status;

  if (raw) {                           /* raw = 마그 원시값. AK09916은 Low,High 순 (buf[1]=HXL, buf[2]=HXH) */
    raw->x = (int16_t)((uint16_t)buf[2] << 8 | buf[1]);
    raw->y = (int16_t)((uint16_t)buf[4] << 8 | buf[3]);
    raw->z = (int16_t)((uint16_t)buf[6] << 8 | buf[5]);
  }
  if (ut) {                            /* ut = 마이크로테슬라. raw * 0.15 */
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
  s = ICM20948_ReadAccel(hicm, accel_raw, accel_g);   /* 1) 가속도 */
  if (s != HAL_OK) return s;
  s = ICM20948_ReadGyro(hicm, gyro_raw, gyro_dps);   /* 2) 자이로 */
  if (s != HAL_OK) return s;
  s = ICM20948_ReadMag(hicm, mag_raw, mag_ut);      /* 3) 마그 */
  return s;
}
```