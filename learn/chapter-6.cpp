#include <iostream>
#include <thread>
#include <exception>
/*
If a data structure is to be accessed from multiple threads, 
* must be completely immutable so the data never changes and no synchronization is necessary
* ensure that changes are correctly synchronized between threads

6.1. What does it mean to design for concurrency?
===================================================
Concurrency means multiple threads can access the data structure concurrently
 * performing the same or distinct operations
 * each thread will see a self-consistent view of the data structure
 * No data will be lost or corrupted
 * All invariants will be upheld,
 * No problematic race conditions.
Such a data structure is said to be thread-safe. 

It should be possible: 
* To have multiple threads performing one type of operation on the data structure
concurrently
* Another operation requires exclusive access by a single thread
* Should be safe to have multiple threads to access a data structure concurrently
if they’re performing different actions
* Multiple threads performing the same action might be problematic

But, truly designing for concurrency means more than that:
It means: 
Providing the opportunity for concurrency to threads accessing 
the data structure. 

Mutex provides mutual exclusion: only one thread can acquire a lock on the
mutex at a time. However, a mutex protects a data structure by explicitly preventing 
true concurrent access to the data it protects. This is called serialisation. 
This is when threads take turns accessing the data protected by the mutex where
they must access it serially rather than concurrently. 

Guidelines for designing data structures for concurrency
==========================================================
You need to make sure you have two aspects to consider when designing 
data structures for concurrent access:
1. Ensure that the accesses are safe
2. Enabling genuine concurrent access. 

We already looked at how to make a datastructure thread-safe:
* Ensure that no thread can see a state where the invariants of the data structure
have been broken by the actions of another thread.

* Take care to avoid race conditions inherent in the interface to the data structure
by providing functions for complete operations rather than for operation steps.

* Pay attention to how the data structure behaves in the presence of exceptions to
ensure that the invariants are not broken.

* Minimize the opportunities for deadlock when using the data structure by
restricting the scope of locks and avoiding nested locks where possible.

Think about questions like :
* If one thread is accessing the data structure through a particular function, which functions are safe to call from
other threads?
* Can the scope of locks be restricted to allow some parts of an operation to be
performed outside the lock?
* Can different parts of the data structure be protected with different mutexes?
* Do all operations require the same level of protection?
* Can a simple change to the data structure improve the opportunities for concurrency
without affecting the operational semantics?

6.2. Lock-based concurrent data structures:
===========================================
- All about ensuring that the right mutex is locked when accessing the data and ensuring that the lock is held for a
minimum amount of time.
- Need to ensure that data can’t be accessed outside the protection
of the mutex lock and there are no race conditions

Thread-safe queue using condition variables:
============================================
*/

template<typename T>
class threadsafe_queue
{
private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;
public:
    threadsafe_queue()
    {}
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(std::move(new_value));
        /** 
         * The problem with this is that if one of the threads throws an exception 
         * while constructing shared_ptr<> is constructed, 
         * none of the other threads will be woken up even though, they are waiting
         * 
         * Alternatives to eliminate this problem: 
         * data_cond.notify_all() - Wakes all the thread but, most will go back to sleep
         * wait_and_pop() calling notify_one() if exception is thrown 
         * Move std::shared_ptr<> to push() and storing std::shared_ptr<> instances rather than direct values 
         * Copying the std::shared_ptr<> out of the internal std::queue<> then can’t throw an exception,
        */
        data_cond.notify_one();
    }

    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk,[this]{return !data_queue.empty();});
        value=std::move(data_queue.front());
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(mut);
        /** 
         * data_cond.wait() won’t return until the
         * underlying queue has at least one element, so you don’t have to worry about the possibility
        *  of an empty queue at this point in the code, and the data is still protected with
        * the lock on the mutex.
        */
        data_cond.wait(lk,[this]{return !data_queue.empty();});
        std::shared_ptr<T> res(
        std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }
    /** 
     * Overloaded
     * @returns bool value - A value is retrieved or not 
    */
    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
            return false;
        value=std::move(data_queue.front());
        data_queue.pop();
        return true;
    }
    /** 
     * Overloaded
    */
    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};

