#ifndef __PROC_H__
#define __PROC_H__

#include "common.h"
#include "memory.h"

#define STACK_SIZE (8 * PGSIZE)

//进程控制块PCB结构体
typedef union {
  uint8_t stack[STACK_SIZE] PG_ALIGN;
  struct {
    _RegSet *tf; //记录陷阱帧的位置
    _Protect as; //虚拟地址空间
    uintptr_t cur_brk; //当前堆区位置
    // we do not free memory, so use `max_brk' to determine when to call _map()
    uintptr_t max_brk;
  };
} PCB;

extern PCB *current;

#endif
