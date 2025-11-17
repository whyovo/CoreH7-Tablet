#include "redirect.h"

#if (DEBUG_OUTPUT_MODE == 2)

#include <string.h>

/* newlib / standard I/O 重定向：实现 _write，printf 等会使用它 */
int _write(int file, char *ptr, int len)
{
    (void)file;
    if (ptr == NULL || len <= 0)
        return 0;

    HAL_StatusTypeDef status = HAL_UART_Transmit(DEBUG_UART, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
    if (status == HAL_OK)
        return len;
    else
        return -1;
}

/* 兼容 fputc / putchar 的实现（有些库使用 fputc） */
int fputc(int ch, FILE *f)
{
    (void)f;
    uint8_t c = (uint8_t)ch;
    HAL_UART_Transmit(DEBUG_UART, &c, 1, HAL_MAX_DELAY);
    return ch;
}

#endif /* DEBUG_OUTPUT_MODE == 2 */