#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
PCB *current = NULL; //记录当前正在运行哪一个进程，指向当前进程的PCB

uintptr_t loader(_Protect *as, const char *filename);

void load_prog(const char *filename) {
  int i = nr_proc ++;
  _protect(&pcb[i].as);

  uintptr_t entry = loader(&pcb[i].as, filename);
  //uintptr_t entry=0x8048000;

  // TODO: remove the following three lines after you have implemented _umake()
  //_switch(&pcb[i].as);
  //current = &pcb[i];
  //((void (*)(void))entry)();

  _Area stack;
  stack.start = pcb[i].stack;
  stack.end = stack.start + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, stack, stack, (void *)entry, NULL, NULL);
}

//完成进程调度
_RegSet* schedule(_RegSet *prev) {
  //return NULL;
  if(current!=NULL)
    current->tf=prev; //保存上下文指针
  //current=&pcb[0];
  //current = (current == &pcb[0] ? &pcb[1] : &pcb[0]); //轮流调度两个进程
  //设置频率
  static int num=0;
  static const int freq=10000;
  if(current==&pcb[0]){
    num++;
  }
  else{
    current=&pcb[0];
  }
  if(num==freq){
    current=&pcb[1];
    num=0;
  }
  Log("PTR=0x%x\n",(uint32_t)current->as.ptr);
  _switch(&current->as);
  return current->tf;
}
