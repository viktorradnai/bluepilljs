#include "ems22a.h"

uint8_t ems22a_check_parity(frame *f)
{
	int pctr = 0;

	for (int i = 1; i < 16; ++i) {
		uint16_t mask = 1 << i;
		if (f->word & mask) pctr++;
	}

	if (pctr % 2 == f->data.parity) return 0;
	else return 1;
}
