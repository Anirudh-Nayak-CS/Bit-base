#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <memory>
#include <string>
#include <chrono>
#include <utility>

#define READERS 5
#define WRITERS 3

int readct = 0;
std::mutex wrt;
std::mutex updatect;
std::mutex iom;

void reader(int id)
{
  while (true)
  {
    iom.lock();
    std::cout << "Reader " << id << " is entering.\n";
    iom.unlock();
    {
      std::lock_guard<std::mutex> lock2(updatect);
      readct++;
      if (readct == 1)
      {
        wrt.lock();
      }
    }
    iom.lock();
    std::cout << "Reader " << id << " is reading.\n";
    iom.unlock();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    {
      std::lock_guard<std::mutex> lock4(updatect);
      readct--;
      if (readct == 0)
      {
        wrt.unlock();
      }
      iom.lock();
      std::cout << "Reader " << id << " finished reading.\n";
      iom.unlock();
    }
  }
}

void writer(int id)
{
  while (true)
  {
    iom.lock();
    std::cout << "Writer " << id << " is entering.\n";
    iom.unlock();
    wrt.lock();
    iom.lock();
    std::cout << "Writer " << id << " is writing\n";
    iom.unlock();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    wrt.unlock();
    iom.lock();
    std::cout << "Writer " << id << " has finished writing\n";
    iom.unlock();
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
}

int main()
{
  std::vector<std::thread> readers(READERS);
  std::vector<std::thread> writers(WRITERS);
  for (int i = 0; i < READERS; i++)
  {
    readers[i] = std::thread(reader, i);
  }
  for (int i = 0; i < WRITERS; i++)
  {
    writers[i] = std::thread(writer, i);
  }
  for (auto &r : readers)
  {
    if (r.joinable())
      r.join();
  }
  for (auto &w : writers)
  {
    if (w.joinable())
      w.join();
  }
}
