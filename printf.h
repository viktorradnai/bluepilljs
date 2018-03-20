#ifndef PRINTF_H_INCLUDED
#define PRINTF_H_INCLUDED

#include "chprintf.h"
extern SerialUSBDriver SDU1;
#define printf(args...) chSysLock(); chprintf((BaseSequentialStream *)&SDU1, args); chSysUnlock()

#endif // PRINTF_H_INCLUDED
