#include <x86.h>
#include <assert.h>
#include <stdio.h>

#define PG_ALIGN __attribute((aligned(PGSIZE)))

static PDE kpdirs[NR_PDE] PG_ALIGN;
static PTE kptabs[PMEM_SIZE / PGSIZE] PG_ALIGN;
static void* (*palloc_f)();
static void (*pfree_f)(void*);

_Area segments[] = {      // Kernel memory mappings
  {.start = (void*)0,          .end = (void*)PMEM_SIZE}
};

#define NR_KSEG_MAP (sizeof(segments) / sizeof(segments[0]))

//准备内核页表
void _pte_init(void* (*palloc)(), void (*pfree)(void*)) {
  palloc_f = palloc;
  pfree_f = pfree;

  int i;

  // make all PDEs invalid
  for (i = 0; i < NR_PDE; i ++) {
    kpdirs[i] = 0;
  }

  PTE *ptab = kptabs;
  for (i = 0; i < NR_KSEG_MAP; i ++) {
    uint32_t pdir_idx = (uintptr_t)segments[i].start / (PGSIZE * NR_PTE);
    uint32_t pdir_idx_end = (uintptr_t)segments[i].end / (PGSIZE * NR_PTE);
    for (; pdir_idx < pdir_idx_end; pdir_idx ++) {
      // fill PDE
      kpdirs[pdir_idx] = (uintptr_t)ptab | PTE_P;

      // fill PTE
      PTE pte = PGADDR(pdir_idx, 0, 0) | PTE_P;
      PTE pte_end = PGADDR(pdir_idx + 1, 0, 0) | PTE_P;
      for (; pte < pte_end; pte += PGSIZE) {
        *ptab = pte;
        ptab ++;
      }
    }
  }

  set_cr3(kpdirs); //设置CR3寄存器存储页目录表基址
  set_cr0(get_cr0() | CR0_PG); //设置CR0寄存器开启分页机制
}

//创建一个进程的虚拟地址空间
void _protect(_Protect *p) {
  PDE *updir = (PDE*)(palloc_f());
  p->ptr = updir; //p->ptr可以获取页目录表的基地址
  // map kernel space
  for (int i = 0; i < NR_PDE; i ++) {
    updir[i] = kpdirs[i];
  }

  p->area.start = (void*)0x8000000;
  p->area.end = (void*)0xc0000000;
}

void _release(_Protect *p) {
}

void _switch(_Protect *p) {
  set_cr3(p->ptr);
}

//提供映射一页的功能，将虚拟地址空间p中的虚拟地址va映射到物理地址pa
void _map(_Protect *p, void *va, void *pa) {
  //首先需要判断虚拟地址中的偏移值和物理地址中的偏移值是否相同
  if(OFF(va) || OFF(pa)){
    printf("Error: va.off!=pa.off\n");
    return;
  }
  PDE* pdbase=(PDE*)p->ptr; //页目录表的基地址
  PTE* ptbase=NULL; //指向页表表项
  PDE* pde=pdbase+PDX(va); //指向页目录表项
  if(!(*pde&PTE_P)){ //若页表项不存在
    ptbase=(PTE*)(palloc_f()); //分配一项
    *pde=(uintptr_t)ptbase|PTE_P;
  }
  ptbase=(PTE*)PTE_ADDR(*pde);
  PTE* pte=ptbase+PTX(va);
  *pte=(uintptr_t)pa|PTE_P;
}

void _unmap(_Protect *p, void *va) {
}

extern void* memcpy(void*,const void*,int);
//用来创建用户进程现场
_RegSet *_umake(_Protect *p, _Area ustack, _Area kstack, void *entry, char *const argv[], char *const envp[]) {
  //return NULL;
  int arg1=0;
  char* arg2=NULL;
  memcpy((void*)ustack.end-4, (void*)arg2, 4);
  memcpy((void*)ustack.end-8, (void*)arg2, 4);
  memcpy((void*)ustack.end-12, (void*)arg1, 4);
  memcpy((void*)ustack.end-16, (void*)arg1, 4);

  _RegSet tf;
  tf.eflags=0x02;
  tf.cs=8;
  tf.eip=(uintptr_t)entry;
  void* ptf=(void*)(ustack.end-16-sizeof(_RegSet));
  memcpy(ptf,(void*)&tf,sizeof(_RegSet));

  return (_RegSet*)ptf;
}
