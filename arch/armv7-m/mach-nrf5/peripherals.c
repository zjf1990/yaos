#include <kernel/module.h>
#include <pinmap.h>

REGISTER_DEVICE(uart, "uart", 1);
REGISTER_DEVICE(gpio, "led", PIN_STATUS_LED);
