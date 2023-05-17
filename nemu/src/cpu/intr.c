#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  //TODO();
  rtl_push(&cpu.eflags);
  rtl_push(&cpu.cs);
  rtl_push(&ret_addr);
  assert(NO*sizeof(GateDesc)<=cpu.idtr.limit);
  vaddr_t gate_addr=cpu.idtr.base+NO*sizeof(GateDesc);
  uint32_t off_15_0=vaddr_read(gate_addr,2);
  uint32_t off_32_16=vaddr_read(gate_addr+sizeof(GateDesc)-2,2);
  decoding.jmp_eip=(off_32_16<<16)|(off_15_0 & 0xffff);
  decoding.is_jmp=1;
}

void dev_raise_intr() {
}
