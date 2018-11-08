#ifndef JOYSTICK_H_INCLUDED
#define JOYSTICK_H_INCLUDED

#include "ems22a.h"
#include "mlx90393.h"
#include "lsm303.h"

float xy_to_hdg(float x, float y);
uint8_t read_buttons(void);
THD_FUNCTION(lsm303c_thread, arg);
THD_FUNCTION(lsm303dlhc_thread, arg);
THD_FUNCTION(mlx90393_thread, arg);
THD_FUNCTION(dummy_thread, arg);
THD_FUNCTION(ems22a_thread, arg);

#endif // JOYSTICK_H_INCLUDED