/** 
 * Let's take all of this into consideration and try again
 * 
*/

template<typename T>
class threadsafe_queue
{
    private:
        mutable std::mutex mut;
        std::queue<std::shared_ptr<T> > data_queue;
        std::condition_variable data_cond;
    public:
        threadsafe_queue()
        {}
        void wait_and_pop(T& value)
        {
            std::unique_lock<std::mutex> lk(mut);
            data_cond.wait(lk,[this]{return !data_queue.empty();});
            value=std::move(*data_queue.front());
            data_queue.pop();
        }
        bool try_pop(T& value)
        {
            std::lock_guard<std::mutex> lk(mut);
            if(data_queue.empty())
                return false;
            value=std::move(*data_queue.front());
            data_queue.pop();
            return true;
        }

        std::shared_ptr<T> wait_and_pop()
        {
            std::unique_lock<std::mutex> lk(mut);
            data_cond.wait(lk,[this]{return !data_queue.empty();});
            std::shared_ptr<T> res=data_queue.front();
            data_queue.pop();
            return res;
        }

        std::shared_ptr<T> try_pop()
        {
            std::lock_guard<std::mutex> lk(mut);
            if(data_queue.empty())
            return std::shared_ptr<T>();
            std::shared_ptr<T> res=data_queue.front();
            data_queue.pop();
            return res;
        }

        void push(T new_value)
        {
            /** 
             * Since the data is held by std::shared_ptr, there is an additional benefit 
             * the allocation of the new instance can now be done outside the lock in push()
             * whereas before we had to lock in pop()
            */
            std::shared_ptr<T> data(
                std::make_shared<T>(std::move(new_value)));
            std::lock_guard<std::mutex> lk(mut);
            data_queue.push(data);
            data_cond.notify_one();
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> lk(mut);
            return data_queue.empty();
        }
};

/** 
 * Use of mutex to protect the entire data structure limits the concurrency
 * supported by this queue i.e.Only one thread can be doing any work at a time
 * 
 * This is becasyse of the use of std::queue<> in the implementation; 
 * by using the standard container you now have essentially one data time that is 
 * either protected or not. 
*/

/** 
 * Thread safe queue using fine-grained locks and condition variables:
 * ===================================================================
 * 
 * Previously, we had one protected data item (data_queue) and thus one mutex. 
 * In order to use finer-grained locking, you need to look inside the queue at its 
 * constituent parts and assiociate one mutex with each distincy data item
 * 
 * Simplest of these queues is a singly linked list: 
 * * Queue contains a head pointer  - first element in the list 
 * * Each item then points to the next item
 * * Data items are removed from the queue by replacing
     the head pointer
   * Queue also contains a tail pointer
   * New nodes are added by changing the next pointer of the last item
   * List is empty: both the head and tail pointers are NULL 

Below shows a simple single-threaded queue implementation
 * 
*/

template<typename T>
class queue
{
    private:
        struct node
        {
            T data;
            std::unique_ptr<node> next;
            node(T data_): data(std::move(data_))
            {}
        };
        /** 
         * You can modify both head and tail so, it would have to lock 
         * both mutexes. Both push() and pop() access the next pointer
         * of a node: push() updates tail->next, and try_pop() reads head->next
         * 
         * When there is a single element -> head==tail and head->next == tail->next
         * 
         * This means you have to lock the same mutex in both push and try_pop()
         * To even compare it, you have to lock both mutex.. So, is there a way out of this? 
         *  See the next comment 
        */
        std::unique_ptr<node> head;
        node* tail;
    public:
        queue() {}
        queue(const queue& other)=delete;
        queue& operator=(const queue& other)=delete;

