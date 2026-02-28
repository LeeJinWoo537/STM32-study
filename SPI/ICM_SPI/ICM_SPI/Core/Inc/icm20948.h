/**
  ******************************************************************************
  * @file    icm20948.h
  * @brief   ICM20948 9-axis IMU driver (SPI) - Accelerometer, Gyroscope, Magnetometer
  ******************************************************************************
  */

#ifndef __ICM20948_H
#define __ICM20948_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* ICM20948 레지스터 뱅크 (REG_BANK_SEL로 선택) */
#define ICM20948_BANK_0   0   /* 사용자 설정, who_am_i, 전원 설정, 가속도/자이로 데이터 */
#define ICM20948_BANK_1   1   /* FIFO, DMP, 인터럽트 */
#define ICM20948_BANK_2   2   /* 자이로/가속도 설정, 샘플레이트, DLPF */
#define ICM20948_BANK_3   3   /* I2C 마스터 (예: AK09916 자력계) */

/* Bank 0 레지스터 */
#define ICM20948_WHO_AM_I         0x00   /* 디바이스 ID (읽기: 0xEA) */
#define ICM20948_USER_CTRL        0x03   /* 사용자 제어: I2C 마스터 활성화, FIFO, DMP 리셋 */
#define ICM20948_LP_CONFIG        0x05   /* 저전력 설정: 가속도/자이로 주기, I2C 마스터 주기 */
#define ICM20948_PWR_MGMT_1       0x06   /* 전원 관리 1: 클럭 소스, 슬립, 디바이스 리셋 */
#define ICM20948_PWR_MGMT_2       0x07   /* 전원 관리 2: 축별 가속도/자이로 활성화 */
#define ICM20948_INT_PIN_CFG      0x0F   /* 인터럽트 핀 설정: 극성, 래치, 바이패스(I2C 자력계용) */
#define ICM20948_ACCEL_XOUT_H     0x2D   /* 가속도 X 출력, 상위 바이트 */
#define ICM20948_ACCEL_XOUT_L     0x2E   /* 가속도 X 출력, 하위 바이트 */
#define ICM20948_ACCEL_YOUT_H     0x2F   /* 가속도 Y 출력, 상위 바이트 */
#define ICM20948_ACCEL_YOUT_L     0x30   /* 가속도 Y 출력, 하위 바이트 */
#define ICM20948_ACCEL_ZOUT_H     0x31   /* 가속도 Z 출력, 상위 바이트 */
#define ICM20948_ACCEL_ZOUT_L     0x32   /* 가속도 Z 출력, 하위 바이트 */
#define ICM20948_GYRO_XOUT_H      0x33   /* 자이로 X 출력, 상위 바이트 */
#define ICM20948_GYRO_XOUT_L      0x34   /* 자이로 X 출력, 하위 바이트 */
#define ICM20948_GYRO_YOUT_H      0x35   /* 자이로 Y 출력, 상위 바이트 */
#define ICM20948_GYRO_YOUT_L      0x36   /* 자이로 Y 출력, 하위 바이트 */
#define ICM20948_GYRO_ZOUT_H      0x37   /* 자이로 Z 출력, 상위 바이트 */
#define ICM20948_GYRO_ZOUT_L      0x38   /* 자이로 Z 출력, 하위 바이트 */
#define ICM20948_EXT_SLV_SENS_DATA_00  0x99   /* 외부 슬레이브 센서 데이터 첫 바이트 (예: AK09916 자력) */
#define ICM20948_REG_BANK_SEL     0x7F   /* 레지스터 뱅크 선택 (0~3); 비트 [3:2] = 뱅크 */

/* Bank 2 레지스터 */
#define ICM20948_GYRO_SMPLRT_DIV     0x00   /* 자이로 샘플레이트 분주: 1 + SMPLRT_DIV = 분주비 */
#define ICM20948_GYRO_CONFIG_1       0x01   /* 자이로 설정: 풀스케일(±250/500/1000/2000 dps), DLPF */
#define ICM20948_ACCEL_SMPLRT_DIV_1  0x10   /* 가속도 샘플레이트 분주, 비트 [15:8] */
#define ICM20948_ACCEL_SMPLRT_DIV_2  0x11   /* 가속도 샘플레이트 분주, 비트 [7:0] */
#define ICM20948_ACCEL_CONFIG        0x14   /* 가속도 설정: 풀스케일(±2/4/8/16 g), DLPF */

