#include <iostream>
#include <thread>
#include <queue>
#include <memory>
#include <string>
#include <chrono>
#include <utility>
#include <semaphore>
#include <mutex>

#define BUFFER_SIZE 5

std::queue<int> buffer;
std::mutex iom;
std::counting_semaphore<BUFFER_SIZE> buff_full(0);
std::counting_semaphore<BUFFER_SIZE> buff_empty(BUFFER_SIZE);
std::mutex buffer_change;

void producer(int id)
{
  while (true)
  {
    buff_empty.acquire();
    iom.lock();
    std::cout << "Producer " << id << " is entering.\n";
    iom.unlock();
    buffer_change.lock();
    buffer.push(rand() % 10);
    buffer_change.unlock();
    iom.lock();
    std::cout << "Producer " << id << " produced an item.\n";
    iom.unlock();

    std::this_thread::sleep_for(std::chrono::seconds(2));
    buff_full.release();
    iom.lock(); 
    std::cout << "Producer " << id << " is leaving.\n";
    iom.unlock();
  }
}
void consumer(int id)
{
  while (true)
  {
    buff_full.acquire();
    iom.lock(); 
    std::cout << "Consumer " << id << " is entering.\n";
    iom.unlock(); 
    buffer_change.lock();
    buffer.pop();

    buffer_change.unlock();
    iom.lock();
    std::cout << "Consumer " << id << " consumed an item.\n";
    iom.unlock(); 

    std::this_thread::sleep_for(std::chrono::seconds(2));
    buff_empty.release();
    iom.lock(); 
    std::cout << "Consumer " << id << " is leaving.\n";
    iom.unlock();
  }
}

int main()
{
  std::thread prod = std::thread(producer, 1);
  std::thread cons = std::thread(consumer, 1);

  if (prod.joinable())
    prod.join();
  if (cons.joinable())
    cons.join();
}
