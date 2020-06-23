#include <iostream>
#include <thread>
#include <mutex>
#include <list>
#include <algorithm>
#include <deque>
#include <exception>
/** 
 * If all the shared data is read-only, there is no problem with sharing the data 
 * amongst other threads. 
 * 
 * Invariants: A statement that is true about a data-structure 
 *  - Number of elements in a list 
 * 
 * Consider a doubly-linked list where each node holds pointer to next and previous node in the list 
 * 
 * In a double-linked list like this, one of the invariants is that at any moment, 
 * for any node, the next pointer of A points to B and prev pointer of B points to A
 * 
 * Steps of deleting an entry 
1 Identify the node to delete (N).
2 Update the link from the node prior to N to point to the node after N.
3 Update the link from the node after N to point to the node prior to N.
4 Delete node N.

Between steps 2 and 3, the invariant that the next and previous point to valid node is false 
Also, if you delete the right most node, the data structure is now corrupted and may crash the program 

Race conditions:
================
This is when the outcome depends on the relative ordering of execution of operations on two or more threads. 
Threads race to perform their respective operations. 

Race conditions are usually fine but, it is when invariants are broken, it becomes a problem. 
Usually occurs when completing an operation requires modification of two or more distinct pieces of data

Because the operation must access two separate pieces of data, these must be modified in separate instructions, 
and another thread could potentially access the data structure when only one of them has been completed
*/

/**
 * There are three ways to avoid race condition:
 * 
 * 1. Wrap the data with a protection mechanism 
 *    This ensures that only the thread actually performing a modification 
 * 2. Lock-free programming - Chapter 7 
 * 3. Transactions 
 *    Requires series of data modifications and reads are stored as transactions 
 *    then committed in a single step .
 *    If the commit cannot be proceeded because the data structure has been modified by
 *    another thread, the transaction is restarted.
 */


/** 
 * Protecting shared data with mutexes:
 * ====================================
 * 
 * Would it not be nice if you could mark all the code in data structure as mutually exclusive so, 
 * when a thread is busy running one of them and the other has to wait? 
 * 
 * The basic idea is you lock the mutex associated with that data before you access it 
 * Then, once you have finished accessing the data structure, you unlock the mutex
 * 
 * There are few problems with it - dealock and protecting either too much or too little data
 * 
 * Use lock_guard class template which implements the RAII for a mutex 
*/

std::list<int> some_list; 
std::mutex some_mutex;
void add_to_list( int new_value )
{
    std::lock_guard<std::mutex> guard(some_mutex);
    some_list.push_back( new_value );
}

bool list_contains( int val )
{
    std::lock_guard<std::mutex> guard(some_mutex);
    return std::find(some_list.begin(), some_list.end(), val ) != some_list.end();
}

/*
    It is better if all the member functions of the class lock the mutex before 
    accessing any other data members and unlock it when done, the data is nicely protected. 

    Well... yes and no...  Any code that has access to that
    pointer or reference can now access (and potentially modify) the protected data without locking the
    mutex. So, technically, it is not protected. 

    RULE OF THUMB:
    =================
    Don’t pass pointers and references to protected data outside the scope of the lock, whether by
    returning them from a function, storing them in externally visible memory, or passing them as
    arguments to user-supplied functions.
*/

class some_data
{
    int a;
    std::string b;

    public:
        void do_something();
};
class data_wrapper
{
    private:
        some_data data;
        std::mutex m;
    public:
        template<typename Function>
        void process_data(Function func)
        {
            std::lock_guard<std::mutex> l(m);
            func(data);
        }   
};

some_data* unprotected;

void malicious_function(some_data& protected_data)
{
    /** 
     * Even though, mutex is added - we are able to access protected data 
    */
    unprotected=&protected_data;
}

data_wrapper x;

void foo()
{
    x.process_data(malicious_function);
    unprotected->do_something();
}

/** 
 * Spotting race conditions inherent in interfaces
 * =================================================
 * 
 * Consider a doubly linked list again. If you want to delete a node safely, 
 * the node be ing deleted and the nodes on either side accesses should be prevented  
 * 
 * You need single mutex that protects the entire list 
*/

template<typename T,typename Container=std::deque<T> >
class stack
{
    public:
        explicit stack(const Container&);
        explicit stack(Container&& = Container());
        template <class Alloc> explicit stack(const Alloc&);
        template <class Alloc> stack(const Container&, const Alloc&);
        template <class Alloc> stack(Container&&, const Alloc&);
        template <class Alloc> stack(stack&&, const Alloc&);
        bool empty() const; // Value returned is not manipulated 
        size_t size() const;
        T& top();
        T const& top() const;
        void push(T const&);
        void push(T&&);
        void pop();
        void swap(stack&&);
};

/**
 * The results of empty() and size() can’t be relied on.
 * Although it may be correct at the time of the call, 
 * once they've returned other threads are free to access the stack 
 * which can changes these constants returned by empty() and size()
 * 
 * With a shared stack object which can be manipulated by multiple strings
 * This call sequence is no longer safe, because there might be a call to pop() from another thread that removes the last
 * element in between the call to empty() and the call to top()
 * 
 * This is a problem within the interface itself. 
 * 
 * So, what's the solution? 
 * 1. Just declare that top() will throw an exception if there are not 
 *    any elements when it is called 

    The stack is protected by a mutex internally, only one thread can be running a
    stack member function at any one time, so the calls get nicely interleaved, while the
    calls to do_something() can run concurrently.
*/

/** 
 * Thread safe stack
*/

struct empty_stack: std::exception
{
    const char* what() const throw();
};

template<typename T>
class threadsafe_stack
{
private: 
    std::stack<T> data; 
    mutable std::mutex m;

public:
    threadsafe_stack() {}
    threadsafe_stack(const threadsafe_stack& other)
    {
        // Have a lockguard on the mutex of the copyee
        std::lock_guard<std::mutex> lock(other.m);
        data = other.data;
    }

    threadsafe_stack operator=(const threadsafe_stack&) = delete;

    void push(T value)
    {
        std::lock_guard<std::mutex> lock(m);
        data.push(value);
    }

    std::shared_ptr<T> pop( void )
    {
        std::lock_guard<std::mutex> lock(m);
        // Check if the data is empty 
        // empty_stack calls the constructor of std::exception 
        // virtual what() in base std::exception and overridden 
        if(data.empty()) throw empty_stack();
        //If the pop() function was defined to return the value popped, as well as remove it from the stack,
        // you have a potential problem: the value being popped is returned to the caller only
        // after the stack has been modified, but the process of copying the data to return to the
        // caller might throw an exception. If this happens, the data just popped is lost; it has
        // been removed from the stack, but the copy was unsuccessful!

        // get the top element (top())
        // and then remove it from the stack (pop()), so that if you can’t safely copy the data,
        std::shared_ptr<T> const res(std::make_shared<T>(data.top()));
        data.pop();
        return res;
    }

    void pop(T& popped_val)
    {
        std::lock_guard<std::mutex> lock(m);
        if(data.empty()) throw empty_stack();
        popped_val = data.top();
        data.pop();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};