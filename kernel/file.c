//
// Support functions for system calls that involve file descriptors.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "stat.h"
#include "proc.h"
#include "fcntl.h"

struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Decrement ref count for file f.
struct file*
fileundup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileundup");
  f->ref--;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if(ff.type == FD_PIPE){
    pipeclose(ff.pipe, ff.writable);
  } else if(ff.type == FD_INODE || ff.type == FD_DEVICE){
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filestat(struct file *f, uint64 addr)
{
  struct proc *p = myproc();
  struct stat st;
  
  if(f->type == FD_INODE || f->type == FD_DEVICE){
    ilock(f->ip);
    stati(f->ip, &st);
    iunlock(f->ip);
    if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
      return -1;
    return 0;
  }
  return -1;
}

// Read from file f.
// addr is a user virtual address.
int
fileread(struct file *f, uint64 addr, int n)
{
  int r = 0;

  if(f->readable == 0)
    return -1;

  if(f->type == FD_PIPE){
    r = piperead(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
      return -1;
    r = devsw[f->major].read(1, addr, n);
  } else if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, 1, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
  } else {
    panic("fileread");
  }

  return r;
}

// Write to file f.
// addr is a user virtual address.
int
filewrite(struct file *f, uint64 addr, int n)
{
  int r, ret = 0;

  if(f->writable == 0)
    return -1;

  if(f->type == FD_PIPE){
    ret = pipewrite(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
      return -1;
    ret = devsw[f->major].write(1, addr, n);
  } else if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op();
      ilock(f->ip);
      if ((r = writei(f->ip, 1, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      end_op();

      if(r != n1){
        // error from writei
        break;
      }
      i += r;
    }
    ret = (i == n ? n : -1);
  } else {
    panic("filewrite");
  }

  return ret;
}

uint64
munmap(uint64 addr, uint64 length)
{
  struct proc *p = myproc();
  printf("[%d]munmap start: addr=%p, len=%d\n", p->pid, addr, length);
  struct vma *v = p->vmalist;
  for(int i = 0; i < NVMA; i++) {
    // find the vma of addr
    if (addr >= v[i].addr && addr <= v[i].addr + v[i].length 
          && addr + length <= v[i].addr + v[i].length) {
      uint npage = NPAGE(length);
      struct file *f = v[i].f;

      if (v[i].flags == MAP_SHARED && v[i].f->writable) {
        begin_op();
        ilock(f->ip);        
        uint64 offset = addr - v[i].addr + v[i].offset; // file's offset

        for (int i = 0; i < npage; i++) {
          uint64 addr_t = addr + i * PGSIZE;
          uint64 offset_t = offset + i * PGSIZE;

          // Already mapped?
          if (walkaddr(p->pagetable, addr_t)) {
            pte_t *pte = walk(p->pagetable, addr_t, 0);
            uint64 flags = PTE_FLAGS(*pte);
            // write back Dirty Pages only
            if (flags & PTE_D)
              writei(f->ip, 1, addr_t, offset_t, PGSIZE);
          }
        }

        iunlock(f->ip);
        end_op();
      }

      v[i].npage -= npage;
      // no mmap pages, free the vma
      if (v[i].npage < 0) {
        fileundup(f);
        v[i].addr = 0;
        v[i].length = 0;
        v[i].prot = 0;
        v[i].flags = 0;
        v[i].f = 0;
        v[i].offset = 0;
        v[i].npage = 0;
      }

      // unmap vm pages
      for (uint64 addr_t = v[i].addr; addr_t < v[i].addr + v[i].length; addr_t += PGSIZE) {
        if (walkaddr(p->pagetable, addr_t)) {
          printf("[%d]munmap: uvmunmap(), addr=%p\n", p->pid, addr_t);
          uvmunmap(p->pagetable, addr_t, 1, 1);
        }
      }
      return 0;
    }
  }
  return (uint64)-1;
}