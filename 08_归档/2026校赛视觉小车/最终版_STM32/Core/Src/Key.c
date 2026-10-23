#include "Key.h"

/**
 * @brief  非阻塞按键检测
 * @retval 按键值：0=无按键，1=KEY1(PB0)，2=KEY2(PB1)
 * @note   下降沿触发（按下时返回一次），5ms消抖（HAL_GetTick计），松开后允许再次触发
 */
uint8_t Key_GetNum(void)
{
    static uint8_t last0 = GPIO_PIN_SET, last1 = GPIO_PIN_SET;
    static uint8_t debouncing0 = 0, debouncing1 = 0;
    static uint32_t dbTimer0 = 0, dbTimer1 = 0;
    static uint8_t triggered0 = 0, triggered1 = 0;

    uint8_t cur0 = HAL_GPIO_ReadPin(KEY_1_GPIOX, KEY_1_GPIO_PIN);
    uint8_t cur1 = HAL_GPIO_ReadPin(KEY_2_GPIOX, KEY_2_GPIO_PIN);
    uint8_t key = 0;

    // ========== PB0 ==========
    // 检测下降沿（按键按下瞬间）
    if (last0 == GPIO_PIN_SET && cur0 == GPIO_PIN_RESET && !debouncing0)
    {
        debouncing0 = 1;
        dbTimer0 = HAL_GetTick();
    }
    last0 = cur0;

    // 消抖：5ms 后确认（快速响应）
    if (debouncing0 && HAL_GetTick() - dbTimer0 >= 5)
    {
        if (cur0 == GPIO_PIN_RESET && !triggered0)
        {
            key = 1;       // 返回按键值
            triggered0 = 1; // 标记已触发，防止重复
        }
        debouncing0 = 0;
    }

    // 松开后清除标志，允许下次触发
    if (cur0 == GPIO_PIN_SET)
        triggered0 = 0;

    if (key) return key;  // PB0 优先

    // ========== PB1 ==========
    if (last1 == GPIO_PIN_SET && cur1 == GPIO_PIN_RESET && !debouncing1)
    {
        debouncing1 = 1;
        dbTimer1 = HAL_GetTick();
    }
    last1 = cur1;

    if (debouncing1 && HAL_GetTick() - dbTimer1 >= 5)
    {
        if (cur1 == GPIO_PIN_RESET && !triggered1)
        {
            key = 2;
            triggered1 = 1;
        }
        debouncing1 = 0;
    }

    if (cur1 == GPIO_PIN_SET)
        triggered1 = 0;

    return key;
}