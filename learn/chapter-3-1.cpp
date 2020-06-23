#include <thread>
#include <mutex>
/** 
 * Deadlock 
 * 
 * Occurs when threads arguing over locks on mutexes
 * For example, imagine you have a pair of threads and 
 * pair of mutexes. Each of these threads will need both of 
 * the pair of mutexes to perform some operation. 
 * 
 * A deadlock is caused when each of these has one 
 * mutex and is waiting for the other. 
 * 
 * The rule of thumb for this problem is if you 
 * lock the mutexes in the same order at all times 
 * in all threads: So, always lock mutex A and then 
 * mutex B no matter which thread you are running on. 
 * 
 * This means if a secondary thread comes to locking mutexes, 
 * it will find that mutex A is locked and doesn't proceed 
 * until it is unlocked. 
 * 
 * std::lock - a function that can lock two or more mutexes 
 * at once without risk of deadlock. 
*/


class some_big_object
{};
void swap(some_big_object& lhs,some_big_object& rhs);

class X
{
    private:
        some_big_object some_detail;
        std::mutex m;
        public:
        X(some_big_object const& sd):some_detail(sd){}

        friend void swap(X& lhs, X& rhs)
        {
            // Acquiring a lock on a std::mutex when you already
            // hold, it is undefined behaviour. 
            if(&lhs==&rhs)
                return;

            // If std::lock has successfully locked one mutex 
            // and an exception is thrown when it acquires a lock on the other mutex
            // The first lock is released automatically 
            std::lock(lhs.m,rhs.m);
            // std::adopt_lock says that the mutexes are already locked and they should 
            // adopt the ownership of the existing lock on the mutex 
            // rather than attempting to try again 
            std::lock_guard<std::mutex> lock_a(lhs.m,std::adopt_lock);
            std::lock_guard<std::mutex> lock_b(rhs.m,std::adopt_lock);
            swap(lhs.some_detail,rhs.some_detail);
        }
};

/* 
 How else can you avoid deadlock? 
 =================================
 You can cause a deadlock by having each thread call 
 join() on the std::thread object for the other. 

 The rule of thumb is: DONT WAIT FOR ANOTHER THREAD IF 
 THERE IS A CHANCE IT IS WAITING FOR YOU 

 1. Avoid Nested Locks
 ======================
  Don't acquire a lock if you already hold one. In other words, 
  Don't let a thread have multiple locks on multiple mutexes

2. Avoid calling user-supplied code while holding a lock
==========================================================
It's simply because you wont know what it could do: including acquiring a lock, 

3. Acquire locks in a fixed order
=====================================
If you can't acquire two or more locks as a single-operation with std::lock, 
the next-best thing is to acquire them in the same order in every thread. 

4. Use a lock hierarchy
=============================
Idea is that you divide your application into layers and identify all the 
mutexes that may be locked in any given layer. 

When a code tries to lock a mutex, it is not permitted to lock 
that mutex if it already holds a lock from a lower layer. 

*/
class hierarchical_mutex
{
    std::mutex internal_mutex;
    unsigned long const hierarchy_value;
    unsigned long previous_hierarchy_value;
    static thread_local unsigned long this_thread_hierarchy_value;

    void check_for_hierarchy_violation()
    {
        if(this_thread_hierarchy_value <= hierarchy_value)
        {
            throw std::logic_error(“mutex hierarchy violated”);
        }
    }

    void update_hierarchy_value()
    {
        previous_hierarchy_value=this_thread_hierarchy_value;
        this_thread_hierarchy_value=hierarchy_value;
    }
    public:
        explicit hierarchical_mutex(unsigned long value):
        hierarchy_value(value),
        previous_hierarchy_value(0) {}

        void lock()
        {
            check_for_hierarchy_violation();
            internal_mutex.lock();
            update_hierarchy_value();
        }

        void unlock()
        {
            this_thread_hierarchy_value=previous_hierarchy_value;
            internal_mutex.unlock();
        }

