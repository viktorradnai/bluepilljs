#ifndef JOYSTICK_H_INCLUDED
#define JOYSTICK_H_INCLUDED

#include "ems22a.h"
#include "mlx90393.h"
#include "lsm303.h"


float xy_to_hdg(float x, float y);
uint8_t read_buttons(void);

#endif // JOYSTICK_H_INCLUDED
