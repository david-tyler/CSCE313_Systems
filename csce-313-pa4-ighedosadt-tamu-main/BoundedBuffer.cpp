// your PA3 BoundedBuffer.cpp code here
#include "BoundedBuffer.h"

using namespace std;
#include <cassert>
#include <iostream>

BoundedBuffer::BoundedBuffer (int _cap) : cap(_cap) {
    // modify as needed
}

BoundedBuffer::~BoundedBuffer () {
    // modify as needed
}

void BoundedBuffer::push (char* msg, int size) {
    // 1. Convert the incoming byte sequence given by msg and size into a vector<char>
    // 2. Wait until there is room in the queue (i.e., queue lengh is less than cap)
    // 3. Then push the vector at the end of the queue
    // 4. Wake up threads that were waiting for push
    // 1. Convert the incoming byte sequence given by msg and size into a vector<char>
    //      use one of the vector constructors
    
    unique_lock<std::mutex> lock(mutex);
    vector<char> vec(msg, msg+size);
    
    // 2. Wait until there is room in the queue (i.e., queue length is less than cap)
    //          waiting on slot available
    /*while((int)q.size() < cap)
    {
        slot_available.wait(lock);
    }*/
    slot_available.wait (lock, [this]{ return (int)q.size() < cap; });

    // slot_available.wait(lock, [this]{if (q.size()<cap);});

    // 3. Then push the vector at the end of the queue
    q.push(vec);
    // 4. Wake up threads that were waiting for push
    //      noitifying data available
    
    lock.unlock();
    data_available.notify_one();
}

int BoundedBuffer::pop (char* msg, int size) {
    // 1. Wait until the queue has at least 1 item
    // 2. Pop the front item of the queue. The popped item is a vector<char>
    // 3. Convert the popped vector<char> into a char*, copy that into msg; assert that the vector<char>'s length is <= size
    // 4. Wake up threads that were waiting for pop
    // 5. Return the vector's length to the caller so that they know how many bytes were popped

    // 1. Wait until the queue has at least 1 item
    //      waiting on data available
    unique_lock<std::mutex> lock(mutex);

    data_available.wait (lock, [this]{ return q.size() > 0; });
    // 2. Pop the front item of the queue. The popped item is a vector<char>

    vector<char> popped_item = q.front();
    q.pop();
    // 3. Convert the popped vector<char> into a char*, copy that into msg; assert that the vector<char>'s length is <= size
    //      use vector::data()
    // msg = popped_item.data();
    
    assert((int)popped_item.size() <= size);
    
    memcpy(msg, popped_item.data(), size);
    // 4. Wake up threads that were waiting for pop
    //      notifying slot available
    lock.unlock();
    slot_available.notify_one();
    // 5. Return the vector's length to the caller so that they know how many bytes were popped
    return popped_item.size();
}

size_t BoundedBuffer::size () {
    return q.size();
}