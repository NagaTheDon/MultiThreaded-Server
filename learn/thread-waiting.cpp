#include <iostream>
#include <thread>

void do_something( int i )
{
    std::cout << "Do something:" << i << std::endl;
}

class thread_guard
{
    std::thread& t;
public: 
    explicit thread_guard(std::thread& t_) : t(t_){}

    ~thread_guard()
    {
        if(t.joinable())
        {
            t.join();
        }
    }

    /** 
     * These are needed to prevent the compiler from automatically 
     * assigning them. Otherwise, 
     * the thread will live outside the scope of thread it was joining
    */
    thread_guard(thread_guard const&) = delete;
    thread_guard& operator=(thread_guard const&) = delete;
};

struct func
{
    int& i;
    func(int& i_):i(i_){}
    void operator()()
    {
        for(unsigned j=0;j<10000;++j)
        {
            do_something(j);
        }
    }
};

void do_something_in_current_thread()
{
    for(int i = 0; i < 5000; i++ )
    {
        std::cout << "Current thread:" << i << std::endl;
    }
}

/** 
 * When this function exits, the destructors are called in reverse order
 * 
 * This means the thread guard destructor is called which 
 * joins the child thread to initial thread. 
*/
void f()
{
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread t(my_func);
    thread_guard g(t);
    do_something_in_current_thread();
}

int main()
{
    f();
    return 0;
}