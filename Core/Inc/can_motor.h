#ifndef __CAN_MOTOR_H
#define __CAN_MOTOR_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* ================= 配置区域 ================= */
// 大疆电调 CAN ID 定义
#define CAN_MOTOR_ID_1          1       // 电调拨码为 1
#define CAN_MOTOR_ID_2          2       // 电调拨码为 2
#define CAN_MOTOR_ID_3          3       // 电调拨码为 3
#define CAN_MOTOR_ID_4          4      // 电调拨码为 4

// CAN 接收 ID (C610/C620 默认反馈 ID)
// ID 1,2 回复到 0x201; ID 3,4 回复到 0x202
#define CAN_RX_MSG_ID_1_2       0x201
#define CAN_RX_MSG_ID_3_4       0x202

// CAN 发送 ID (控制指令 ID)
// 发送 0x200 控制 ID 1-4; 0x201 控制 ID 5-8 (本例只用 1-4)
#define CAN_TX_MSG_ID           0x200

// 最大电流限制 (单位：协议数值，16384 对应电调最大电流)
// 建议初始调试设为 5000 (约 5A)，稳定后再加大
#define MOTOR_MAX_CURRENT       8000    
#define MOTOR_STOP_CURRENT      0

/* ================= 数据结构 ================= */
typedef struct {
    uint16_t pos_raw;       // 原始位置 (0-8191)
    int32_t pos_total;      // 多圈累计位置 (用于位置环)
    int16_t speed_rpm;      // 速度 (rpm)
    int16_t current_raw;    // 原始电流 (mA)
    float current_filt;     // 滤波后电流 (可选)
    bool is_online;         // 在线标志位 (超时未收到数据置 false)
    uint32_t last_update_time; // 上次收到数据的时间戳
} Motor_Data_t;

// 电机实例数组 (支持 4 个电机)
extern Motor_Data_t motor_data[4];

/* ================= 函数声明 ================= */
void CAN_Motor_Init(void);                  // 初始化 CAN 和 NVIC
void CAN_Motor_StartReceive(void);          // 开启接收中断
void CAN_Motor_SendCurrent(int16_t iq[4]);  // 发送 4 个电机的电流指令
void CAN_Motor_ProcessFeedback(uint8_t *data, uint32_t id); // 内部解析函数，由中断调用
void CAN_Motor_UpdateOnlineStatus(void);    // 在主循环调用，检测掉线

#endif