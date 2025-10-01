#ifndef CONFIG_H
#define CONFIG_H

#include <stdarg.h>
#include <stdint.h>

/* Global function declarations */
extern void UART_Printf(const char *format, ...);
extern uint8_t mode;
extern uint32_t offTime;
extern uint8_t triggerFlg;


#endif /* CONFIG_H */
