/**
 * @file mpu6050.c
 * @author 菜菜why（B站：菜菜whyy）
 * @brief MPU6050 统一驱动实现
 *
 * @warning 【重要提示】
 *          开启卡尔曼滤波模式(MPU_USE_KALMAN)时，初始化包含零偏校准过程。
 *          请务必在模块上电后的前 3 秒内保持模块绝对静止！
 *          否则会导致角度漂移严重。
 * 同时，如果采用dmp库，如果自检失败，也会启用软件校准以减少零偏。需要等3s
 */

#include "mpu6050.h"
#include <math.h>

#ifdef MPU6050_I2C_ENABLE

#define MPU_ADDR 0xD0 // MPU6050 I2C地址 (AD0接地)

/* =================================================================================
 *                               底层 I2C 接口实现
 * ================================================================================= */

#ifdef MPU_USE_HARD_I2C
/* ---------------- 硬件 I2C ---------------- */

/**
 * @brief 写寄存器 (多字节)
 */
uint8_t MPU_Write_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
	return HAL_I2C_Mem_Write(&MPU_HARD_HANDLE, addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
}

/**
 * @brief 读寄存器 (多字节)
 */
uint8_t MPU_Read_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
	return HAL_I2C_Mem_Read(&MPU_HARD_HANDLE, addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
}

#elif defined(MPU_USE_SOFT_I2C)
/* ---------------- 软件 I2C ---------------- */
#include "soft_i2c.h"
static SOFT_I2C_Handle mpu_i2c;

static void MPU_Soft_Init_GPIO(void)
{
	SOFT_I2C_ConfigHandle(&mpu_i2c, MPU_SOFT_SCL_PORT, MPU_SOFT_SCL_PIN,
						  MPU_SOFT_SDA_PORT, MPU_SOFT_SDA_PIN, MPU_SOFT_DELAY_US);
}

uint8_t MPU_Write_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
	uint8_t i;
	SOFT_I2C_Start(&mpu_i2c);
	SOFT_I2C_WriteByte(&mpu_i2c, addr);
	if (SOFT_I2C_WaitAck(&mpu_i2c))
	{
		SOFT_I2C_Stop(&mpu_i2c);
		return 1;
	}
	SOFT_I2C_WriteByte(&mpu_i2c, reg);
	SOFT_I2C_WaitAck(&mpu_i2c);
	for (i = 0; i < len; i++)
	{
		SOFT_I2C_WriteByte(&mpu_i2c, buf[i]);
		if (SOFT_I2C_WaitAck(&mpu_i2c))
		{
			SOFT_I2C_Stop(&mpu_i2c);
			return 1;
		}
	}
	SOFT_I2C_Stop(&mpu_i2c);
	return 0;
}

uint8_t MPU_Read_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
	SOFT_I2C_Start(&mpu_i2c);
	SOFT_I2C_WriteByte(&mpu_i2c, addr);
	if (SOFT_I2C_WaitAck(&mpu_i2c))
	{
		SOFT_I2C_Stop(&mpu_i2c);
		return 1;
	}
	SOFT_I2C_WriteByte(&mpu_i2c, reg);
	SOFT_I2C_WaitAck(&mpu_i2c);

	SOFT_I2C_Start(&mpu_i2c);
	SOFT_I2C_WriteByte(&mpu_i2c, addr | 0x01);
	if (SOFT_I2C_WaitAck(&mpu_i2c))
	{
		SOFT_I2C_Stop(&mpu_i2c);
		return 1;
	}

	while (len)
	{
		if (len == 1)
			*buf = SOFT_I2C_ReadByte(&mpu_i2c, 0);
		else
			*buf = SOFT_I2C_ReadByte(&mpu_i2c, 1);
		len--;
		buf++;
	}
	SOFT_I2C_Stop(&mpu_i2c);
	return 0;
}
#endif

/* ---------------- 兼容层 ---------------- */
int i2c_write(unsigned char slave_addr, unsigned char reg_addr, unsigned char length, unsigned char const *data)
{
	return MPU_Write_Len(slave_addr << 1, reg_addr, length, (uint8_t *)data);
}

int i2c_read(unsigned char slave_addr, unsigned char reg_addr, unsigned char length, unsigned char *data)
{
	return MPU_Read_Len(slave_addr << 1, reg_addr, length, data);
}

/* =================================================================================
 *                               DMP 模式实现
 * ================================================================================= */
#ifdef MPU_USE_DMP

#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"

