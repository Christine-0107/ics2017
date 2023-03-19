#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
// Return a free WP struct
WP* new_wp(char *exp) {
  assert(free_!=NULL);
  WP *result = free_;
  free_ = free_->next;
  result->next = NULL;
  strcpy(result->exp,exp);
  bool success = false;
  uint32_t ret = expr(exp, &success);
  if(success==false){
    printf("Error: In evacuation .\n");
    assert(0);
  }
  else{
    printf("The unsigned value of the watchpoint expr is %u .\n", ret);
  }
  result->value=ret;
  if(head==NULL){
    head = result;
  }
  else{
    WP *p = head;
    while(p->next!=NULL){
      p=p->next;
    }
    p->next = result;
  }
  return result;
}

