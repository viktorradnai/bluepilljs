#ifndef EMS22A_H_INCLUDED
#define EMS22A_H_INCLUDED

#include "hal.h"
#include "ch.h"

#define EMS22A_CHAIN_LEN 2

/* EMS22A data frame layout */
typedef union {
    struct {
        uint16_t
                parity          : 1,
                mag_decrease    : 1,
                mag_increase    : 1,
                linearity_alarm : 1,
                cordic_oflow    : 1,
                end_offst_comp  : 1,
                value           : 10;
    } data;
    uint16_t word;
} ems22a_frame;

uint8_t ems22a_check_parity(ems22a_frame *f);
void ems22a_receive(ems22a_frame frames[], uint8_t frame_count);

#endif // EMS22A_H_INCLUDED
