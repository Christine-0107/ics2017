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
  if(current->cur_brk==0){
    current->cur_brk=current->max_brk=new_brk;
  }
  else{
    if(new_brk>current->max_brk){
      //TODO: map memory region [current->max_brk,new_brk) into address space current->as
      uint32_t first=PGROUNDUP(current->max_brk);
      uint32_t end=PGROUNDDOWN(new_brk);
      if((new_brk&0xfff)==0){
        end-=PGSIZE;
      }
      for(uint32_t va=first;va<=end;va+=PGSIZE){
        void* pa=new_page();
        _map(&(current->as),(void*)va,pa);
      }
      current->max_brk=new_brk;
    }
    current->max_brk=new_brk;
  }
  return 0;
}

//Nanos-lite在初始化时先调用init_mm()函数初始化MM（存储管理器）
void init_mm() {
  pf = (void *)PGROUNDUP((uintptr_t)_heap.start); //将堆区起始地址作为空闲页的首地址
  Log("free physical pages starting from %p", pf);

  _pte_init(new_page, free_page); //准备内核页目录表和页表
}
