#include "ems22a.h"

uint8_t ems22a_check_parity(ems22a_frame *f)
{
	int pctr = 0;

	for (int i = 1; i < 16; ++i) {
		uint16_t mask = 1 << i;
		if (f->word & mask) pctr++;
	}

	if (pctr % 2 == f->data.parity) return 0;
	else return 1;
}

void ems22a_receive(ems22a_frame frames[], uint8_t frame_count)
{
    spiSelect(&SPID1);
    spiReceive(&SPID1, frame_count+1, (uint16_t *)frames);
    spiUnselect(&SPID1);

    /* 17 bit to 16 bit data conversion */
    uint16_t tmp;
    for (uint8_t i = 0; i < frame_count+1; i++) {
        tmp = frames[i].word >> (16-i); // Only keep the bits which will get moved to the previous frame
        frames[i].word <<= (i+1);
        if (i > 0) {
            frames[i-1].word += tmp;
            frames[i-1].data.parity = ems22a_check_parity(&frames[i-1]);
        }
    }
}
