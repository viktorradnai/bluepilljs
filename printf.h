#ifndef PRINTF_H_INCLUDED
#define PRINTF_H_INCLUDED

#define CHPRINTF_USE_FLOAT 1
#include "chprintf.h"
extern SerialUSBDriver SDU1;
#define printf(args...) chSysLock(); chprintf((BaseSequentialStream *)&SDU1, args); chSysUnlock()

#endif // PRINTF_H_INCLUDED