        std::shared_ptr<T> try_pop()
        {
            if(!head)
            {
                return std::shared_ptr<T>();
            }
            std::shared_ptr<T> const res(
                std::make_shared<T>(std::move(head->data)));
                /** 
                 * unique_ptr<node> to manage the nodes, because 
                 * this ensures that they get deleted when they're no
                 * longer needed automatically. 
                 * 
                 * This implementation works fine in a single-threaded context 
                 * a couple of things will cause you problems if you try to use 
                 * fine-grained locking in a multi-threaded context. 
                */
            std::unique_ptr<node> const old_head = std::move(head);
            head=std::move(old_head->next);
            return res;
        }

        void push(T new_value)
        {
            std::unique_ptr<node> p(new node(std::move(new_value)));
            node* const new_tail=p.get();
            
            if(tail)
            {
                tail->next=std::move(p);
            }
            else
            {
                head=std::move(p);
            }

            tail=new_tail;
        }
};

/** 
 * The problem can be eliminated by preallocating a dummy node 
 * with no data to ensure that theres always at least one node 
 * in the queue which will seperate the node being accessed at 
 * the head from the being accessed at the tail. 
 * 
 * For an empty queue, head and tail now both point to the 
 * dummy node rather than being NULL. 
 * 
 * This means: 
 * * try_pop() doesn’t access head->next if the queue is empty.
 * * If you add a node, head and tail now point to seperate nodes 
 *   and there is no race on head->next and tail->next
 *   (an extra dummy node is created everytime)
 * `
*/

template<typename T>
class queue
{
    private:
        struct node
        {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
        };
        std::unique_ptr<node> head;
        node* tail;
    public:
        queue():
        head(new node),tail(head.get())
        {}
        queue(const queue& other)=delete;
        queue& operator=(const queue& other)=delete;

        /** 
         * Accesses both head and tail, 
         * but tail is needed only for comparison 
         * 
         * Another massive advantage is that try_pop
         * and push are never operating the same node
        */
        std::shared_ptr<T> try_pop()
        {
            /** 
             * Checking for head.get() == tail 
             * because dummy node means head is never NULL
            */
            if(head.get()==tail)
            {
                return std::shared_ptr<T>();
            }
            std::shared_ptr<T> const res(head->data);
            std::unique_ptr<node> old_head=std::move(head);
            head=std::move(old_head->next);
            return res;
        }

    /** 
     * push() now accesses only tail not head which is an improvement
     * 
    */
    void push(T new_value)
    {
        std::shared_ptr<T> new_data(
        std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        tail->data=new_data;
        node* const new_tail=p.get();
        tail->next=std::move(p);
        tail=new_tail;
    }
};

/** 
 * Now, that we have implemented the thread-safe. Let's add the fine-grained lock 
*/
template<typename T>
class threadsafe_queue
{
    private:
    struct node
    {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    std::mutex head_mutex;
    std::unique_ptr<node> head;
    /** 
     * tail_mutex means empty node & data and next
     * are correctly for the old tail node which now the 
     * last real node in the list 
    */
    std::mutex tail_mutex;
    node* tail;

    node* get_tail()
    {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }

    std::unique_ptr<node> pop_head()
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if(head.get()==get_tail())
        {
            return nullptr;
        }
        std::unique_ptr<node> old_head=std::move(head);
        head=std::move(old_head->next);
        return old_head;
    }

    public:
    threadsafe_queue():
    head(new node),tail(head.get())
    {}
    threadsafe_queue(const threadsafe_queue& other)=delete;
    threadsafe_queue& operator=(const threadsafe_queue& other)=delete;

    std::shared_ptr<T> try_pop()
    {
        // Ca
        std::unique_ptr<node> old_head=pop_head();
        return old_head?old_head->data:std::shared_ptr<T>();
    }

