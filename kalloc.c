// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

#define NFRAMES PHYSTOP/PGSIZE

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
  int numFreePages;
} kmem;


struct{ 
  uint rcount[NFRAMES];
  struct spinlock lock;
} rtable;


void
rinit(void)
{
  initlock(&rtable.lock, "rtable");
  acquire(&rtable.lock);
  int i;
  for(i = 0; i< NFRAMES ; i++){
    rtable.rcount[i] = 0;
  }
  // cprintf("\ncpu%d: rinit called\n\n", cpunum());
  release(&rtable.lock);
}

void increment(char *va){
  acquire(&rtable.lock);
  uint pa = V2P(va)>>PGSHIFT;
  rtable.rcount[pa]++;
  release(&rtable.lock);
}
void decrement(char *va){
  acquire(&rtable.lock);
  uint pa = V2P(va)>>PGSHIFT;
  rtable.rcount[pa]--;
  release(&rtable.lock);
}


int checkZero(char *va){
  acquire(&rtable.lock);
  uint pa = V2P(va)>>PGSHIFT;
  uint temp = rtable.rcount[pa];
  release(&rtable.lock);
  return temp;
}

void setOne(char *va){
  uint pa = V2P(va)>>PGSHIFT;
  rtable.rcount[pa] = 1 ;
}

void setZero(char* va){
  uint pa = V2P(va)>>PGSHIFT;
  rtable.rcount[pa] = 0 ;
}

int getNumFreePages(){
  if(kmem.use_lock){
    acquire(&kmem.lock);
  }
  int temp = kmem.numFreePages;

  if(kmem.use_lock){
    release(&kmem.lock);
  }
  return temp;
}
// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  cprintf("\ncpu%d: kinit1 called\n\n", cpunum());
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  kmem.numFreePages = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}

//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  if(kmem.use_lock){
    acquire(&kmem.lock);
    acquire(&rtable.lock);
  }
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.numFreePages++;
  setZero((char*)r);
  if(kmem.use_lock){
    release(&kmem.lock);
    release(&rtable.lock);
  }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock){
    acquire(&kmem.lock);
    acquire(&rtable.lock);
  }
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    kmem.numFreePages--;
    setOne((char*)r);
  }

  if(kmem.use_lock){
    release(&kmem.lock);
    release(&rtable.lock);
  }

  return (char*)r;
}