/* 陀螺仪方向设置 */
static signed char gyro_orientation[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};

/* 软件校准标志位与偏移量 */
static uint8_t dmp_use_soft_calibration = 0; // 0:使用官方校准(自检通过), 1:使用软件补偿(自检失败)
static float dmp_pitch_offset = 0.0f;
static float dmp_roll_offset = 0.0f;
static float dmp_yaw_offset = 0.0f;

static unsigned short inv_row_2_scale(const signed char *row)
{
	unsigned short b;
	if (row[0] > 0)
		b = 0;
	else if (row[0] < 0)
		b = 4;
	else if (row[1] > 0)
		b = 1;
	else if (row[1] < 0)
		b = 5;
	else if (row[2] > 0)
		b = 2;
	else if (row[2] < 0)
		b = 6;
	else
		b = 7;
	return b;
}

static unsigned short inv_orientation_matrix_to_scalar(const signed char *mtx)
{
	unsigned short scalar;
	scalar = inv_row_2_scale(mtx);
	scalar |= inv_row_2_scale(mtx + 3) << 3;
	scalar |= inv_row_2_scale(mtx + 6) << 6;
	return scalar;
}

static unsigned char run_self_test(void)
{
	int result;
	long gyro[3], accel[3];

	result = mpu_run_self_test(gyro, accel);

	if (result == 0x3)
	{
		// 自检通过：使用官方库的内部校准
		float sens;
		unsigned short accel_sens;
		mpu_get_gyro_sens(&sens);
		gyro[0] = (long)(gyro[0] * sens);
		gyro[1] = (long)(gyro[1] * sens);
		gyro[2] = (long)(gyro[2] * sens);
		dmp_set_gyro_bias(gyro);
		mpu_get_accel_sens(&accel_sens);
		accel[0] *= accel_sens;
		accel[1] *= accel_sens;
		accel[2] *= accel_sens;
		dmp_set_accel_bias(accel);

		DEBUG_INFO("DMP Self-Test Passed! Using internal calibration.");
		dmp_use_soft_calibration = 0; // 标记不需要软件补偿
		return 0;
	}
	else
	{
		// 自检失败：标记需要软件补偿
		DEBUG_INFO("DMP Self-Test Failed (Result: 0x%02X)! Enabling software compensation...", result);
		dmp_use_soft_calibration = 1; // 标记启用软件补偿
		return 0;					  // 忽略错误继续运行
	}
}

/**
 * @brief DMP 软件校准 (仅在自检失败时调用)
 * @note  采集 300 次 DMP 输出，计算平均值作为初始偏移
 */
static void MPU_DMP_Calibrate(void)
{
	float p, r, y;
	double sum_p = 0, sum_r = 0, sum_y = 0;
	int count = 0;
	const int target_samples = 300; // 采集300个有效样本

	DEBUG_INFO("MPU6050 DMP Calibrating... Please keep static for 3 seconds!");

	// 尝试采集数据，最多尝试 1000 次循环 (防止死循环)
	for (int i = 0; i < 1000 && count < target_samples; i++)
	{
		// 调用 Get_Angle 读取一次 (此时 offset 还是 0)
		if (MPU6050_Get_Angle(&p, &r, &y) == 0)
		{
			sum_p += p;
			sum_r += r;
			sum_y += y;
			count++;
		}
		HAL_Delay(10); // 间隔 10ms
	}

	if (count > 0)
	{
		dmp_pitch_offset = (float)(sum_p / count);
		dmp_roll_offset = (float)(sum_r / count);
		dmp_yaw_offset = (float)(sum_y / count);
		DEBUG_INFO("DMP Calibration Done. Samples: %d", count);
		DEBUG_INFO("Offsets -> Pitch: %.2f, Roll: %.2f, Yaw: %.2f", dmp_pitch_offset, dmp_roll_offset, dmp_yaw_offset);
	}
	else
	{
		DEBUG_ERROR("DMP Calibration Failed! No valid data.");
	}
}

