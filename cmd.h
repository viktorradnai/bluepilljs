#ifndef CMD_H_INCLUDED
#define CMD_H_INCLUDED

#include "shell.h"
#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

void cmd_reset(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_restart(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_calibrate(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_cal_load(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_cal_save(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_calprint(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_calread(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_flashwrite(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_flashread(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_flashinfo(BaseSequentialStream *chp, int argc, char *argv[]);

const ShellCommand commands[] = {
  {"cal", cmd_calibrate},
  {"cal_print", cmd_calprint},
  {"cal_save", cmd_cal_save},
  {"cal_load", cmd_cal_load},
  {"cal_read", cmd_calread},
  {"flashwrite", cmd_flashwrite},
  {"flashread", cmd_flashread},
  {"flashinfo", cmd_flashinfo},
  {"reset", cmd_reset},
  {"restart", cmd_restart},
  {NULL, NULL}
};

#endif // CMD_H_INCLUDED
