#include "proc.h"
#include "memory.h"

static void *pf = NULL;

//分配空闲的物理页
void* new_page(void) {
  assert(pf < (void *)_heap.end);
  void *p = pf;
  pf += PGSIZE;
  return p;
}

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uint32_t new_brk) {
  return 0;
}

//Nanos-lite在初始化时先调用init_mm()函数初始化MM（存储管理器）
void init_mm() {
  pf = (void *)PGROUNDUP((uintptr_t)_heap.start); //将堆区起始地址作为空闲页的首地址
  Log("free physical pages starting from %p", pf);

  _pte_init(new_page, free_page); //准备内核页目录表和页表
}
