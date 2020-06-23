#include <iostream>
#include <thread>

/** 
 * Transferring ownership of a thread can be done using move
*/

void some_function(); 
void some_other_function(); 

void f()
{
    std::thread t1(some_function); 
    std::thread t2 = std::move(t1); 
    t1 = std::thread(some_other_function); 
    std::thread t3; 
    t3 = std::move(t2);
    t1 = std::move(t3); 
    // Since t1 is running some_other function 
    // std::terminate() is called. You need to either detach or join 
    // before destruction 
}

class scoped_thread
{
    std::thread t;
    public:
        explicit scoped_thread(std::thread t_):
        t(std::move(t_))
        {
            if(!t.joinable())
                throw std::logic_error(“No thread”);
        }

        ~scoped_thread()
        {
            t.join();
        }

        scoped_thread(scoped_thread const&)=delete;
        scoped_thread& operator=(scoped_thread const&)=delete;
};

struct func;

void f2()
{
    int some_local_state = 0;
    scoped_thread t(std::thread(func(some_local_state)));

    do_something_in_current_state();
}

int main()
{

    return 0;
}