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

#define hash_map(dev, blockno) ((((dev)<<27)|(blockno))%NBUCKET)

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
} bcache;

struct {
  struct spinlock lock;
  struct buf head;
}hash_bucket[NBUCKET];

// extern int ticks;

void
binit(void)
{
  // struct buf *b;

  initlock(&bcache.lock, "bcache");
  
  for(int i = 0; i < NBUCKET; i++){
    initlock(&hash_bucket[i].lock, "bcache.bucket");
    hash_bucket[i].head.next = 0;
  }

  for(int i = 0; i < NBUF; i++){
    // int index = i % NBUCKET;
    // struct buf cur = bcache.buf[i];
    // 这里是错误的根源，这里只是获得了一个复制，而实际上
    // buf[i]的指向并没有改变，这里应该用指针
    struct buf * cur = &bcache.buf[i];
    cur -> ticks = 0;
    cur -> refcnt = 0;
    initsleeplock(&cur -> lock, "buffer");
    cur -> next = hash_bucket[0].head.next;
    hash_bucket[0].head.next = cur;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buf *res = 0;
  
  uint index = hash_map(dev, blockno);

  acquire(&hash_bucket[index].lock);
  for(b = hash_bucket[index].head.next; b != 0; b = b->next){
    if(b -> dev == dev && b -> blockno == blockno){
      b->refcnt++;
      // b->ticks = ++block_ticks;
      release(&hash_bucket[index].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  
  //先释放锁，否则会导致很多地方阻塞，影响效率
  release(&hash_bucket[index].lock);
  acquire(&bcache.lock);
  
  //still have empty block
  struct buf * b_last = 0;
  struct buf * res_last = 0;
  int min_ticks = -1;
  uint cur_index = -1;
  for(int i = 0; i < NBUCKET; i++){
    // if(i != index)
    //   acquire(&hash_bucket[i].lock);
    acquire(&hash_bucket[i].lock);
    b_last = &hash_bucket[i].head;
    for(b = hash_bucket[i].head.next; b != 0; b = b -> next, b_last = b_last -> next){
      //这里也是需要判断的，因为前面释放了锁，所以可能会导致此时数据已经更新过了，所以要再判断一下
      if(b -> dev == dev && b -> blockno == blockno && hash_map(dev, blockno) == i){
        b->refcnt++;
        // b->ticks = ++block_ticks;
        release(&hash_bucket[i].lock);
        release(&bcache.lock);
        acquiresleep(&b->lock);
        return b;
      }

      if(b -> refcnt == 0){
        if(min_ticks == -1 || b -> ticks < min_ticks){
          min_ticks = b -> ticks;
          res = b;
          res_last = b_last;
          if(i != cur_index && cur_index != -1){
            release(&hash_bucket[cur_index].lock);
          }
          cur_index = i;
        }
      }
    }
    if(cur_index != i){
      release(&hash_bucket[i].lock);
    }
  }

  if(res){
    res_last -> next = res -> next;
    if(index != cur_index){
      release(&hash_bucket[cur_index].lock);
      acquire(&hash_bucket[index].lock);
    }
    res->dev = dev;
    res->blockno = blockno;
    res->valid = 0;
    res->refcnt = 1;
    res -> next = hash_bucket[index].head.next;
    hash_bucket[index].head.next = res;
    release(&hash_bucket[index].lock);
    release(&bcache.lock);
    acquiresleep(&res->lock);
    return res;
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
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint index = hash_map(b->dev, b->blockno);
  acquire(&hash_bucket[index].lock);
  b->refcnt--;
  if(b->refcnt == 0){
    b->ticks = ticks;
  }
  release(&hash_bucket[index].lock);
}

void
bpin(struct buf *b) {
  uint index = hash_map(b->dev, b->blockno);
  acquire(&hash_bucket[index].lock);
  b->refcnt++;
  release(&hash_bucket[index].lock);
}

void
bunpin(struct buf *b) {
  uint index = hash_map(b->dev, b->blockno);
  acquire(&hash_bucket[index].lock);
  b->refcnt--;
  release(&hash_bucket[index].lock);
}


