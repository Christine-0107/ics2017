#include "nemu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

//添加帮助地址转换的宏定义
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/\------ OFF(va) ------/
#define PDX(va)     (((uint32_t)(va) >> 22) & 0x3ff)
#define PTX(va)     (((uint32_t)(va) >> 12) & 0x3ff)
#define OFF(va)     ((uint32_t)(va) & 0xfff)
// Address in page table or page directory entry, 取高20位
#define PTE_ADDR(pte)   ((uint32_t)(pte) & ~0xfff)



uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  int flag = is_mmio(addr);
  if(flag==-1){
    return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
  }
  else{
    return mmio_read(addr,len,flag);
  }
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int flag = is_mmio(addr);
  if(flag==-1){
    memcpy(guest_to_host(addr), &data, len);
  }
  else{
    mmio_write(addr,len,data,flag);
  }
}

paddr_t page_translate(vaddr_t vaddr, bool flag) {
  PDE pde;  //页目录表项
  PDE *pdbase; //指向页目录表的基址
  PTE pte; //页表表项
  PTE *ptbase; //指向页表的基址
  //当CR0的标志位标识保护模式和分页模式启动，才能进行操作
  //cpu.cr0.protect_enable && cpu.cr0.paging
  if(cpu.cr0.protect_enable && cpu.cr0.paging) {
    printf("abc\n");
    pdbase = (PDE*)(PTE_ADDR(cpu.cr3.val)); //找到页目录表基址
    pde.val = paddr_read((paddr_t)&pdbase[PDX(vaddr)], 4); //PDX()找到页目录表偏移
    assert(pde.present); //检查present标志位
    pde.accessed = true; //设置访问标志位
    ptbase = (PTE*)(PTE_ADDR(pde.val)); //找到页表的基址
    pte.val = paddr_read((paddr_t)&ptbase[PTX(vaddr)], 4); //PTX()找到页表偏移
    assert(pte.present); //检查present标志位
    pte.accessed = true;
    pte.dirty = flag ? 1 : pte.dirty; //设置写
    return PTE_ADDR(pte.val) | OFF(vaddr);
  }
  return vaddr;
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  if(PTE_ADDR(addr)!=PTE_ADDR(addr+len-1)){ //数据跨越了虚拟页边界
    //printf("Error: the data passes tow pages\n");
    //assert(0);
    uint32_t ret = 0;
    for(int i=0; i<len; i++){
      paddr_t paddr = page_translate(addr+i, false);
      ret += paddr_read(paddr, 1) << (8 * i);
    }
    return ret;
  }
  else{
    paddr_t paddr = page_translate(addr,false);
    return paddr_read(addr, len);
  }
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if(PTE_ADDR(addr)!=PTE_ADDR(addr+len-1)){ //数据跨越了虚拟页边界
    //printf("Error: the data passes tow pages\n");
    //assert(0);
    for(int i=0;i<len;i++){
      paddr_t paddr = page_translate(addr+i,true);
      paddr_write(paddr, 1, data>>(8*i));
    }
  }
  else{
    paddr_t paddr = page_translate(addr,true);
    return paddr_write(addr, len, data);
  }
}
