#include "hal.h"
#include "ch.h"
#include "chprintf.h"

#define SHELL_NEWLINE_STR "\n"
static void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[]) {
  size_t n, total, largest;

  (void)argv;
  if (argc > 0) {
    shellUsage(chp, "mem");
    return;
  }
  n = chHeapStatus(NULL, &total, &largest);
  chprintf(chp, "core free memory : %u bytes" SHELL_NEWLINE_STR, chCoreGetStatusX());
  chprintf(chp, "heap fragments   : %u" SHELL_NEWLINE_STR, n);
  chprintf(chp, "heap free total  : %u bytes" SHELL_NEWLINE_STR, total);
  chprintf(chp, "heap free largest: %u bytes" SHELL_NEWLINE_STR, largest);
}

void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[]) {
  static const char *states[] = {CH_STATE_NAMES};
  thread_t *tp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: threads\r\n");
    return;
  }
  chprintf(chp, "    addr    stack prio refs     state\r\n");
  tp = chRegFirstThread();
  do {
    chprintf(chp, "%08lx %08lx %4lu %4lu %9s\r\n",
             (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
             (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
             states[tp->p_state]);
    tp = chRegNextThread(tp);
  } while (tp != NULL);
}
