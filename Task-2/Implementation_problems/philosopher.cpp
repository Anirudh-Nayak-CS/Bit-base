#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <memory>
#include <string>
#include <chrono>
#include <utility>


#define N 5

std::mutex chopsticks[N];
std::mutex iom;

void philosopher(int index)
{
  int left = index;
  int right = (index + 1) % N;
   if(index==4) {
      std::swap(left,right);
    }

  while (true)
  {  
    iom.lock();
    std::cout << "Philosopher " << index+1 << " is thinking.\n";
    iom.unlock(); 
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::lock_guard<std::mutex> lock(chopsticks[left]);
    std::lock_guard<std::mutex> lock2(chopsticks[right]);
    iom.lock();
    std::cout << "Philosopher " << index+1 << " is eating.\n";
    iom.unlock();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    iom.lock();
    std::cout << "Philosopher " << index+1 << " finished  thinking and eating and has put down the chopsticks.\n";
    iom.unlock(); 
  }
}


int main()
{
  std::vector<std::thread> philosophers(N);
  for (int i = 0; i < N; i++)
  {
    philosophers[i] = std::thread(philosopher, i);
  }
  for (auto &phil : philosophers)
  {
    if (phil.joinable())
      phil.join();
  }
}
