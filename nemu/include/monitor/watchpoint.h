#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char exp[1000];
  uint32_t value;

} WP;

WP* new_wp(char *exp);
bool free_wp(WP *wp);
WP* find_wp(int num);

bool test_watchpoint();
void print_wp();

#endif
