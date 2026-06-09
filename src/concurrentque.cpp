#include "concurrentque.hpp"

ConcurrentQueue::ConcurrentQueue(int max_size){
    head = tail = new Node();
    this->max_size = max_size;
    cur_size = 0;
}

void ConcurrentQueue::Enqueue(char* p_buffer, int size){
    std::unique_lock<std::mutex> lock(mtx);
    cv_not_full.wait(lock, [this,size]{return (cur_size+size) <= max_size;});

    Node* tmp = new Node{p_buffer, size,nullptr};
    tail->next = tmp;
    tail = tmp;
    cur_size += size;

    cv_not_empty.notify_one();
}


int ConcurrentQueue::Dequeue(char*& p_buffer, int& size){
    std::unique_lock<std::mutex> lock(mtx);

    cv_not_empty.wait(lock, [this]{return head->next != nullptr || is_shutdown;});

    if(head->next == nullptr && is_shutdown){
        return -1;
    }

    Node* tmp = head;
    Node* new_head = tmp->next;

    p_buffer = new_head->p_buffer;
    int buf_size = new_head->size;
    size = buf_size;

    head = new_head;
    cur_size -= buf_size;

    lock.unlock();
    cv_not_full.notify_one();

    delete tmp;
    return 0;
}

void ConcurrentQueue::Shutdown(){
    std::lock_guard<std::mutex> lock(mtx);
    is_shutdown = true;

    cv_not_empty.notify_all();
    cv_not_full.notify_all();
}