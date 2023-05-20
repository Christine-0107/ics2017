#include "common.h"
#include "fs.h"

//#define DEFAULT_ENTRY ((void *)0x4000000)
#define DEFAULT_ENTRY ((void *)0x8048000)

extern size_t get_ramdisk_size();
extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern void* new_page(void);

uintptr_t loader(_Protect *as, const char *filename) {
  //TODO();
  //size_t ramdisk_size = get_ramdisk_size();
  //ramdisk_read((void *)DEFAULT_ENTRY, 0, ramdisk_size);
  int fd = fs_open(filename, 0, 0);
  int f_size = fs_filesz(fd);

  //Log("Load %d bytes file, named %s, fd %d", f_size, filename, fd);
  void* pa = DEFAULT_ENTRY;
  void* va = DEFAULT_ENTRY;
  while(f_size > 0){
    pa = new_page();
    _map(as, va, pa);
    fs_read(fd, pa, PGSIZE);

    va += PGSIZE;
    f_size -= PGSIZE;
    // Log("f_size remaining:%d..",f_size);
  }
  fs_close(fd);
  return (uintptr_t)DEFAULT_ENTRY;
}
