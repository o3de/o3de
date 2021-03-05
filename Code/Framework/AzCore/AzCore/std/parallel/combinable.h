/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef AZSTD_PARALLEL_COMBINABLE_H
#define AZSTD_PARALLEL_COMBINABLE_H 1

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/thread.h>

namespace AZStd
{
    /**
     * Provides a thread-local value which can then be combined into a single value when all work is complete.
     */
    template<typename T, class Allocator = AZStd::allocator>
    class combinable
    {
    public:
        combinable()
            : m_initFunc(&DefaultInitializer)
        {
            for (int i = 0; i < NUM_BUCKETS; ++i)
            {
                m_buckets[i].store(NULL, memory_order_release);
            }
        }

        template<typename F>
        combinable(F initFunc)
            : m_initFunc(initFunc)
        {
            for (int i = 0; i < NUM_BUCKETS; ++i)
            {
                m_buckets[i].store(NULL, memory_order_release);
            }
        }

        combinable(const combinable& other)
            : m_initFunc(other.m_initFunc)
        {
            Copy(other);
        }

        ~combinable()
        {
            clear();
        }

        combinable& operator=(const combinable& other)
        {
            clear();
            m_initFunc = other.m_initFunc;
            Copy(other);
            return *this;
        }

        void clear()
        {
            for (int i = 0; i < NUM_BUCKETS; ++i)
            {
                Node* node = m_buckets[i].load(memory_order_acquire);
                while (node)
                {
                    Node* next = node->m_next;
                    m_allocator.deallocate(node, sizeof(Node), alignment_of<Node>::value);
                    node = next;
                }
                m_buckets[i].store(NULL, memory_order_release);
            }
        }

        T& local()
        {
            thread::id threadId = this_thread::get_id();
            Node* node = FindNode(threadId);
            if (!node)
            {
                node = AddNode(threadId);
            }
            return node->m_value;
        }

        T& local(bool& exists)
        {
            thread::id threadId = this_thread::get_id();
            Node* node = FindNode(threadId);
            if (node)
            {
                exists = true;
            }
            else
            {
                node = AddNode(threadId);
                exists = false;
            }
            return node->m_value;
        }

        template<typename F>
        T combine(F f)
        {
            Node* currentNode = NULL;
            int currentNodeBucket = 0;

            for (int i = 0; i < NUM_BUCKETS; ++i)
            {
                Node* node = m_buckets[i].load(memory_order_acquire);
                if (node)
                {
                    currentNode = node;
                    currentNodeBucket = i;
                    break;
                }
            }

            if (!currentNode)
            {
                return m_initFunc();
            }

            T result = currentNode->m_value;

            currentNode = currentNode->m_next;
            while (currentNodeBucket < NUM_BUCKETS)
            {
                while (currentNode)
                {
                    result = f(result, currentNode->m_value);
                    currentNode = currentNode->m_next;
                }

                ++currentNodeBucket;
                if (currentNodeBucket < NUM_BUCKETS)
                {
                    currentNode = m_buckets[currentNodeBucket].load(memory_order_acquire);
                }
            }

            return result;
        }

        template<typename F>
        void combine_each(F f)
        {
            for (int i = 0; i < NUM_BUCKETS; ++i)
            {
                Node* node = m_buckets[i].load(memory_order_acquire);
                while (node)
                {
                    f(node->m_value);
                    node = node->m_next;
                }
            }
        }

    private:
        //alternative implementation could use actual TLS, but it's not available on all platforms, we don't have a
        //proper dynamic TLS variable pool yet, and this is the same as the MS implementation anyway so it will be good
        //enough.
        struct Node
        {
            AZ_ALIGN(T m_value, 64); //alignment to avoid cache line sharing
            thread::id m_threadId;
            Node* m_next;
        };
        enum
        {
            NUM_BUCKETS = 8
        };

        static T DefaultInitializer()
        {
            return T();
        }

        void Copy(const combinable& other)
        {
            for (int i = 0; i < NUM_BUCKETS; ++i)
            {
                m_buckets[i].store(NULL, memory_order_release);

                Node* otherNode = other.m_buckets[i].load(memory_order_acquire);
                while (otherNode)
                {
                    Node* newNode = static_cast<Node*>(m_allocator.allocate(sizeof(Node), alignment_of<Node>::value));
                    newNode->m_threadId = otherNode->m_threadId;
                    newNode->m_value = otherNode->m_value;
                    newNode->m_next = m_buckets[i].load(memory_order_acquire);
                    m_buckets[i].store(newNode, memory_order_release);

                    otherNode = otherNode->m_next;
                }
            }
        }

        Node* FindNode(thread_id threadId)
        {
            size_t bucketIndex = (size_t)(threadId.m_id) % NUM_BUCKETS;
            Node* node = m_buckets[bucketIndex].load(memory_order_acquire);
            while (node)
            {
                if (node->m_threadId == threadId)
                {
                    return node;
                }
                node = node->m_next;
            }
            return NULL;
        }

        Node* AddNode(thread_id threadId)
        {
            size_t bucketIndex = (size_t)(threadId.m_id) % NUM_BUCKETS;

            Node* newNode = static_cast<Node*>(m_allocator.allocate(sizeof(Node), alignment_of<Node>::value));
            newNode->m_threadId = threadId;
            newNode->m_value = m_initFunc();

            Node* oldHead = m_buckets[bucketIndex].load(memory_order_acquire);
            newNode->m_next = oldHead;
            while (!m_buckets[bucketIndex].compare_exchange_weak(oldHead, newNode, memory_order_acq_rel, memory_order_acquire))
            {
                newNode->m_next = oldHead;
            }

            return newNode;
        }

        atomic<Node*> m_buckets[NUM_BUCKETS];
        Allocator m_allocator;
        AZStd::function<T ()> m_initFunc;
    };
}

#endif
#pragma once
