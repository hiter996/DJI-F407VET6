#include "motor_control.h"

/* ================= 配置区域 ================= */
// 测试用的恒定电流值 (单位：协议单位，约 1mA/单位)
#define TEST_CURRENT_IQ   2000  

// 当前控制模式
// 0: 停止 (所有电机归零)
// 1: 恒流测试 (电机1 给固定电流)
// 2: 预留 (例如：速度闭环)
static uint8_t control_mode = 1; 
/* =========================================== */

// 外部引用：在 main.c 中定义的全局电流指令数组
extern int16_t motor_iq[4];

/**
 * @brief 设置控制模式
 */
void Motor_Control_SetMode(uint8_t mode) {
    control_mode = mode;
    
    // 切换模式时，为了安全，先暂时清零
    if (mode == 0) {
        for(int i=0; i<4; i++) motor_iq[i] = 0;
    }
}

/**
 * @brief 核心控制逻辑
 */
void Motor_Control_Update(Motor_Data_t *motor_status) {
    // 临时变量，用于计算本次周期的目标电流
    int16_t target_iq[4] = {0};

    switch (control_mode) {
        case 1: // 【恒流测试模式】(你提供的原始逻辑)
            // 遍历 4 个电机
            for (int i = 0; i < 4; i++) {
                if (motor_status[i].is_online) {
                    // 示例：只给电机 1 (索引0) 发电流，其他保持 0
                    if (i == 0) {
                        target_iq[i] = TEST_CURRENT_IQ;
                    } else {
                        target_iq[i] = 0;
                    }
                } else {
                    // 掉线保护：该电机归零
                    target_iq[i] = 0;
                }
            }
            break;

        case 2: // 【预留：速度闭环模式】
            // 未来可以在这里写 PID 算法
            // target_iq[0] = PID_Calc(&motor_status[0]);
            break;

        case 0: // 【停止模式】
        default:
            // 全部归零
            for(int i=0; i<4; i++) target_iq[i] = 0;
            break;
    }

    // 【重要】将计算好的目标电流写入全局变量，供主循环发送
    // 使用 memcpy 或直接赋值均可
    for(int i=0; i<4; i++) {
        motor_iq[i] = target_iq[i];
    }
}