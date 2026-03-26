#ifndef __VOFA_H
#define __VOFA_H

#include "main.h"
#include <stdint.h>
#include <string.h>

/* ================= 配置区域 ================= */
// 请确保在 main.c 或 usart.c 中定义了 huart1
extern UART_HandleTypeDef huart1; 
#define VOFA_UART         huart1
#define VOFA_DMA_BUF_SIZE 64

/* ================= 函数声明 ================= */
void VOFA_Init(void);
void VOFA_Send(float *data, uint8_t len);
void VOFA_Send_Var(const char *name, float val); // 可选：JSON 模式调试用

#endif /* __VOFA_H */