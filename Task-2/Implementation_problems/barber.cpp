#include <iostream>
#include <thread>
#include <queue>
#include <memory>
#include <string>
#include <chrono>
#include <utility>
#include <semaphore>
#include <mutex>

#define N 5

std::mutex ctupdate;
std::counting_semaphore<N> customer(0);
std::binary_semaphore barber(0);
int customerct = 0;
std::mutex iom;


void barberfunc(int id)
{
  while (true)
  {
    iom.lock();
    std::cout << "Barber " << id + 1 << " is free.\n";
    iom.unlock(); 
    customer.acquire();
    ctupdate.lock();
    customerct--;
    ctupdate.unlock();

    barber.release();
    iom.lock();
    std::cout << "Barber " << id + 1 << " is cutting customer's hair.\n";
    iom.unlock(); 
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
}

void customerfunc(int id)
{
  while (true)
  {
    iom.lock(); 
    std::cout << "Customer " << id + 1 << " has entered.\n";
    iom.unlock(); 

    ctupdate.lock();
    if (customerct == N)
    {
      ctupdate.unlock();
      continue;
    }
    customerct++;
    ctupdate.unlock();
    customer.release();
    barber.acquire();
    iom.lock();
    std::cout << "Customer " << id + 1 << " is getting a haircut.\n";
    iom.unlock(); 

  }
}

int main()
{

  std::vector<std::thread> customers(N);
  auto barber = std::thread(barberfunc, 1);
  for (int i = 0; i < N; i++)
    customers[i] = std::thread(customerfunc, i);

  if (barber.joinable())
    barber.join();
  for (auto &th : customers)
  {
    if (th.joinable())
      th.join();
  }
}
