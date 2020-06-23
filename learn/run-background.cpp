#include <iostream>
#include <thread>

/** 
 * Calling detach() on a std::thread object leaves the thread to run in the background,
 *  - We cannot communicate with it at all 
 * 
 * ownership and control are passed over to the C++ Runtime
 * Library, which ensures that the resources associated with the thread are correctly
 * reclaimed when the thread exits
*/

/** 
 * Detached Threads are often called daemon threads.
 * that runs in the background without any explicit UI. 
 * 
 * They can run for the entire lifetime doing background tasks 
 * like monitoring the filesystem,
 * clearing unused entries out of object caches 
 * 
*/

/** 
 * You can pass arguments to a thread function too 
*/

void f(int i,std::string const& s)
{
    std::cout << i << s; 
}

void not_oops( int some_params )
{
    // //You can allocate a char buffer like below 
    char buffer[1000]; 
    sprintf(buffer,"Hello");
    // // But the problem is by the time 
    // // function exits, the conversion from 
    // // char* to std::string wouldn't have happened 

    // std::thread t(f, some_params, buffer);

    // So, just cast it to a string directly 
    std::thread t(f, some_params, std::string(buffer));
    t.detach();

    // Just detaching a thread doesn't give it permission to outlive the main thread
    // This is why detach doesn't sometimes print 
    // By the time, the print is called in the detached thread
    // The main thread ends the program. 
}

/** 
 * Thread is oblivious to the types and references 
 * So, for example
*/
/*
// The function below takes a reference 
void update_data_for_widget(widget_id w,widget_data& data);

void oops_again(widget_id w)
{
    widget_data data;
    // Here, the thread calls the function and it creates a reference 
    // to the internal copy of data not the data itself. 

    // This means when the function exits, the references don't mean shit
    // since all the updates are done within the function 
    // will be disregarded since internal copies are destroyed
    std::thread t(update_data_for_widget,w,data);
    display_status();
    t.join();
    // The old data is processed not the updated data
    process_widget_data(data);
}

/** 
 * Solution: wrap the references with std::ref
 * std::thread t(update_data_for_widget,w,std::ref(data));
*/

class X
{
public:
    void do_lengthy_work()
    {
        for(int i = 0; i < 100000; i++)
            std::cout << "Length work" << std::endl;
    }
};
    
/** 
 * std::move is when the data held within one object is moved over 
 * leaving the original object empty 
 * 
 * We can use this and unique_ptr to give the ownership 
 * of dynamic object into a thread 
*/
void process_big_object(std::unique_ptr<X>);

void foo_move()
{
    std::unique_ptr<X> p(new X);
    std::thread t(process_big_object, std::move(p));
    // The ownership of big_object is transferred into
    // internal storage for newly created thread and 
    // then into process_big_object
}

int main()
{
    // not_oops(6);
    X my_x;
    std::thread t(&X::do_lengthy_work, &my_x);
    t.join();

    return 0;
}