/* Bank 3 레지스터 - I2C 마스터 */
#define ICM20948_I2C_MST_ODR_CONFIG  0x00   /* I2C 마스터 클럭 속도 (ODR) */
#define ICM20948_I2C_MST_CTRL        0x01   /* I2C 마스터 활성화, 멀티마스터, ES 대기 */
#define ICM20948_I2C_MST_DELAY_CTRL  0x02   /* 슬레이브 읽기와 다음 통신 사이 지연 */
#define ICM20948_I2C_SLV0_ADDR       0x03   /* 슬레이브 0 I2C 7비트 주소 + R/W 비트 */
#define ICM20948_I2C_SLV0_REG        0x04   /* 슬레이브 0 읽기/쓰기할 레지스터 주소 */
#define ICM20948_I2C_SLV0_CTRL       0x05   /* 슬레이브 0 활성화, 길이, 읽기/쓰기, 스왑, 그룹 */
#define ICM20948_I2C_SLV0_DO         0x06   /* 슬레이브 0 데이터 출력 (쓰기 시 보낼 바이트) */

/* WHO_AM_I 기대값 */
#define ICM20948_WHO_AM_I_VALUE   0xEA   /* ICM-20948 디바이스 ID (반환값) */

/* AK09916 자력계 (내장) I2C 주소 및 레지스터 */
#define AK09916_I2C_ADDR         0x0C   /* AK09916 I2C 7비트 주소 (내부 연결) */
#define AK09916_WIA2             0x01   /* AK09916 Who Am I 2 (디바이스 ID) */
#define AK09916_WIA2_VALUE       0x09   /* AK09916 WHO_AM_I 기대값 */
#define AK09916_ST1              0x10   /* 상태 1: 데이터 준비(DRDY), 오버플로우 */
#define AK09916_HXL              0x11   /* 자력계 X 하위 바이트 (HXL~HZH: 0x11~0x16, 다음에 ST2 읽어야 다음 샘플 갱신) */
#define AK09916_ST2              0x18   /* 상태 2: HXL~HZH 읽기 후 반드시 ST2까지 읽어야 다음 측정 데이터 갱신 */
#define AK09916_CNTL2            0x31   /* 제어 2: 모드(절전/단일/연속/자체테스트) */
#define AK09916_CNTL3            0x32   /* 제어 3: 소프트 리셋 */
#define AK09916_CNTL2_SINGLE     0x01   /* CNTL2 값: 단일 측정 모드 */
#define AK09916_CNTL2_CONT_10HZ  0x08   /* CNTL2 값: 연속 측정 10Hz (움직일 때마다 갱신) */
#define AK09916_ST1_DRDY         0x01   /* ST1 비트: 데이터 준비 */

/* Sensor data structure (raw and scaled) */
typedef struct {
  int16_t x;
  int16_t y;
  int16_t z;
} ICM20948_AxisRaw_t;

typedef struct {
  float accel_x;   /* g */
  float accel_y;
  float accel_z;
  float gyro_x;    /* deg/s */
  float gyro_y;
  float gyro_z;
  float mag_x;     /* uT */
  float mag_y;
  float mag_z;
} ICM20948_Data_t;

/* Driver handle */
typedef struct {
  SPI_HandleTypeDef *hspi;
  GPIO_TypeDef *cs_port;
  uint16_t cs_pin;
  uint8_t bank;
} ICM20948_Handle_t;

/* Functions */
HAL_StatusTypeDef ICM20948_Init(ICM20948_Handle_t *hicm, SPI_HandleTypeDef *hspi,
                                GPIO_TypeDef *cs_port, uint16_t cs_pin);
HAL_StatusTypeDef ICM20948_ReadAccelGyro(ICM20948_Handle_t *hicm, ICM20948_Data_t *data);
HAL_StatusTypeDef ICM20948_ReadMagnetometer(ICM20948_Handle_t *hicm, ICM20948_Data_t *data);
void ICM20948_ScaleData(ICM20948_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* __ICM20948_H */
