/**
  ******************************************************************************
  * @file    icm20948.h
  * @brief   ICM20948 9-axis IMU driver (Accel, Gyro, Mag/AK09916)
  *
  * - ICM20948: 가속도(4g), 자이로(500dps), 내장 마그(AK09916, I2C 마스터 경유)
  * - I2C 주소: 0x68 또는 0x69 (Init에서 둘 다 시도)
  * - 소프트웨어 필터: 2단 EMA / 이동평균(MA) 중 선택 가능
  ******************************************************************************
  */

#ifndef __ICM20948_H
#define __ICM20948_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* ==================== I2C 주소 ====================
 * AD0=LOW -> 0x68, AD0=HIGH -> 0x69. 브레이크아웃 보드마다 다름.
 * Init 시 0x68 먼저 시도, 실패하면 0x69로 재시도.
 */
#define ICM20948_I2C_ADDR_0       (0x68U << 1)
#define ICM20948_I2C_ADDR_1       (0x69U << 1)
#define ICM20948_I2C_ADDR         ICM20948_I2C_ADDR_0

/* WHO_AM_I(0x00) 읽었을 때 나와야 하는 값. 안 맞으면 센서/주소 오류 */
#define ICM20948_WHO_AM_I         0xEAU

/* ==================== 뱅크 선택 ====================
 * ICM20948 레지스터는 뱅크 0~3으로 나뉨. 레지스터 접근 전 0x7F에 뱅크 값 기록.
 */
#define ICM20948_REG_BANK_SEL     0x7FU
#define ICM20948_BANK_0           0x00U
#define ICM20948_BANK_1           0x10U
#define ICM20948_BANK_2           0x20U
#define ICM20948_BANK_3           0x30U

/* Bank 0 registers */
#define ICM20948_WHO_AM_I_REG     0x00U
#define ICM20948_USER_CTRL        0x03U
#define ICM20948_PWR_MGMT_1       0x06U
#define ICM20948_PWR_MGMT_2       0x07U
#define ICM20948_ACCEL_XOUT_H     0x2DU
#define ICM20948_ACCEL_XOUT_L     0x2EU
#define ICM20948_ACCEL_YOUT_H     0x2FU
#define ICM20948_ACCEL_YOUT_L     0x30U
#define ICM20948_ACCEL_ZOUT_H     0x31U
#define ICM20948_ACCEL_ZOUT_L     0x32U
#define ICM20948_GYRO_XOUT_H      0x33U
#define ICM20948_GYRO_XOUT_L      0x34U
#define ICM20948_GYRO_YOUT_H      0x35U
#define ICM20948_GYRO_YOUT_L      0x36U
#define ICM20948_GYRO_ZOUT_H      0x37U
#define ICM20948_GYRO_ZOUT_L      0x38U
#define ICM20948_EXT_SLV_SENS_DATA_00  0x3BU  /* 마그 데이터는 I2C 마스터로 읽은 뒤 여기서 수신 */

/* Bank 2: 자이로/가속도 스케일·DLPF 설정 */
#define ICM20948_GYRO_CONFIG_1    0x01U
#define ICM20948_ACCEL_CONFIG     0x14U

/* Bank 3: 내부 마그(AK09916) 접근용 I2C 마스터·SLV0 레지스터 */
#define ICM20948_I2C_MST_CTRL     0x01U
#define ICM20948_I2C_SLV0_ADDR    0x03U
#define ICM20948_I2C_SLV0_REG     0x04U
#define ICM20948_I2C_SLV0_CTRL    0x05U

/* USER_CTRL: I2C 마스터 활성화 시 마그 접근 가능 */
#define ICM20948_I2C_MST_EN       (1U << 5)

/* PWR_MGMT_1 */
#define ICM20948_CLKSEL_PLL       0x01U
#define ICM20948_SLEEP            (1U << 6)
#define ICM20948_DEVICE_RESET     (1U << 7)

/* AK09916 (magnetometer) I2C address and registers */
#define AK09916_I2C_ADDR          0x0CU
#define AK09916_WIA2              0x01U   /* Who am I -> 0x09 */
#define AK09916_ST1               0x10U
#define AK09916_HXL               0x11U
#define AK09916_CNTL2             0x31U
#define AK09916_CNTL3             0x32U
#define AK09916_CNTL2_CONTINUOUS  0x08U   /* Continuous mode 100Hz */
#define AK09916_CNTL3_SRST        0x01U   /* Soft reset */

/* ==================== 스케일 (raw -> 물리량) ====================
 * 가속도: LSB/g, 자이로: LSB/(dps), 마그: 0.15 uT/LSB
 */
