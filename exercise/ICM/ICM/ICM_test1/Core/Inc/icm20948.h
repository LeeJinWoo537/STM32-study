/**
  ******************************************************************************
  * @file    icm20948.h
  * @brief   ICM20948 9-axis IMU driver (Accel, Gyro, Mag) — no filter
  ******************************************************************************
  */

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
#define ICM20948_WHO_AM_I         0xEAU

/* 뱅크 선택: 레지스터 접근 전에 0x7F에 쓸 값. 이걸 써야 그 다음 주소가 어느 Bank인지 정해짐 */
#define ICM20948_REG_BANK_SEL     0x7FU
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

#endif /* __ICM20948_H */
