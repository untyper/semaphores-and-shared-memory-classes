// Copyright (c) [2024] [Jovan J. E. Odassius]
//
// License: MIT (See the LICENSE file in the root directory)
// Github: https://github.com/untyper/semaphores-and-shared-memory-classes

#ifndef SEMAPHORES_AND_SHARED_MEMORY_CLASSES_H
#define SEMAPHORES_AND_SHARED_MEMORY_CLASSES_H

#include <iostream>

#ifdef _WIN32
// Windows includes
#include <windows.h>
#else
// Unix includes
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstring>
#endif

// Define INFINITE for Unix
#ifndef _WIN32
  #ifndef INFINITE
  #define INFINITE ((unsigned int)-1)
  #endif
#endif

namespace sasm
{
  // ********** Declarations **********

  // Simple cross platform Semaphore class for Unix and Windows
  class Semaphore
  {
    static constexpr int max_count{ 1024 };
    std::string name;

#ifdef _WIN32
    HANDLE object{ nullptr };
#else
    sem_t* object{ nullptr };
#endif

  public:
    // Getters
    const std::string& get_name() const;
    
#ifdef _WIN32
    HANDLE get_object() const;
#else
    sem_t* get_object() const;
#endif

    void close();
    bool wait(unsigned int timeout_ms = INFINITE) const;
    bool increment(int count = 1) const;
    bool create(const std::string& name, int initial_count = 0);

    // Constructor
    Semaphore(const std::string& name, int initial_count = 0);

    // Ask compiler to generate these for us
    Semaphore(const Semaphore&) = default;            // Copy constructor
    Semaphore& operator=(const Semaphore&) = default; // Copy assignment
    Semaphore(Semaphore&&) = default;                 // Move constructor
    Semaphore& operator=(Semaphore&&) = default;      // Move assignment

    Semaphore() {} // Default constructor
    ~Semaphore();
  };

  // Simple cross platform Shared_Memory class for Unix and Windows
  class Shared_Memory
  {
    std::string name;
    std::size_t size{ 0 };
    void* address{ nullptr };

#ifdef _WIN32
    HANDLE file_mapping{ nullptr };
#else
    int file_mapping{ -1 };
#endif

    void* map();

  public:
    // Getters
    const std::string& get_name() const;
    std::size_t get_size() const;
    void* get_address() const;
#ifdef _WIN32
    HANDLE get_file_mapping() const;
#else
    int get_file_mapping() const;
#endif
    

    void close();
    bool create(const std::string& name, std::size_t size);

    // Constructor
    Shared_Memory(const std::string& name, std::size_t size);

    // Ask compiler to generate these for us
    Shared_Memory(const Shared_Memory&) = default;            // Copy constructor
    Shared_Memory& operator=(const Shared_Memory&) = default; // Copy assignment
    Shared_Memory(Shared_Memory&&) = default;                 // Move constructor
    Shared_Memory& operator=(Shared_Memory&&) = default;      // Move assignment

    Shared_Memory() {} // Default constructor
    ~Shared_Memory();
  };

  // ********** Definitions **********

  // Semaphore

  inline const std::string& Semaphore::get_name() const
  {
    return this->name;
  }

#ifdef _WIN32
  HANDLE Semaphore::get_object() const
#else
  sem_t* Semaphore::get_object() const
#endif
  {
    return this->object;
  }

  inline void Semaphore::close()
  {
    if (this->object == nullptr)
    {
      return;
    }

    // Release any blocked threads.
    this->increment(Semaphore::max_count);

#ifdef _WIN32
    CloseHandle(this->object);
#else
    sem_close(this->object);

    if (!this->name.empty())
    {
      sem_unlink(this->name.data());
    }
#endif

    // Clear members
    this->object = nullptr;
    this->name.clear();
  }

  inline bool Semaphore::wait(unsigned int timeout_ms) const
  {
    if (this->object == nullptr)
    {
      return false;
    }

#ifdef _WIN32
    DWORD result = WaitForSingleObject(this->object, timeout_ms);
    return result == WAIT_OBJECT_0;
#else
    if (timeout_ms == INFINITE)
    {
      return sem_wait(this->object) == 0;
    }
    else
    {
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      ts.tv_sec += timeout_ms / 1000;
      ts.tv_nsec += (timeout_ms % 1000) * 1000000;

      if (ts.tv_nsec >= 1000000000)
      {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
      }

      return sem_timedwait(this->object, &ts) == 0;
    }
#endif
  }