#define ICM20948_ACCEL_SCALE_2G   16384.0f   /* LSB/g */
#define ICM20948_ACCEL_SCALE_4G   8192.0f
#define ICM20948_ACCEL_SCALE_8G   4096.0f
#define ICM20948_ACCEL_SCALE_16G  2048.0f
#define ICM20948_GYRO_SCALE_250   131.0f     /* LSB/(dps) */
#define ICM20948_GYRO_SCALE_500   65.5f
#define ICM20948_GYRO_SCALE_1000  32.8f
#define ICM20948_GYRO_SCALE_2000  16.4f
#define AK09916_MAG_SCALE         0.15f      /* uT/LSB */

/* ==================== 필터 모드 ====================
 * EMA: 2단 지수이동평균. alpha 작을수록 부드럽고 지연 증가.
 * MA:  이동평균. 최근 N개 평균. 고정 시 노이즈 감소에 유리.
 */
#define ICM20948_FILTER_EMA       0U   /* 2-stage exponential moving average */
#define ICM20948_FILTER_MA       1U   /* Moving average (better when still) */
#define ICM20948_MA_MAX_LEN      16U  /* max window for MA */

/* ==================== 데이터 구조 ==================== */
typedef struct {
  float x, y, z;      /* 물리 단위 (g, dps, uT) */
} ICM20948_Axis_t;

typedef struct {
  int16_t x, y, z;    /* 센서 raw 값 */
} ICM20948_AxisRaw_t;

/* 드라이버 핸들: I2C, 주소, 스케일, 필터 상태/버퍼 모두 포함 */
typedef struct {
  I2C_HandleTypeDef *hi2c;
  uint8_t addr;                   /* 실제 사용 중인 I2C 주소 (0x68 or 0x69 << 1) */
  float accel_scale;
  float gyro_scale;
  /* ---- 필터: 0=끔, 1=켬. mode로 EMA/MA 선택 ---- */
  uint8_t filter_enabled;
  uint8_t filter_mode;             /* ICM20948_FILTER_EMA or ICM20948_FILTER_MA */
  float filter_alpha;             /* EMA용 (0~1, 작을수록 부드러움) */
  uint8_t filter_initialized;     /* EMA 1단: bit0,1,2 / 2단: bit4,5,6 (accel,gyro,mag) */
  ICM20948_Axis_t filter_accel, filter_gyro, filter_mag;   /* EMA 1단 상태 */
  ICM20948_Axis_t filter_accel2, filter_gyro2, filter_mag2; /* EMA 2단 상태 */
  /* ---- 이동평균(MA) 전용: axis 0~2 accel, 3~5 gyro, 6~8 mag ---- */
  uint8_t ma_window;               /* 윈도우 크기 4~16 */
  uint8_t ma_len[9];               /* 축별로 채워진 샘플 수 */
  uint8_t ma_idx[9];               /* 원형 버퍼 쓰기 위치 */
  float ma_sum[9];
  float ma_buf[9][ICM20948_MA_MAX_LEN];
} ICM20948_HandleTypeDef;

/* 필터 설정: EMA(2단) — alpha 0.2~0.3 보통, 0.1 더 부드러움 */
void ICM20948_SetFilter(ICM20948_HandleTypeDef *hicm, uint8_t enable, float alpha);
/* 필터 설정: 이동평균 — window 8~12 권장, 4~16 가능 */
void ICM20948_SetFilterMovingAvg(ICM20948_HandleTypeDef *hicm, uint8_t enable, uint8_t window);
/* 필터 내부 상태만 초기화 (다음 샘플부터 다시 스무딩) */
void ICM20948_ResetFilter(ICM20948_HandleTypeDef *hicm);

/* ==================== 센서 읽기 API ==================== */
HAL_StatusTypeDef ICM20948_Init(ICM20948_HandleTypeDef *hicm, I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef ICM20948_ReadAccel(ICM20948_HandleTypeDef *hicm, ICM20948_AxisRaw_t *raw, ICM20948_Axis_t *g);
HAL_StatusTypeDef ICM20948_ReadGyro(ICM20948_HandleTypeDef *hicm, ICM20948_AxisRaw_t *raw, ICM20948_Axis_t *dps);
HAL_StatusTypeDef ICM20948_ReadMag(ICM20948_HandleTypeDef *hicm, ICM20948_AxisRaw_t *raw, ICM20948_Axis_t *ut);
HAL_StatusTypeDef ICM20948_ReadAll(ICM20948_HandleTypeDef *hicm,
  ICM20948_AxisRaw_t *accel_raw, ICM20948_Axis_t *accel_g,
  ICM20948_AxisRaw_t *gyro_raw, ICM20948_Axis_t *gyro_dps,
  ICM20948_AxisRaw_t *mag_raw, ICM20948_Axis_t *mag_ut);

#ifdef __cplusplus
}
#endif

#endif /* __ICM20948_H */
