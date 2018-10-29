#ifndef CMD_H_INCLUDED
#define CMD_H_INCLUDED

void cmd_reset(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_flashwrite(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_flashread(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_flashdump(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_flashinfo(BaseSequentialStream *chp, int argc, char *argv[]);

#endif // CMD_H_INCLUDED
