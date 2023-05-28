#include <iostream>
#include <stdio.h>
#include <string>
#include <time.h>
#include "../shm_spsc.h"

const int msg_size = 1500;
#define SING_TEST_NUM 1000000

void read_func(SPSCQueue* queue)
{
  uint8_t data[msg_size];
  unsigned long long bytes = 0;
  time_t last_time = time(NULL);
  while(1){
    int ret = queue->Recv(data, msg_size);
    if(ret == (int)SPSCErr::OK)
    {
      if(last_time < time(NULL))
      {
        last_time = time(NULL);
        printf("read rate %.2f MB\n", bytes/(1024.0*1024.0));
        bytes = 0;
      }
      bytes += msg_size;
    }
    else if(ret == (int)SPSCErr::NO_DATA)
      continue;
  }
}

void write_func(SPSCQueue* queue)
{
  uint32_t write_i = 0;
  const std::string data(msg_size, 'a');
  unsigned long long bytes = 0;
  time_t last_time = time(NULL);
  while(1){
    int ret = queue->Send((uint8_t*)data.c_str(), data.length());
    if(ret == 0){
      if(last_time < time(NULL))
      {
        last_time = time(NULL);
        printf("write rate %.2f MB\n", bytes/(1024.0*1024.0));
        bytes = 0;
      }
      bytes += msg_size;
    } else if(ret == (int)(SPSCErr::NO_SPACE))
    {
      fflush(stdout);
      continue;
    }
    else {
      printf("error \n");
      exit(-1);
    }
  }
}

int main(int argc, char* argv[])
{
  if(argc < 2) {
    std::cout << "input read or send" << std::endl;
    return 0;
  }
  SPSCQueue* queue = SPSCQueue::CreateSPSCQueue("shm-queue", 1024*1024);
  std::string name(argv[1]);
  printf("queue addr %p\n", queue);
  if(name == "read") {
    read_func(queue);
  }else if(name == "send") {
    write_func(queue);
  }else {
    std::cout << "input para error" << std::endl;
  }
  return 0;
}
