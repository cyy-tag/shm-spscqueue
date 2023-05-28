#pragma once
#include <cstddef>
#include <stdio.h>
#include <cstdint>

#define CACHELINE_SIZE 64
typedef char cacheline_pad_t [CACHELINE_SIZE];
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#define __WRITE_BARRIER__ \
    __asm__ __volatile__("sfence":::"memory")

#define err_msg(format,...) \
    fprintf(stderr, "[%s:%d]:" format "\n",__FILE__, __LINE__, ##__VA_ARGS__)

enum struct SPSCErr
{
  OK = 0,
  PARAM_ERR = -1,
  NO_SPACE = -2,
  NO_DATA = -3,
  DATA_ERR = -4,
  RECV_BUFFER_TOO_SMALL = -5,
};

class SPSCQueue
{

  struct alignas(CACHELINE_SIZE) RingHeader {
  int read;
  cacheline_pad_t pad;
  int write;
  };

public:
  explicit SPSCQueue(uint8_t * addr, size_t size);
  ~SPSCQueue();
  static SPSCQueue* CreateSPSCQueue(const char* nome, size_t size);
  static size_t PowerCeil(size_t x) {
      if (x <= 1) return 1;
      int power = 2;
      x--;
      while (x >>= 1) power <<= 1;
      return power;
  }

  int Send(const uint8_t* data, size_t size);
  int Recv(uint8_t* data, size_t size);
  inline size_t  Size(){ return (size_ + ring_h_->write - ring_h_->read)&(size_-1); }

private:
  inline int FreeSize()
  {
    return (size_ - ring_h_->write + ring_h_->read - extra_byte_) & (size_ - 1);
  }

private:
  uint8_t* shm_addr_;
  int shm_size_;
  size_t size_;
  // mmap ring buffer
  RingHeader *ring_h_;
  uint8_t * queue_addr_;

  static const size_t head_len_{sizeof(size_t)};
  static const int extra_byte_{8};
};