        bool try_lock()
        {
            check_for_hierarchy_violation();
            if(!internal_mutex.try_lock())
                return false;
            update_hierarchy_value();
            return true;
        }
};
/** 
 * representing the hierarchy value for the current thread: this_thread_hierarchy_value
 * Maximum value since any mutex (which will definitely be lower) can be locked 
 * 
 * thread_local means every thread has its own copy so, the state of the variable 
 * in one thread is entirely independent of the state of variable when read from another thread
 * 
 * First time, since the hierarchial value is smaller than this_thread_hierarchy_value, it passes. 
 * 
 * internal_mutex.lock() locks the current mutex
 * Once the lock has been acquired, update the hierarchy value 
 * 
 * 
 * 
 * 
*/
thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX);

hierarchical_mutex high_level_mutex(10000);
hierarchical_mutex low_level_mutex(5000);

int do_low_level_stuff();

int low_level_func()
{
    std::lock_guard<hierarchical_mutex> lk(low_level_mutex);
    return do_low_level_stuff();
}

void high_level_stuff(int some_param);

void high_level_func()
{
    std::lock_guard<hierarchical_mutex> lk(high_level_mutex);
    high_level_stuff(low_level_func());
}

/** 
 * Abides by rules: 
 * Calls high_level_func which locks high_level_mutex
 * Then, calls low_level_func which locks low_level_mutex 
 * which is fine becays of a lower hierarchy value of 5000
 * 
*/
void thread_a()
{
    high_level_func();
}

hierarchical_mutex other_mutex(100);
void do_other_stuff();

void other_stuff()
{
    high_level_func();
    do_other_stuff();
}

/* 
* thread_b is not fine. 
* Locks other mutex which has a hierarchy value of 100. 
* other_stuff calls high_level_func -> violation

You cannot hold two locks at the same time if they are the same 
level in the hierarchy
*/
void thread_b()
{
    std::lock_guard<hierarchical_mutex> lk(other_mutex);
    other_stuff();
}
/* *
 * Another tip to prevent deadlock to ensure that your threads are
 * joined in the same function that started them
 */

/*
std::unique_lock:
=================
Gives us flexibility that does not always own the mutex that it is associated with

* std::adopt_lock as second argument - have constructor to have the lock object manage the lock on a mutex

* std::defer_lock as second argument - have constructor to indicate that the mutex should emain unlocked on construction
 lock() can be acquired later by calling lock() on std::unique_lock object

 Unique_lock takes more space and is fraction slower than lock_guard
*/

class some_big_object;
void swap(some_big_object& lhs,some_big_object& rhs);

class X
{
    private:
        some_big_object some_detail;
        std::mutex m;
    public:
        X(some_big_object const& sd):some_detail(sd){}
        friend void swap(X& lhs, X& rhs)
        {
            if(&lhs==&rhs)
                return;
            /** 
             * unique_lock provides lock(), try_lock() and unlock() member functions
             * These objects contain flags which will reprsent whether the mutex is currently 
             * owned by that instance. This is needed to ensure whether 
             * unlock needs to called or not if it does or does not own a mutex 
            */
            std::unique_lock<std::mutex> lock_a(lhs.m,std::defer_lock);
            std::unique_lock<std::mutex> lock_b(rhs.m,std::defer_lock);
            std::lock(lock_a,lock_b);
            swap(lhs.some_detail,rhs.some_detail);
        }
};

/** 
 * Transferring mutex ownership between scopes:
 * ============================================
 * 
 * Since std::unique_lock instances do not have to own their 
 * own associated mutexes, the ownership of a mutex can be transferred
 * between instances by moving the instances around. 
 * 
 * lvalue - a real variable or reference to one 
 * rvalue - a temporary of some kind 
 * 
 * The ownership trasnsfer of this type is automatically if it the source is a rvalue 
 * If it is a lvlaue, it must be done explicity through function like std::move 
 * 
*/

std::unique_lock<std::mutex> get_lock()
{
    extern std::mutex some_mutex;
    std::unique_lock<std::mutex> lk(some_mutex);
    prepare_data();
    return lk;
}
void process_data()
{
    // std::move is done implicitly 
    std::unique_lock<std::mutex> lk(get_lock());
    do_something();
}

/** 
 * 3.2.8 Locking at an appropriate granularity:
 * ============================================
 * Lock granuality is the amount of data protected by a single lock 
 * A fine-grained lock protects a small amount of data
 * coarse-grained lock protects a large amount of data 
 * 
 * If multiple threads are waiting for the same resource, 
 * 
 * In general, a lock should be held for only the
 * minimum possible time needed to perform the required operations
*/