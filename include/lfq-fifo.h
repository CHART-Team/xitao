/* BSD 3-Clause License

* Copyright (c) 2019-2021, contributors
* All rights reserved.

* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:

* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.

* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.

* 3. Neither the name of the copyright holder nor the names of its
*    contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.

* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


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
#include <iostream>
using namespace std;

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
    
    T& front() {
        LFQNode* nxt = first->next;
        if( nxt != nullptr ) { 
            return nxt->value;
        } else {
            return first->value;
        }
    }

    bool empty() {
        return ( first->next == nullptr );
    }
    void pop() {
      if( first->next != nullptr ) {  // if queue is nonempty 
            LFQNode* oldFirst = first;
            first = first->next;
            first->value = nullptr;     // of the Node
            delete oldFirst;            // both allocations
        }
    }

    bool pop_front( T& result ) {
        if( first->next != nullptr ) {  // if queue is nonempty 
            LFQNode* oldFirst = first;
            first = first->next;
            T value = first->value;     // take it out
            first->value = nullptr;     // of the Node
            result = value;            // now copy it back
            delete oldFirst;            // both allocations
            return true;                // and report success
        }
        return false;                   // queue was empty
    }

    bool push( T t )  {
        LFQNode* tmp = new LFQNode( t );    // do work off to the side
        last->next = tmp;               // A: publish the new item
        last = tmp;                     // B: not "last->next"
        return true;
    }
};
#endif