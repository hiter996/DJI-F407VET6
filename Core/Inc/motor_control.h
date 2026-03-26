#ifndef __MOTOR_CONTROL_H
#define __MOTOR_CONTROL_H

#include "main.h"
#include "can_motor.h" // 包含电机数据结构定义

/**
 * @brief 电机控制策略更新函数
 * @note 此函数应根据传感器、遥控器或算法计算目标电流，并写入全局变量 motor_iq
 * @param motor_status: 电机当前状态数组 (来自 CAN 反馈)
 * @retval None
 */
void Motor_Control_Update(Motor_Data_t *motor_status);

/**
 * @brief 控制模式设置 (可选扩展)
 * @param mode: 0=停止, 1=恒流测试, 2=速度闭环, 3=位置闭环
 */
void Motor_Control_SetMode(uint8_t mode);

#endif /* __MOTOR_CONTROL_H */