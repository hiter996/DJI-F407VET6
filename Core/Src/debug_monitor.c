#include "debug_monitor.h"
#include "vofa.h"       // 依赖之前写的 VOFA 驱动
#include "can_motor.h"  // 依赖电机数据结构 motor_data

/* ================= 配置区域 ================= */
// 发送频率配置
// 假设主循环 delay 为 2ms (500Hz)，设为 5 则表示 10ms 发送一次 (100Hz)
#define VOFA_SEND_INTERVAL_COUNT  5 

// 需要监控的电机索引 (0~3)
#define MONITOR_MOTOR_ID          0 
/* =========================================== */

// 内部静态变量
static uint8_t vofa_timer = 0;

// 外部引用：需要在 main.c 或 can_motor.c 中定义
extern Motor_Data_t motor_data[4]; 
// 注意：motor_iq 通常在 main.c 局部变量，若要在这里访问，
// 建议将其改为全局变量，或者通过参数传入。
// 这里假设你在 main.c 将 motor_iq 改为了全局变量，或者我们通过其他方式获取目标值。
// 为了演示通用性，这里暂时假设 motor_iq 是全局的，稍后在 main.c 中会说明。
extern int16_t motor_iq[4]; 

/**
 * @brief 初始化
 */
void Debug_Monitor_Init(void) {
    vofa_timer = 0;
}

/**
 * @brief 核心更新函数
 */
void Debug_Monitor_Update(void) {
    vofa_timer++;
    
    // 时间未到，直接返回，不执行任何操作
    if (vofa_timer < VOFA_SEND_INTERVAL_COUNT) {
        return;
    }
    
    // 时间到，重置计数器
    vofa_timer = 0;

    // 准备数据数组 (最多支持 VOFA 定义的通道数，这里设为 3)
    float debug_data[3];
    
    // 获取当前监控的电机指针
    Motor_Data_t *motor = &motor_data[MONITOR_MOTOR_ID];

    if (motor->is_online) {
        // 通道 0: 实际转速 (RPM)
        debug_data[0] = (float)motor->speed_rpm;
        
        // 通道 1: 目标电流 (协议值)
        // 注意：确保 motor_iq 是全局变量或在其他地方可访问
        debug_data[1] = (float)motor_iq[MONITOR_MOTOR_ID];
        
        // 通道 2: 实际电流反馈 (协议值)
        debug_data[2] = (float)motor->current_raw;
        
        // 【扩展示例】如果想看温度，可以加通道 3 (需修改 VOFA_Send 长度)
        // debug_data[3] = (float)motor->temperature; 
    } else {
        // 电机掉线，发送 0 保持波形平直，或者发送特定错误码 (如 -9999)
        debug_data[0] = 0.0f;
        debug_data[1] = 0.0f;
        debug_data[2] = 0.0f;
    }

    // 调用 VOFA 发送 (发送 3 个通道)
    VOFA_Send(debug_data, 3);
}