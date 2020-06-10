# Chapter 1 
> This markdown document covers the notes taken from Chapter 1 of **C++ Concurrency in Action** - Anthony Williams

One of the most significant new features of C++11 standard is the support of multi-threaded programs. 
## Introduction
### What is  concurrency?

In a nutshell, concurrency is about two or more seperate activities happening at the same time. Author relates this concept with human's ability to walk and talk at the same time; 

In computers, it means a single system performing multiple independent activities in parallel rather than in sequential manner. Concurrency has been a thing for a long time where multitasking OS allow multiple applications to run at the same time using methods ike task switching. This gives an illusion of doing concurrency but, it really isn't. 

Nowadays, computers contains multiple processors to perform true concurrency. This could be due to the fact the computer has multiple processors or multiple cores within a processor. 

### Context Switch

<p align="center">
  <img src="./img/context_switch.jpg"/>
</p>

The image above shows how two individual tasks are carried out on a dual core vs single core. As you can see on a single core, it has a much thicker seperator bars than ones on single core. 

This is because when you want switch task in single core, the system has to perform *context switch* everytime and this consumes considerably more time. 

OS performs *Context Switch*  by: 
* Save the CPU state
* Save instruction pointer for the currently running task
* Work out which task to switch to
* Reload the CPU state for the task being switched to

There are also several processors which can execute multipe threads on a single core. Therefore, the importact factor is:  **hardware threads** is the measure of how many independent tasks the hardware can genuinely run concurrently.   

Even with a system that has genuine hardware concurrency, it’s easy to have
more tasks than the hardware can run in parallel. So, *context switching* needs to be used. 

## Approaches to concurency

Authors postulates another example. Imagine you had two programmers working in a building. You can give them their resources and seperate offices to solve a problem. But, here the overheads: using two offices rather than one and communication. 

Another way would be to put them in the same office. But, now the problem will turn into resource allocation (who gets to use user manual/computer?) but, communication will be btter. 

Similarly, each develop represents a thread and each office represents a process. The first approach is a **multiple single-threaded process** and second approach is **multiple-thread in single process**. 

### Concurrency with multiple processes

This is when a running application is divided into multiple, seperate single-threaded processes that are run at the same time. The communication between these seperate processes occurs through interconnects like signals, cockets, files, pipes and so on. 

Disadvantages include:
* Complicated or slow to set-up - OS provides several protection between proceses to avoid modifying each other's data. 
* Inherent overhead: takes time to start a process; resource allocation

Advantages include: 
* Easier to write safe concurrent code with processes rather than threads
* Ability to run the separate processes on distinct machines connected over a network

### Concurrency with multiple threads
The other way is to run multiple threads in a single process.
Each thread runs independenly of others and each thread may run different sequence of instructions. But, they all share the same address space. This means the most of the data can be accessed directly from all the threads -- global variables, pointers, references and the data passed around. 

Advantages: 
* **Overhead is much smaller** - shared address space and lack of protection of data

Disadvantages:

* **Data consistency**: application programmer
must ensure that the view of data seen by each thread is consistent whenever it is accessed.

This method is preferred over seperate process since it has low overhead associated with launching and communicating. In addition, C++ Standard doesn’t provide any intrinsic support for communication between processes, so applications that use multiple processes will have to rely on platform-specific APIs to do so.

## Why concurrency?

* **Seperation of concerns** - To seperate distinct areas of functionality that needs to happen at the same time. 

* **Performance** - Allows approaches like *data parallelism* and *task-parallism* increasing the throughput of a program


## When not to use concurrency?
1. DO NOT USE CONCURRENCY WHEN BENEFIT IS NOT WORTH THE COST. 

You have to bear in mind although the thread is lightweight compared to process - it takes time and resources:
* Allocate associated kernel resources
* Allocate stack space
* Add the new thread to the scheduler

2. Too many threads running at once consumes OS resources and make the system as a whole slower. Possibility of exhausting the available memory or address space for a process, because each thread requires a separate stack space. 

Along with this, the more threads you have running, the more context switching the operating system has to do reducing the overall application. Therefore, a thing called **Thread pool** should be used. 