uint8_t MPU6050_Init(void)
{
	uint8_t res = 0;
	struct int_param_s *p = 0;
	uint8_t id = 0;

	DEBUG_INFO("MPU6050 DMP Init Start...");

#ifdef MPU_USE_SOFT_I2C
	MPU_Soft_Init_GPIO();
#endif

	HAL_Delay(100);

	// --- 新增：先读取 WHO_AM_I 确认硬件连接 ---
	if (MPU_Read_Len(MPU_ADDR, 0x75, 1, &id) == 0)
	{
		DEBUG_INFO("MPU6050 ID: 0x%02X", id);
		if (id != 0x68 && id != 0x70)
		{
			DEBUG_ERROR("Warning: ID mismatch (Expected 0x68). Check wiring/AD0.");
		}
	}
	else
	{
		DEBUG_ERROR("I2C Error: Could not read WHO_AM_I register. Check wiring!");
		return 11; // I2C 通讯失败
	}
	// ----------------------------------------
	res = mpu_init(p);
	if (res == 0)
	{
		res = mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
		if (res)
		{
			DEBUG_ERROR("mpu_set_sensors failed: %d", res);
			return 1;
		}

		res = mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
		if (res)
		{
			DEBUG_ERROR("mpu_configure_fifo failed: %d", res);
			return 2;
		}

		res = mpu_set_sample_rate(100);
		if (res)
		{
			DEBUG_ERROR("mpu_set_sample_rate failed: %d", res);
			return 3;
		}

		res = dmp_load_motion_driver_firmware();
		if (res)
		{
			DEBUG_ERROR("dmp_load_firmware failed: %d", res);
			return 4;
		}

		res = dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation));
		if (res)
		{
			DEBUG_ERROR("dmp_set_orientation failed: %d", res);
			return 5;
		}

		res = dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP |
								 DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL |
								 DMP_FEATURE_SEND_CAL_GYRO | DMP_FEATURE_GYRO_CAL);
		if (res)
		{
			DEBUG_ERROR("dmp_enable_feature failed: %d", res);
			return 6;
		}

		res = dmp_set_fifo_rate(100);
		if (res)
		{
			DEBUG_ERROR("dmp_set_fifo_rate failed: %d", res);
			return 7;
		}

		// 运行自检，根据结果决定是否启用软件补偿
		res = run_self_test();
		if (res)
		{
			DEBUG_ERROR("run_self_test failed: %d", res);
			return 8;
		}

		res = mpu_set_dmp_state(1);
		if (res)
		{
			DEBUG_ERROR("mpu_set_dmp_state failed: %d", res);
			return 9;
		}

		// 只有当自检失败（启用软件补偿）时，才执行 3秒 校准
		if (dmp_use_soft_calibration)
		{
			MPU_DMP_Calibrate();
		}

		DEBUG_INFO("MPU6050 DMP Init Success!");
	}
	else
	{
		DEBUG_ERROR("mpu_init failed!return %d", res);
		return 10;
	}
	return 0;
}

uint8_t MPU6050_Get_Angle(float *pitch, float *roll, float *yaw)
{
	float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;
	unsigned long sensor_timestamp;
	short gyro[3], accel[3], sensors;
	unsigned char more;
	long quat[4];
	int res;

	// 读取 FIFO
	res = dmp_read_fifo(gyro, accel, quat, &sensor_timestamp, &sensors, &more);

	if (res != 0)
	{
		return 1;
	}

	if (sensors & INV_WXYZ_QUAT)
	{
		q0 = quat[0] / 1073741824.0f;
		q1 = quat[1] / 1073741824.0f;
		q2 = quat[2] / 1073741824.0f;
		q3 = quat[3] / 1073741824.0f;

		// 计算原始角度
		float raw_pitch = asin(-2 * q1 * q3 + 2 * q0 * q2) * 57.3f;
		float raw_roll = atan2(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2 * q2 + 1) * 57.3f;
		float raw_yaw = atan2(2 * (q1 * q2 + q0 * q3), q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3) * 57.3f;

		// 如果启用了软件补偿（自检失败），则减去偏移量
		if (dmp_use_soft_calibration)
		{
			*pitch = raw_pitch - dmp_pitch_offset;
			*roll = raw_roll - dmp_roll_offset;
			*yaw = raw_yaw - dmp_yaw_offset;
		}
		else
		{
			*pitch = raw_pitch;
			*roll = raw_roll;
			*yaw = raw_yaw;
		}

		return 0;
	}
	return 2;
}

/* =================================================================================
 *                               卡尔曼滤波模式实现
 * ================================================================================= */
#elif defined(MPU_USE_KALMAN)

/* 寄存器定义 */
#define MPU_PWR_MGMT_1 0x6B
#define MPU_SMPLRT_DIV 0x19
#define MPU_CONFIG 0x1A
#define MPU_GYRO_CONFIG 0x1B
#define MPU_ACCEL_CONFIG 0x1C
#define MPU_WHO_AM_I 0x75

