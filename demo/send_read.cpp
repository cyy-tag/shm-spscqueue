#include <iostream>
#include <string>
#include "../shm_spsc.h"

const int msg_size = 1500;
#define SING_TEST_NUM 1000000

void read_func(SPSCQueue* queue)
{
  uint8_t data[msg_size];
  uint32_t read_index = 0;
  while(1){
    int ret = queue->Recv(data, msg_size);
    if(ret == (int)SPSCErr::NO_DATA)
      continue;
    int num = atoi((const char*)data);
    if(num == SING_TEST_NUM) {
      printf("success\n");
      break;
    }
    if(read_index != num)
    {
      printf("read_index != num, read: %u. num %u\n", read_index, num);
      exit(-1);
    }
    read_index++;
  }
}

void write_func(SPSCQueue* queue)
{
  uint32_t write_i = 0;
  while(1){
    if(write_i > SING_TEST_NUM) {
      break;
    }
    const std::string data = std::to_string(write_i);
    int ret = queue->Send((uint8_t*)data.c_str(), data.length());
    if(ret == 0){
      write_i++;
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
