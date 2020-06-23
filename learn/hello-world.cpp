#include <iostream>
#include <thread>

/** 
 * Every thread needs to have an initial function 
 * 
 * This is where the new thread of execution begins
*/
void hello()
{
    std::cout << "Hello World! \n";
}

int main()
{
    /** 
     * Program launches a whole new thread to do it 
     * bringing thread count to two
     * 
     * 1. Initial thread that starts at main 
     * 2. New thread that starts at hello 
     * 
     * If it didn't wait, it will continue to the end of the main
     * and thus end the program
     * 

    */
    std::thread t(hello); 
    /** 
     * This is why you use join() - 
     * This means the calling thread (main thread) 
     * waits for the thread associated with std::thread
     * object- thread t 
    */
    t.join();
    return 0; 
}