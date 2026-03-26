#ifndef __DEBUG_MONITOR_H
#define __DEBUG_MONITOR_H

#include "main.h"
#include <stdint.h>

/**
 * @brief 初始化调试监控模块
 * @note 通常只需调用一次，用于重置内部计时器
 */
void Debug_Monitor_Init(void);

/**
 * @brief 更新并发送调试数据到 VOFA+
 * @note 此函数内部包含计时逻辑，建议在主循环 while(1) 中周期性调用
 *       它会自动判断是否达到发送间隔，若未达到则直接返回，不执行发送。
 * @retval None
 */
void Debug_Monitor_Update(void);

#endif /* __DEBUG_MONITOR_H */