/* 全局变量 */
short mpu_aacx, mpu_aacy, mpu_aacz;
short mpu_gyrox, mpu_gyroy, mpu_gyroz;

/* 零偏校准变量 */
static float gyro_x_offset = 0, gyro_y_offset = 0, gyro_z_offset = 0;

/* Yaw 积分变量 */
static float yaw_accumulated = 0.0f;

/* 卡尔曼参数 */
static float Q_angle = 0.001f, Q_gyro = 0.003f, R_angle = 0.5f, dt = 0.01f; // dt需根据调用频率调整 (此处假设10ms)
static float q_bias_x = 0, angle_err_x = 0;
static float PCt_0, PCt_1, E, K_0, K_1, t_0, t_1;
static float P_x[4] = {0, 0, 0, 0};
static float PP_x[2][2] = {{1, 0}, {0, 1}};
static float angle_x_final = 0;

static float q_bias_y = 0, angle_err_y = 0;
static float P_y[4] = {0, 0, 0, 0};
static float PP_y[2][2] = {{1, 0}, {0, 1}};
static float angle_y_final = 0;

/**
 * @brief 卡尔曼滤波器核心算法
 */
static void Kalman_Filter(float Accel, float Gyro, float *Angle_Final, float *Q_bias, float PP[2][2])
{
	Accel = -Accel;
	Gyro = -Gyro;
	*Angle_Final += (Gyro - *Q_bias) * dt;

	float P[4];
	P[0] = Q_angle - PP[0][1] - PP[1][0];
	P[1] = -PP[1][1];
	P[2] = -PP[1][1];
	P[3] = Q_gyro;

	PP[0][0] += P[0] * dt;
	PP[0][1] += P[1] * dt;
	PP[1][0] += P[2] * dt;
	PP[1][1] += P[3] * dt;

	float Angle_err = -Accel - *Angle_Final;

	PCt_0 = PP[0][0];
	PCt_1 = PP[1][0];
	E = R_angle + PCt_0;
	K_0 = PCt_0 / E;
	K_1 = PCt_1 / E;

	t_0 = PCt_0;
	t_1 = PP[0][1];

	PP[0][0] -= K_0 * t_0;
	PP[0][1] -= K_0 * t_1;
	PP[1][0] -= K_1 * t_0;
	PP[1][1] -= K_1 * t_1;

	*Angle_Final += K_0 * Angle_err;
	*Q_bias += K_1 * Angle_err;
}

/**
 * @brief 静态零偏校准
 * @note  在初始化时调用，设备需保持静止
 *        采集 300 次数据，每次间隔 10ms，耗时约 3 秒
 */
static void MPU_Calibrate(void)
{
	int32_t sum_gx = 0, sum_gy = 0, sum_gz = 0;
	uint8_t buf[6];
	const int sample_count = 300; // 采样次数 300次

	DEBUG_INFO("MPU6050 Calibrating... Please keep static for 3 seconds!");

	for (int i = 0; i < sample_count; i++)
	{
		MPU_Read_Len(MPU_ADDR, 0x43, 6, buf); // 读取陀螺仪数据 (0x43 - 0x48)
		sum_gx += (short)((buf[0] << 8) | buf[1]);
		sum_gy += (short)((buf[2] << 8) | buf[3]);
		sum_gz += (short)((buf[4] << 8) | buf[5]);
		HAL_Delay(10); // 间隔 10ms
	}

	// 计算平均零偏
	gyro_x_offset = (float)sum_gx / sample_count;
	gyro_y_offset = (float)sum_gy / sample_count;
	gyro_z_offset = (float)sum_gz / sample_count;

	DEBUG_INFO("Calibration Done. Offsets: X=%.2f, Y=%.2f, Z=%.2f", gyro_x_offset, gyro_y_offset, gyro_z_offset);
}