  inline bool Semaphore::increment(int count) const
  {
    if (this->object == nullptr)
    {
      return false;
    }

#ifdef _WIN32
    return ReleaseSemaphore(this->object, count, nullptr);
#else
    bool success = true;

    for (int i = 0; i < count; ++i)
    {
      if (sem_post(this->object) != 0)
      {
        success = false;
      }
    }

    return success;
#endif
  }

  inline bool Semaphore::create(const std::string& name, int initial_count = 0)
  {
    if (name.empty())
    {
      return false;
    }

    this->name = name;

#ifdef _WIN32
    this->object = CreateSemaphoreA(nullptr, initial_count, LONG_MAX, name.data());
    return this->object != nullptr;
#else
    this->object = sem_open(name.data(), O_CREAT, 0666, initial_count);
    return this->object != SEM_FAILED;
#endif
  }

  inline Semaphore::Semaphore(const std::string& name, int initial_count)
  {
    this->create(name, initial_count);
  }

  inline Semaphore::~Semaphore()
  {
    this->close();
  }

  // Shared_Memory

  inline const std::string& Shared_Memory::get_name() const
  {
    return this->name;
  }

  inline std::size_t Shared_Memory::get_size() const
  {
    return this->size;
  }

  inline void* Shared_Memory::get_address() const
  {
    return this->address;
  }

#ifdef _WIN32
  inline HANDLE Shared_Memory::get_file_mapping() const
#else
  inline int Shared_Memory::get_file_mapping() const
#endif
  {
    return this->file_mapping;
  }

  inline void Shared_Memory::close()
  {
#ifdef _WIN32
    if (this->file_mapping == nullptr)
#else
    if (this->file_mapping == -1)
#endif
    {
      return;
    }

#ifdef _WIN32
    if (this->address != nullptr)
    {
      UnmapViewOfFile(this->address);
    }

    CloseHandle(this->file_mapping);
#else
    if (this->address != nullptr)
    {
      munmap(this->address, this->size);
    }

    close(this->file_mapping);

    if (!this->name.empty())
    {
      shm_unlink(this->name.data());
    }
#endif

    // Finally clear members
    this->name.clear();
    this->size = 0;
    this->address = nullptr;

#ifdef _WIN32
    this->file_mapping = nullptr;
#else
    this->file_mapping = -1;
#endif
  }

  inline void* Shared_Memory::map()
  {
    if (this->file_mapping == nullptr)
    {
      return nullptr;
    }

    if (this->size == 0)
    {
      return nullptr;
    }

#ifdef _WIN32
    void* address = MapViewOfFile(this->file_mapping, FILE_MAP_ALL_ACCESS, 0, 0, this->size);

    if (address == nullptr)
    {
      CloseHandle(this->file_mapping); // Cleanup handle
      this->file_mapping = nullptr;
    }

    return address;
#else
    void* address = mmap(nullptr, this->size, PROT_READ | PROT_WRITE, MAP_SHARED, this->file_mapping, 0);

    if (address == MAP_FAILED)
    {
      close(this->file_mapping); // Cleanup file descriptor
      this->file_mapping = -1;
      return nullptr;
    }

    return address;
#endif
  }

  inline bool Shared_Memory::create(const std::string& name, std::size_t size)
  {
    if (name.empty())
    {
      return false;
    }

    if (size == 0)
    {
      return false;
    }

    this->name = name;
    this->size = size;

#ifdef _WIN32
    this->file_mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, name.data());
    // return this->file_mapping != nullptr;
#else
    int shm_fd = shm_open(name.data(), O_CREAT | O_RDWR, 0666);

    if (shm_fd == -1)
    {
      return false;
    }

    if (ftruncate(shm_fd, size) == -1)
    {
      close(shm_fd);
      shm_unlink(name.data());
      return false;
    }

    this->file_mapping = shm_fd;
    // return true;
#endif

    // Now map to memory
    this->address = this->map();
    return this->address != nullptr;
  }

  inline Shared_Memory::Shared_Memory(const std::string& name, std::size_t size)
  {
    this->create(name, size);
  }

  inline Shared_Memory::~Shared_Memory()
  {
    this->close();
  }
} // namespace sasm

#endif // SEMAPHORES_AND_SHARED_MEMORY_CLASSES_H
