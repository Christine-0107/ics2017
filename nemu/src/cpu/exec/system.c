#include "cpu/exec.h"

void diff_test_skip_qemu();
void diff_test_skip_nemu();

make_EHelper(lidt) {
  //TODO();
  /*t1=id_dest->val; //data数组的地址
  cpu.idtr.limit = vaddr_read(t1, 2); // 读取data[0]为idtr.limit
  cpu.idtr.base = vaddr_read(t1 + 2, 4); // 读取data[1]、data[2]为idtr.base, 32bit
  print_asm_template1(lidt);*/
  cpu.idtr.limit=vaddr_read(id_dest->addr,2); //limit读取16位
  if(decoding.is_operand_size_16){
    cpu.idtr.base=vaddr_read(id_dest->addr+2,3); //base24
  }
  else{
    cpu.idtr.base=vaddr_read(id_dest->addr+2,4); //base32
  }
  print_asm_template1(lidt);
}

make_EHelper(mov_r2cr) {
  //TODO();
  switch(id_dest->reg){
  case 0:
    cpu.cr0.val = id_src->val;
    break;
  case 3:
    cpu.cr3.val = id_src->val;
    break;
  }
  print_asm("movl %%%s,%%cr%d", reg_name(id_src->reg, 4), id_dest->reg);
}

make_EHelper(mov_cr2r) {
  //TODO();
  switch(id_src->reg){
  case 0:
    operand_write(id_dest, &cpu.cr0.val);
    break;
  case 3:
    operand_write(id_dest, &cpu.cr3.val);
    break;
  }
  print_asm("movl %%cr%d,%%%s", id_src->reg, reg_name(id_dest->reg, 4));

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(int) {
  //TODO();
  uint8_t NO=id_dest->val&0xff;
  raise_intr(NO,decoding.seq_eip);

  print_asm("int %s", id_dest->str);

#ifdef DIFF_TEST
  diff_test_skip_nemu();
#endif
}

make_EHelper(iret) {
  //TODO();
  rtl_pop(&decoding.jmp_eip);
  decoding.is_jmp = 1;
  rtl_pop(&cpu.cs);
  rtl_pop(&cpu.eflags);
  print_asm("iret");
}

uint32_t pio_read(ioaddr_t, int);
void pio_write(ioaddr_t, int, uint32_t);

make_EHelper(in) {
  t1 = pio_read(id_src->val,id_dest->width);
  operand_write(id_dest,&t1);

  print_asm_template2(in);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(out) {
  pio_write(id_dest->val,id_dest->width,id_src->val);

  print_asm_template2(out);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}
