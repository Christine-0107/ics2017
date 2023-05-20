#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
PCB *current = NULL;

uintptr_t loader(_Protect *as, const char *filename);

void load_prog(const char *filename) {
  int i = nr_proc ++;
  printf("protect begin.\n");
  _protect(&pcb[i].as);
  printf("protect end.\n");

  printf("loader begin.\n");
  uintptr_t entry = loader(&pcb[i].as, filename);
  printf("loader end.\n");

  // TODO: remove the following three lines after you have implemented _umake()
  printf("switch begin.\n");
  _switch(&pcb[i].as);
  printf("switch end.\n");
  current = &pcb[i];
  printf("entry begin.\n");
  ((void (*)(void))entry)();
  printf("entry end.\n");

  _Area stack;
  stack.start = pcb[i].stack;
  stack.end = stack.start + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, stack, stack, (void *)entry, NULL, NULL);
}

_RegSet* schedule(_RegSet *prev) {
  return NULL;
}
