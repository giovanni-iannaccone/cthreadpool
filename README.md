# 🏊‍♂️ Thread Pool

This project implements a lightweight thread pool in C using the POSIX threads (pthread) library. It allows efficient concurrent execution of tasks using a configurable number of threads and a dynamic task queue. Ideal for basic multithreading needs in systems programming and performance-critical applications.

## 🚀 Features
- Fixed-size or dynamically resizable thread pool
- Graceful shutdown with support for pending task completion
- Thread-safe task submission and pool resizing

## 📦 Prerequisites
The project assumes a UNIX-like environment (e.g. Linux, macOS).

## 🧮 Installation
1. First clone the repository
```
git clone https://github.com/giovanni-iannaccone/cthreadpool
```

2. Include the library in your project
```c
#include "cthreadpool.h"
```

## 🛠️ Usage
- `new_threadpool(n_threads, queue_size)`: Initializes a threadpool with n_threads and a task queue of size queue_size.
- `submit_task(pool, function, arguments)`: Adds a task to the queue. The task is a function pointer with optional arguments.
- `destroy_threadpool(pool)`: Safely terminates threads, destroys synchronization primitives, and frees memory.

You can also resize:
- `inc_tasksqueue_size(pool, inc)`: Dynamically increases the task queue size.
- `inc_threadpool_size(pool, inc)`: Adds threads to the pool on-the-fly.

## 🧩 Contributing
We welcome contributions! Please follow these steps:

1. Fork the repository.
2. Create a new branch ( using <a href="https://medium.com/@abhay.pixolo/naming-conventions-for-git-branches-a-cheatsheet-8549feca2534">this</a> convention).
3. Make your changes and commit them with descriptive messages.
4. Push your changes to your fork.
5. Create a pull request to the main repository.

## ⚖️ License
This project is licensed under the GPL-3.0 License. See the LICENSE file for details.

## ⚔️ Contact
- For any inquiries or support, please contact <a href="mailto:iannacconegiovanni444@gmail.com"> iannacconegiovanni444@gmail.com </a>.
- Visit my site for more informations about me and my work <a href="https://giovanni-iannaccone.github.io" target=”_blank” rel="noopener noreferrer"> https://giovanni-iannaccone.github.io </a>