// lf-fifo.h -- a lock-free FIFO queue
// This code is adapted from
// http://stackoverflow.com/questions/2945312/optimistic-lock-free-fifo-queues-impl
// which is in turn adapted from Herb Sutter

// Changes:
//  - changed alignment buffers to GCC attributes
//  - reverted changes in c++ naming 
//  - changed references to pointers 
//  - removed locks that ensure single producer/consumer. This is already enforced by the GO:TAO design
#ifndef LFQ
#define LFQ
#include <atomic>

#define CACHE_LINE_SIZE (64)

template <typename T> class LFQueue {
private:
    struct LFQNode {
        LFQNode( T val ) : value(val), next(nullptr) { }
        T value;
        std::atomic<LFQNode *> next;
    } __attribute__ ((aligned(CACHE_LINE_SIZE)));

    LFQNode* first __attribute__ ((aligned(CACHE_LINE_SIZE)));    
    LFQNode* last __attribute__ ((aligned(CACHE_LINE_SIZE)));     
    char pad __attribute__(( aligned ( CACHE_LINE_SIZE)));        // just in case  
public:
    LFQueue() {
        first = last = new LFQNode( nullptr ); // no more divider
    }

    ~LFQueue() {
        while( first != nullptr ) {
            LFQNode* tmp = first;
            first = tmp->next;
            delete tmp;
        }
    }
    
    bool pop_front( T* result ) {
        if( first->next != nullptr ) {  // if queue is nonempty 
            LFQNode* oldFirst = first;
            first = first->next;
            T value = first->value;     // take it out
            first->value = nullptr;     // of the Node
            *result = value;            // now copy it back
            delete oldFirst;            // both allocations
            return true;                // and report success
        }
        return false;                   // queue was empty
    }

    bool push_back( T t )  {
        LFQNode* tmp = new LFQNode( t );    // do work off to the side
        last->next = tmp;               // A: publish the new item
        last = tmp;                     // B: not "last->next"
        return true;
    }
};
#endif