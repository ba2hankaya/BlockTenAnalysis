#ifndef CONCURRENTQUE
#define CONCURRENTQUE

#include <mutex>
#include <condition_variable>
class ConcurrentQueue{
private:
    struct Node{
        char* p_buffer;
        int size;
        Node* next;
    };

    Node *head;
    Node *tail;

    std::mutex mtx;
    std::condition_variable cv_not_full, cv_not_empty;

    int cur_size = 0;
    int max_size;

    bool is_shutdown = false;

public:
    ConcurrentQueue(int max_size);

    void Enqueue(char* p_buffer, int size);

    int Dequeue(char*& p_buffer, int &size);

    void Shutdown();
};

#endif