    void push(T new_value)
    {
        std::shared_ptr<T> new_data(
        std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        node* const new_tail=p.get();
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        tail->data=new_data;
        tail->next=std::move(p);
        tail=new_tail;
    }
};  

/** 
 * These are list of invariants:
 * 1. tail->next==nullptr
 * 2. tail->data==nullptr;
 * 3. head==tail for empty list  
 * 4. Single element list has head head->next==tail 
 * 5. Following the next nodes from head will eventually yield tail.
 * 
 * So, I think I can explain this as clearly as I could: 
 * 
 * So, without the lock on the get_tail(), there could be a data race:
 * How? Imagine you trying to push and pop concurrently, 
 * This means when one thread is trying to add a node, it calls get_tail to check
 * and reads the value from tail and other thread will try to remove this tail from pop()
 * causing an undefined behaviour
 * 
 * This is clearly wrong. But, thankfully, the tail_mutex in get_tail() solves everything
 * Since the get_tail() locks the same mutex as the call to push()
 * 
 * Either: 
 * * get_tail() occurs before the push() - sees the old value of tail
 * * get_tail() occurs after the push() - sees the new value of tail and the new data is attached the previous value of tail
 * 
 * Make sure you tail_lock is done after the head_lock. 
 * 
 * What would happen otherwise? 
 * Imagine if the tail_lock is locked first and then, head_lock
 * 
 * By the time the tail_lock is acquired and get_tail() returns, 
 * another thread could have changed the head by popping or who knows even tail could have removed 
 * Then, the comparison for get_tail() may fail and 
 * 
 * If you lock tail_lock after head_tail, 
 * it will ensure that no other thread can change head and 
 * tail only moves further away
 * 
*/

/**
 * Waiting for an item to pop:
 * ===========================
 * 
 * Modifications for push():
 * -------------------------
 * 
 * Add data_cond.notify_one() at the end of the function
 * But, the problem is if you leave the mutex locked across 
 * the call to notify_one(), it will have to wait for the mutex to unlock
 * 
 * But, if you unlock the mutex before the notify_one(), it is available 
 * for the waiting thread to acquire it when it wakes up. 
 * 
 * Modifications for wait_and_pop:
 * -------------------------------
 * use get_tail() to lock the tail as it is only required for reading the tail value 
 *  -- NEED TO COME BACK 
 */

/** 
 * Writing a thread
 * 
*/

template<typename T>
class threadsafe_list
{
    struct node
    {
        std::mutex m;
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
        node():next()
        {}
        node(T const& value): data(std::make_shared<T>(value))
        {}
    };

    node head;
    public:
        threadsafe_list()
        {}
        ~threadsafe_list()
        {
            remove_if([](node const&){return true;});
        }
        threadsafe_list(threadsafe_list const& other)=delete;
        threadsafe_list& operator=(threadsafe_list const& other)=delete;

        void push_front(T const& value)
        {
            std::unique_ptr<node> new_node(new node(value));
            std::lock_guard<std::mutex> lk(head.m);
            new_node->next=std::move(head.next);
            head.next=std::move(new_node);
        }

        template<typename Function>
        void for_each(Function f)
        {
            node* current=&head;
            std::unique_lock<std::mutex> lk(head.m);
            while(node* const next=current->next.get())
            {
                std::unique_lock<std::mutex> next_lk(next->m);
                lk.unlock();
                f(*next->data);
                current=next;
                lk=std::move(next_lk);
            }
        }

        template<typename Predicate>
        std::shared_ptr<T> find_first_if(Predicate p)
        {
            node* current=&head;
            std::unique_lock<std::mutex> lk(head.m);
            while(node* const next=current->next.get())
            {
                std::unique_lock<std::mutex> next_lk(next->m);
                lk.unlock();
                if(p(*next->data))
                {
                    return next->data;
                }
                current=next;
                lk=std::move(next_lk);
            }
            return std::shared_ptr<T>();
        }

