#ifndef JOYSTICK_H_INCLUDED
#define JOYSTICK_H_INCLUDED

#include "ems22a.h"
#include "mlx90393.h"
#include "lsm303.h"

typedef struct {
    float offset;
    float m_neg;
    float m_pos;
} cal_data_t;

typedef union {
    struct {
        char start[4];
        cal_data_t data;
        char end[4];
    } content;
    char stream[20];
} cal_frame_t;

extern bool_t jscal_switch;
extern cal_data_t *cal_data;
extern cal_frame_t cal_frame;

bool cal_load(void);

THD_FUNCTION(lsm303c_thread, arg);
THD_FUNCTION(lsm303dlhc_thread, arg);
THD_FUNCTION(mlx90393_thread, arg);
THD_FUNCTION(dummy_thread, arg);
THD_FUNCTION(ems22a_thread, arg);

#endif // JOYSTICK_H_INCLUDED
