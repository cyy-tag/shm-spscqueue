#include "shm_spsc.h"
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

SPSCQueue* SPSCQueue::CreateSPSCQueue(const char* name, size_t size)
{
  int fd = shm_open(name, O_RDWR, 0660);
  if(fd < 0)
  {
    if(errno == ENOENT) {
      fd = shm_open(name, O_CREAT|O_RDWR, 0660);
      if(fd < 0) {
        err_msg("shm_open file %s err %s", name, strerror(errno));
        exit(-1);
      }
    }
    else {
      err_msg("shm_open file %s err %s", name, strerror(errno));
      exit(-1);
    }
  }
  // round to 2^n
  size = PowerCeil(size);
  int file_size = size + sizeof(RingHeader);
  if(ftruncate(fd, file_size) < 0 )
  {
    err_msg("ftruncate file %s err %s", name, strerror(errno));
    exit(-1);
  }
  uint8_t * addr = (uint8_t*)mmap(0, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if(addr == MAP_FAILED)
  {
    err_msg("mmap failed err %s", strerror(errno));
    exit(-1);
  }
  if(close(fd) < 0 )
  {
    err_msg("close file err %s", strerror(errno));
    exit(-1);
  }
  SPSCQueue * ret = new SPSCQueue(addr, file_size);
  return ret;
}

SPSCQueue::SPSCQueue(uint8_t * addr, size_t size)
{
  shm_addr_ = addr;
  shm_size_ = size;

  size_ = size - sizeof(RingHeader);
  ring_h_ =  reinterpret_cast<RingHeader*>(addr);
  queue_addr_ = addr + sizeof(RingHeader);
}

SPSCQueue::~SPSCQueue()
{
  if(munmap(shm_addr_, shm_size_) < 0) {
    err_msg("munmap failed %s", strerror(errno));
  }
}

int SPSCQueue::Send(const uint8_t* data, size_t size)
{
  if(unlikely(!data || size <= 0)){
    return static_cast<int>(SPSCErr::PARAM_ERR);
  }

  int free_size = FreeSize();
  if(unlikely((size + head_len_) > free_size)){
    return static_cast<int>(SPSCErr::NO_SPACE);
  }

  int cur_write = ring_h_->write;
  uint8_t* src = reinterpret_cast<uint8_t*>(&size);
  for(int i = 0; i < head_len_; ++i)
  {
    queue_addr_[cur_write] = src[i] ;
    cur_write = (cur_write + 1) & (size_ - 1);
  }

  size_t tmp_len = size > size_ - cur_write ? size_ - cur_write : size;
  memcpy(&queue_addr_[cur_write], data, tmp_len);
  size_t remain_len = size - tmp_len;

  if(unlikely(remain_len > 0))
  {
    memcpy(&queue_addr_[0], data + tmp_len, remain_len);
  }
  __WRITE_BARRIER__;
  ring_h_->write = (cur_write + size) & (size_ - 1);
  return static_cast<int>(SPSCErr::OK);
}

int SPSCQueue::Recv(uint8_t* data, size_t size)
{
  if(unlikely(!data || size < head_len_)) {
    return static_cast<int>(SPSCErr::PARAM_ERR);
  }

  size_t total_size = Size();
  if(unlikely(total_size == 0)) {
    return static_cast<int>(SPSCErr::NO_DATA);
  }

  if(unlikely(total_size < head_len_)) {
    err_msg("data len illegal total_size: %d", total_size);
    exit(-1);
  }

  size_t data_size = 0;
  uint8_t* dst = reinterpret_cast<uint8_t*>(&data_size);
  size_t cur_read = ring_h_->read;
  for(int i = 0; i < head_len_; ++i) {
    dst[i] = queue_addr_[cur_read];
    cur_read = (cur_read + 1) & (size_ - 1);
  }
  if(unlikely(total_size < data_size)) {
    err_msg("data len illegal total_size: %u data_size: %u\n",
            total_size,data_size);
    exit(-1);
  }

  if(unlikely(data_size > size)) {
    err_msg("data buffer size is small data size: %u continue cur data\n", 
            data_size);
    ring_h_->read = (cur_read + data_size) & (size_ - 1);
    return static_cast<int>(SPSCErr::RECV_BUFFER_TOO_SMALL);
  }
  dst = data;
  size_t min_size = data_size > size_ - cur_read ? size_ - cur_read : data_size;
  memcpy(data, &queue_addr_[cur_read], min_size);
  size_t remain_size = data_size - min_size;
  if(unlikely(remain_size > 0)) {
    memcpy(&data[min_size], &queue_addr_[0], remain_size);
  }

  __WRITE_BARRIER__;
  ring_h_->read = (cur_read + data_size) & (size_ - 1);
  return static_cast<int>(SPSCErr::OK);
}