        template<typename Predicate>
        void remove_if(Predicate p)
        {
            node* current=&head;
            std::unique_lock<std::mutex> lk(head.m);
            while(node* const next=current->next.get())
            {
                std::unique_lock<std::mutex> next_lk(next->m);
                if(p(*next->data))
                {
                    std::unique_ptr<node> old_next=std::move(current->next);
                    current->next=std::move(next->next);
                    next_lk.unlock();
                }
                else
                {
                    lk.unlock();
                    current=next;
                    lk=std::move(next_lk);
                }
            }
        }
};

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <iterator>

int reducer(int a, int b)
{
  return a + b; 
}

struct node
{
    std::mutex m;
    std::shared_ptr<int> data;
    std::unique_ptr<node> next;
    node():next()
    {}
    node(int const& value): data(std::make_shared<int>(value))
    {}
};
template<typename T>
class threadsafe_list
{

    node head;
    public:
        threadsafe_list()
        {}
        ~threadsafe_list()
        {
            remove_if([](node const&){return true;});
        }
        threadsafe_list(threadsafe_list const& other)=delete;
        threadsafe_list& operator=(threadsafe_list const& other)=delete;


        void push_front(T const& value)
        {
            std::unique_ptr<node> new_node(new node(value));
            std::lock_guard<std::mutex> lk(head.m);
            new_node->next=std::move(head.next);
            head.next=std::move(new_node);
        }

        template<typename Function>
        void for_each(Function f)
        {
            node* current=&head;
            std::unique_lock<std::mutex> lk(head.m);
            while(node* const next=current->next.get())
            {
                std::unique_lock<std::mutex> next_lk(next->m);
                lk.unlock();
                f(*next->data);
                current=next;
                lk=std::move(next_lk);
            }
        }

        template<typename Predicate>
        std::shared_ptr<T> find_first_if(Predicate p)
        {
            node* current=&head;
            std::unique_lock<std::mutex> lk(head.m);
            while(node* const next=current->next.get())
            {
                std::unique_lock<std::mutex> next_lk(next->m);
                lk.unlock();
                if(p(*next->data))
                {
                    return next->data;
                }
                current=next;
                lk=std::move(next_lk);
            }
            return std::shared_ptr<T>();
        }

        template<typename Predicate>
        void remove_if(Predicate p)
        {
            node* current=&head;
            std::unique_lock<std::mutex> lk(head.m);
            while(node* const next=current->next.get())
            {
                std::unique_lock<std::mutex> next_lk(next->m);
                if(p(*next->data))
                {
                    std::unique_ptr<node> old_next=std::move(current->next);
                    current->next=std::move(next->next);
                    next_lk.unlock();
                }
                else
                {
                    lk.unlock();
                    current=next;
                    lk=std::move(next_lk);
                }
            }
        }

        node* getHead()
        {
          return &head;
        }
};

node* getHead(std::vector<int> a)
{
  threadsafe_list<int>* list = new threadsafe_list<int>;
  for (auto i = a.begin(); i != a.end(); ++i)
  {
    // std::cout << *i << ' ';
    list->push_front(*i);
  }
  
  node* head = list->getHead();
  return head;
}
template<typename  Function>
int reduce_list(node* head, Function f)
{
  node* current=head;
  int sum = 0; 
  std::unique_lock<std::mutex> lk(head->m);
  while(node* const next=current->next.get())
  {
      std::unique_lock<std::mutex> next_lk(next->m);
      lk.unlock();
  
      if(current->data == NULL)
        sum = f(*(next->data), 0);
      else 
        sum = f(*(next->data), sum);
      std::cout << sum << std::endl;
      current=next;
      lk=std::move(next_lk);
  }

return sum;
}


int main()
{
  std::vector<int> a; 
  a.push_back(7);
  a.push_back(9);
  a.push_back(10);

  node* head = getHead(a);
  if(head)
  {
    std::cout << "HEAD IS AVAILABLE" << std::endl;
  }
  int sum = reduce_list(head, reducer);

  for (auto i = a.begin(); i != a.end(); ++i)
  {
    std::cout << *i << ' ';
  }
  std::cout << "SUM: " << sum << std::endl;
  return 0;
}