uint8_t MPU6050_Init(void)
{
	uint8_t id = 0;
	uint8_t data;

	DEBUG_INFO("MPU6050 Init Start..."); // 打印开始标记

#ifdef MPU_USE_SOFT_I2C
	MPU_Soft_Init_GPIO();
#endif
	HAL_Delay(100);

	// 读取器件 ID
	MPU_Read_Len(MPU_ADDR, MPU_WHO_AM_I, 1, &id);

	DEBUG_INFO("MPU6050 ID: 0x%02X", id); // 打印读取到的 ID

	if (id != 0x68 && id != 0x70)
	{
		// 如果 ID 不对，打印错误并尝试强制继续（或者您可以选择 return 1 终止）
		DEBUG_ERROR("MPU6050 ID Error! Please check wiring or AD0 pin.");
		// 如果您确定接线没问题，可能是 AD0 接高电平导致地址变了，或者是软排线接触不良
		return 1;
	}

	data = 0x01;
	MPU_Write_Len(MPU_ADDR, MPU_PWR_MGMT_1, 1, &data); // 唤醒
	data = 0x00;
	MPU_Write_Len(MPU_ADDR, 0x6C, 1, &data); // PWR_MGMT_2
	data = 0x09;
	MPU_Write_Len(MPU_ADDR, MPU_SMPLRT_DIV, 1, &data); // 分频
	data = 0x06;
	MPU_Write_Len(MPU_ADDR, MPU_CONFIG, 1, &data); // 滤波
	data = 0x18;
	MPU_Write_Len(MPU_ADDR, MPU_GYRO_CONFIG, 1, &data); // 2000deg/s
	data = 0x01;
	MPU_Write_Len(MPU_ADDR, MPU_ACCEL_CONFIG, 1, &data); // 2g

	// 执行校准 (耗时约3秒)
	MPU_Calibrate();

	return 0;
}

uint8_t MPU6050_Get_Angle(float *pitch, float *roll, float *yaw)
{
	uint8_t buf[14];
	float accx, accy, accz;
	float gyrox, gyroy, gyroz;
	float angle_x_temp, angle_y_temp;

	if (MPU_Read_Len(MPU_ADDR, 0x3B, 14, buf) != 0)
		return 1;

	mpu_aacx = (buf[0] << 8) | buf[1];
	mpu_aacy = (buf[2] << 8) | buf[3];
	mpu_aacz = (buf[4] << 8) | buf[5];
	mpu_gyrox = (buf[8] << 8) | buf[9];
	mpu_gyroy = (buf[10] << 8) | buf[11];
	mpu_gyroz = (buf[12] << 8) | buf[13];

	// 1. 计算加速度 (g)
	if (mpu_aacx < 32764)
		accx = mpu_aacx / 16384.0f;
	else
		accx = 1 - (mpu_aacx - 49152) / 16384.0f;

	if (mpu_aacy < 32764)
		accy = mpu_aacy / 16384.0f;
	else
		accy = 1 - (mpu_aacy - 49152) / 16384.0f;

	if (mpu_aacz < 32764)
		accz = mpu_aacz / 16384.0f;
	else
		accz = (mpu_aacz - 49152) / 16384.0f;

	// 2. 计算加速度角度 (atan)
	angle_x_temp = (atan(accy / accz)) * 57.3f;
	angle_y_temp = (atan(accx / accz)) * 57.3f;

	// 3. 计算角速度 (deg/s) 并减去零偏
	// 注意：这里需要将原始数据减去 offset，再转换为度/秒
	// 2000deg/s 量程对应的灵敏度是 16.4 LSB/(deg/s)

	float raw_gx = (float)mpu_gyrox - gyro_x_offset;
	float raw_gy = (float)mpu_gyroy - gyro_y_offset;
	float raw_gz = (float)mpu_gyroz - gyro_z_offset;

	// 处理数据溢出/转换逻辑 (保持原有逻辑，但应用 offset)
	// 原有逻辑处理了补码，这里简化处理：直接除以灵敏度即可，因为 short 类型已经处理了负数
	// 但为了兼容你原有的特殊处理逻辑，我们先还原成 float 再处理

	gyrox = raw_gx / 16.4f;
	gyroy = raw_gy / 16.4f;
	gyroz = raw_gz / 16.4f;

	// 4. 卡尔曼滤波 (Pitch & Roll)
	// 注意：MPU6050 安装方向不同，X/Y 轴可能需要互换或取反，请根据实际情况调整
	Kalman_Filter(angle_x_temp, gyrox, &angle_x_final, &q_bias_x, PP_x);
	Kalman_Filter(angle_y_temp, gyroy, &angle_y_final, &q_bias_y, PP_y);

	// 5. 计算 Yaw (Z轴积分)
	// 增加死区处理，防止静止时微小噪声导致漂移
	if (fabs(gyroz) > 0.2f)
	{
		yaw_accumulated += gyroz * dt;
	}

	*pitch = angle_x_final;
	*roll = angle_y_final;
	*yaw = yaw_accumulated;

	return 0;
}

#endif

#endif /* OLED_H */
