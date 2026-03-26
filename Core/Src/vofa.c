#include "vofa.h"

// 静态缓冲区，防止栈溢出，并保证 4 字节对齐 (F407 要求)
static uint8_t vofa_tx_buf[VOFA_DMA_BUF_SIZE] __attribute__((aligned(4)));
static uint8_t is_dma_busy = 0; // 简单的标志位，防止 DMA 冲突

/**
 * @brief 初始化 VOFA (实际上只需确保 UART 已初始化)
 */
void VOFA_Init(void) {
    // 如果 CubeMX 已经生成了 MX_USART1_UART_Init()，这里通常不需要额外操作
    // 可以在这里检查 DMA 是否使能
}

/**
 * @brief 发送 RawData 协议数据到 VOFA+
 * @param data: 浮点数数组指针
 * @param len:  通道数量 (例如 3 代表 3 个波形)
 */
void VOFA_Send(float *data, uint8_t len) {
    if (len == 0 || len > 10) return; // 限制最大通道数，防止溢出

    // 如果上一次 DMA 还没发完，为了安全起见，本次丢弃或等待
    // 在 1kHz 控制环 + 921600 波特率下，通常不会发生拥堵
    if (is_dma_busy) {
        // 可选：return; (直接丢弃旧帧，保证实时性)
        // 或者强制终止上一帧 (不推荐，可能导致波形撕裂)
        return; 
    }

    uint8_t index = 0;
    uint8_t checksum = 0;
    uint8_t data_len = len * 4; // float 是 4 字节

    // 1. 帧头 0x55 0xAA
    vofa_tx_buf[index++] = 0x55;
    vofa_tx_buf[index++] = 0xAA;

    // 2. 协议类型 0x00 (RawData)
    vofa_tx_buf[index++] = 0x00;

    // 3. 数据长度
    vofa_tx_buf[index++] = data_len;

    // 4. 拷贝数据 (memcpy 比循环赋值快且安全)
    memcpy(&vofa_tx_buf[index], data, data_len);

    // 5. 计算校验和 (从类型字节开始算起)
    for (int i = 2; i < index + data_len; i++) {
        checksum += vofa_tx_buf[i];
    }
    vofa_tx_buf[index + data_len] = checksum;

    // 6. 启动 DMA 发送
    uint16_t total_len = index + data_len + 1;
    is_dma_busy = 1;
    
    // 注意：HAL_UART_Transmit_DMA 是非阻塞的
    HAL_UART_Transmit_DMA(&VOFA_UART, vofa_tx_buf, total_len);
}

/**
 * @brief DMA 发送完成回调 (自动清除 busy 标志)
 * @note 此函数需放在本文件或 main.c 中，并确保被调用
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == VOFA_UART.Instance) {
        is_dma_busy = 0;
    }
}