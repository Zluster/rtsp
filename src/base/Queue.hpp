#pragma once
#include <deque>
#include <memory_resource>
#include <atomic>
#include <memory>
#include <cstddef>

namespace base
{
    template <typename T>
    class LockFreeQueue
    {
    private:
        struct Node
        {
            alignas(64) std::atomic<Node *> next{nullptr};
            T data;

            Node() = default;
            explicit Node(const T &item) : data(item) {}
            explicit Node(T &&item) : data(std::move(item)) {}
        };

    public:
        LockFreeQueue();
        ~LockFreeQueue();

        LockFreeQueue(const LockFreeQueue &) = delete;
        LockFreeQueue &operator=(const LockFreeQueue &) = delete;

        LockFreeQueue(LockFreeQueue &&other) noexcept;
        LockFreeQueue &operator=(LockFreeQueue &&other) noexcept;

        bool enqueue(T &&item);
        bool enqueue(const T &item);
        bool dequeue(T &item);
        bool empty() const;
        size_t size() const;

    private:
        std::atomic<Node *> head_;
        std::atomic<Node *> tail_;
    };

    // 模板实现移到头文件中
    template <typename T>
    LockFreeQueue<T>::LockFreeQueue()
    {
        Node *dummy = new Node();
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }

    template <typename T>
    LockFreeQueue<T>::~LockFreeQueue()
    {
        while (Node *node = head_.load(std::memory_order_relaxed))
        {
            head_.store(node->next.load(std::memory_order_relaxed), std::memory_order_relaxed);
            delete node;
        }
    }

    template <typename T>
    LockFreeQueue<T>::LockFreeQueue(LockFreeQueue &&other) noexcept
        : head_(other.head_.load(std::memory_order_relaxed)),
          tail_(other.tail_.load(std::memory_order_relaxed))
    {
        other.head_.store(nullptr, std::memory_order_relaxed);
        other.tail_.store(nullptr, std::memory_order_relaxed);
    }

    template <typename T>
    LockFreeQueue<T> &LockFreeQueue<T>::operator=(LockFreeQueue &&other) noexcept
    {
        if (this != &other)
        {
            while (Node *node = head_.load(std::memory_order_relaxed))
            {
                head_.store(node->next.load(std::memory_order_relaxed), std::memory_order_relaxed);
                delete node;
            }
            head_.store(other.head_.load(std::memory_order_relaxed), std::memory_order_relaxed);
            tail_.store(other.tail_.load(std::memory_order_relaxed), std::memory_order_relaxed);
            other.head_.store(nullptr, std::memory_order_relaxed);
            other.tail_.store(nullptr, std::memory_order_relaxed);
        }
        return *this;
    }

    template <typename T>
    bool LockFreeQueue<T>::enqueue(T &&item)
    {
        Node *node = new Node(std::forward<T>(item));
        Node *prev = tail_.exchange(node, std::memory_order_acq_rel);
        prev->next.store(node, std::memory_order_release);
        return true;
    }

    template <typename T>
    bool LockFreeQueue<T>::enqueue(const T &item)
    {
        Node *node = new Node(item);
        Node *prev = tail_.exchange(node, std::memory_order_acq_rel);
        prev->next.store(node, std::memory_order_release);
        return true;
    }

    template <typename T>
    bool LockFreeQueue<T>::dequeue(T &item)
    {
        Node *head = head_.load(std::memory_order_relaxed);
        Node *next = head->next.load(std::memory_order_acquire);
        if (next == nullptr)
        {
            return false;
        }
        item = std::move(next->data);
        head_.store(next, std::memory_order_release);
        delete head;
        return true;
    }

    template <typename T>
    bool LockFreeQueue<T>::empty() const
    {
        Node *head = head_.load(std::memory_order_relaxed);
        Node *next = head->next.load(std::memory_order_acquire);
        return next == nullptr;
    }

    template <typename T>
    size_t LockFreeQueue<T>::size() const
    {
        size_t count = 0;
        Node *head = head_.load(std::memory_order_relaxed);
        Node *current = head->next.load(std::memory_order_acquire);

        while (current != nullptr)
        {
            ++count;
            current = current->next.load(std::memory_order_acquire);
        }

        return count;
    }
} // namespace base
