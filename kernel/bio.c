// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  struct spinlock bucketlock[BC_NBUCKET];
  // each bucket is a dual-linked-list of buf
  struct buf bucket[BC_NBUCKET];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  // init empty bucket
  for (int i = 0; i < BC_NBUCKET; i++) {
    initlock(&bcache.bucketlock[i], "bcache-bucket");
    // bucket link-list init as self-loop
    bcache.bucket[i].prev = &bcache.bucket[i];
    bcache.bucket[i].next = &bcache.bucket[i];
  }
  // init all buf in bcache.buf
  for(b = bcache.buf; b < bcache.buf+NBUF; b++) {
    // all buffer init in bucket[0]
    b->next = bcache.bucket[0].next;
    b->prev = &bcache.bucket[0];
    initsleeplock(&b->lock, "buffer");
    bcache.bucket[0].next->prev = b;
    bcache.bucket[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int i = BLOCK2BUCKET(blockno);

  // acquire(&bcache.lock);
  
  // Is the block already cached?
  // search through the bucket[i]
  // no need for global lock, just acquire bucket lock
  acquire(&bcache.bucketlock[i]);
  for (b = bcache.bucket[i].next; b != &bcache.bucket[i]; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.bucketlock[i]);
      // release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // first global lock, then bucket lock
  release(&bcache.bucketlock[i]);
  acquire(&bcache.lock);
  acquire(&bcache.bucketlock[i]);
  // Check again: Is the block already cached? 
  // since we just release bucket lock once
  for (b = bcache.bucket[i].next; b != &bcache.bucket[i]; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.bucketlock[i]);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Recycle the least recently used (LRU) unused buffer.
  // bucket loop
  for (int k = 0; k < BC_NBUCKET; k++) {
    struct buf *b_lru = 0;
    uint mintick = 0xffffffff;
    int j = (i + k) % BC_NBUCKET;
    if (i != j)
      acquire(&bcache.bucketlock[j]);
    // find LRU buffer in bucket[j]
    for (b = bcache.bucket[j].next; b != &bcache.bucket[j]; b = b->next) {
      if (b->refcnt == 0 && b->tick < mintick) {
        b_lru = b;
        mintick = b_lru->tick;
      }
    }
    if (b_lru) {
      if (i != j) {
        // different bucket
        // remove b_lru from the origin bucket
        b_lru->next->prev = b_lru->prev;
        b_lru->prev->next = b_lru->next;
        release(&bcache.bucketlock[j]);
        // add to current bucket
        b_lru->next = bcache.bucket[i].next;
        b_lru->prev = &bcache.bucket[i];
        bcache.bucket[i].next->prev = b_lru;
        bcache.bucket[i].next = b_lru;
      }
      b_lru->dev = dev;
      b_lru->blockno = blockno;
      b_lru->valid = 0;     // buffer invalid, wait for reading disk
      b_lru->refcnt = 1;
      release(&bcache.bucketlock[i]);
      release(&bcache.lock);
      acquiresleep(&b_lru->lock);
      return b_lru;
    }

    if (i != j)
      release(&bcache.bucketlock[j]);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  int i = BLOCK2BUCKET(b->blockno);
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.bucketlock[i]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // recycle the buf, record ticks of last used for LRU
    // no need for linked-list operation, since we use ticks for LRU now
    b->tick = ticks;
    // no one is waiting for it.
    // b->next->prev = b->prev;
    // b->prev->next = b->next;
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    // bcache.head.next->prev = b;
    // bcache.head.next = b;
  }
  
  release(&bcache.bucketlock[i]);
}

void
bpin(struct buf *b) {
  int i = BLOCK2BUCKET(b->blockno);
  acquire(&bcache.bucketlock[i]);
  b->refcnt++;
  release(&bcache.bucketlock[i]);
}

void
bunpin(struct buf *b) {
  int i = BLOCK2BUCKET(b->blockno);
  acquire(&bcache.bucketlock[i]);
  b->refcnt--;
  release(&bcache.bucketlock[i]);
}


