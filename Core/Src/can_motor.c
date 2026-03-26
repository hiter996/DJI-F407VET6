#include "can_motor.h"
#include "string.h"
// 确保 main.h 已经包含了 stm32f4xx_hal.h

// 【修改点 1】变量名改为 hcan1 (适配 F407 CubeMX 默认生成名)
extern CAN_HandleTypeDef hcan1; 

// 电机数据全局数组
Motor_Data_t motor_data[4] = {0};

// 内部辅助变量
static uint16_t last_pos_raw[4] = {0};
static int32_t circle_count[4] = {0};

/**
 * @brief 初始化 CAN 电机驱动
 */
void CAN_Motor_Init(void) {
    memset(motor_data, 0, sizeof(motor_data));
    memset(last_pos_raw, 0, sizeof(last_pos_raw));
    memset(circle_count, 0, sizeof(circle_count));
    
    // 1. 启动 CAN 外设
    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        Error_Handler();
    }
    
    // 2. 【关键修改】配置 CAN 过滤器 (F407 复位后默认关闭，必须配置才能收数据)
    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;      // 接收所有 ID (0x201-0x204)
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;  // F407 双 CAN 必需

    if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK) {
        Error_Handler();
    }

    // 3. 开启接收中断
    CAN_Motor_StartReceive();
}

/**
 * @brief 激活 CAN 接收中断
 */
void CAN_Motor_StartReceive(void) {
    // 只需激活一次，无需在中断里反复激活
    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief 发送电流指令给 4 个电机
 */
void CAN_Motor_SendCurrent(int16_t iq[4]) {
    CAN_TxHeaderTypeDef tx_header;
    uint8_t tx_data[8];

    // 限幅保护
    for (int i = 0; i < 4; i++) {
        if (iq[i] > MOTOR_MAX_CURRENT) iq[i] = MOTOR_MAX_CURRENT;
        if (iq[i] < -MOTOR_MAX_CURRENT) iq[i] = -MOTOR_MAX_CURRENT;
    }

    // 打包数据 (大端模式)
    tx_data[0] = (iq[0] >> 8) & 0xFF;
    tx_data[1] = iq[0] & 0xFF;
    tx_data[2] = (iq[1] >> 8) & 0xFF;
    tx_data[3] = iq[1] & 0xFF;
    tx_data[4] = (iq[2] >> 8) & 0xFF;
    tx_data[5] = iq[2] & 0xFF;
    tx_data[6] = (iq[3] >> 8) & 0xFF;
    tx_data[7] = iq[3] & 0xFF;

    // 配置发送头
    tx_header.StdId = CAN_TX_MSG_ID;      // 0x200
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = 8;
    tx_header.TransmitGlobalTime = DISABLE;

    // 发送
    uint32_t tx_mailbox;
    // 建议增加超时或重试机制，防止邮箱满导致死锁
    if (HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data, &tx_mailbox) != HAL_OK) {
        // 发送失败处理 (可选：记录错误或点亮 LED)
    }
}

/**
 * @brief 解析接收到的 CAN 数据
 */
void CAN_Motor_ProcessFeedback(uint8_t *data, uint32_t id) {
    int motor_idx = -1;

    // 映射 ID 到索引 (大疆 C620 协议)
    // 拨码 1 -> 0x201 -> index 0
    // 拨码 2 -> 0x202 -> index 1
    // 拨码 3 -> 0x203 -> index 2
    // 拨码 4 -> 0x204 -> index 3
    if (id == 0x201)      motor_idx = 0;
    else if (id == 0x202) motor_idx = 1;
    else if (id == 0x203) motor_idx = 2;
    else if (id == 0x204) motor_idx = 3;
    else return; // 非目标 ID

    Motor_Data_t *motor = &motor_data[motor_idx];

    // 解析数据 (Big-Endian)
    uint16_t pos = ((uint16_t)data[0] << 8) | data[1];
    int16_t spd = ((int16_t)data[2] << 8) | data[3];
    int16_t cur = ((int16_t)data[4] << 8) | data[5];
    // uint8_t temp = data[6]; // 温度暂不使用

    // 更新基础数据
    motor->pos_raw = pos;
    motor->speed_rpm = spd;
    motor->current_raw = cur;
    motor->is_online = true;
    motor->last_update_time = HAL_GetTick();

    // 计算多圈位置
    int32_t diff = (int32_t)pos - (int32_t)last_pos_raw[motor_idx];
    
    // 阈值判断 (8192 / 2 = 4096)
    if (diff > 4096) {
        circle_count[motor_idx]--; 
    } else if (diff < -4096) {
        circle_count[motor_idx]++;
    }
    
    last_pos_raw[motor_idx] = pos;
    motor->pos_total = (circle_count[motor_idx] * 8192) + pos;
}

/**
 * @brief 检测电机掉线
 */
void CAN_Motor_UpdateOnlineStatus(void) {
    uint32_t now = HAL_GetTick();
    for (int i = 0; i < 4; i++) {
        if (now - motor_data[i].last_update_time > 200) { 
            motor_data[i].is_online = false;
            motor_data[i].speed_rpm = 0; 
            // 可选：将 current_filt 也清零
        }
    }
}

/**
 * @brief CAN 接收中断回调函数
 * @note 将此函数放在本文件末尾，确保链接器能找到
 *       前提是 stm32f4xx_it.c 中的 CAN1_RX0_IRQHandler 调用了 HAL_CAN_IRQHandler
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    // 确认是 CAN1 触发的中断
    if (hcan->Instance == CAN1) {
        CAN_RxHeaderTypeDef rx_header;
        uint8_t rx_data[8];
        
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK) {
            CAN_Motor_ProcessFeedback(rx_data, rx_header.StdId);
            
            // 【重要提示】
            // 通常不需要在这里重新调用 HAL_CAN_ActivateNotification。
            // HAL 库在处理完中断后会自动保持通知开启，除非你手动关闭了它。
            // 如果发现丢包，请检查是否在别处关闭了通知，或者尝试取消下面这行的注释：
            // HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
        }
    }
}