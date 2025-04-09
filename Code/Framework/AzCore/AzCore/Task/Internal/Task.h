/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Task/Internal/TaskConfig.h>
#include <AzCore/Task/TaskDescriptor.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/typetraits/is_assignable.h>
#include <AzCore/std/typetraits/is_destructible.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ::Internal
{
    using TaskInvoke_t = void (*)(void* lambda);
    using TaskRelocate_t = void (*)(void* dst, void* src);
    using TaskDestroy_t = void (*)(void* obj);

    class CompiledTaskGraph;

    // Lambdas are opaque types and we cannot extract any member function pointers. In order to store lambdas in a
    // type erased fashion, we instead use a single function call indirection, invoking the lambda function in a
    // static class function which has a stable address in memory. The Erased* methods return addresses to the
    // indirect callers of the lambda copy/move assignment operators, call operator, and destructor.
    //
    // For lambdas that are trivially relocatable, both the returned move and copy assignment function pointers
    // will be nullptr.
    //
    // Lambdas that are trivially destructible will result in a nullptr returned TaskDestroy_t pointer.
    //
    // The class will check that the lambda is copy assignable or movable.
    template<typename Lambda>
    class TaskTypeEraser final
    {
    public:
        constexpr TaskInvoke_t ErasedInvoker()
        {
            return reinterpret_cast<TaskInvoke_t>(Invoker);
        }

        constexpr TaskRelocate_t ErasedRelocator()
        {
            if constexpr (AZStd::is_trivially_move_constructible_v<Lambda>)
            {
                return nullptr;
            }
            else if constexpr (AZStd::is_move_constructible_v<Lambda>)
            {
                return reinterpret_cast<TaskRelocate_t>(Mover);
            }
            else if constexpr (AZStd::is_copy_constructible_v<Lambda>)
            {
                return reinterpret_cast<TaskRelocate_t>(Copier);
            }
            else
            {
                static_assert(
                    AZStd::is_move_constructible_v<Lambda> || AZStd::is_copy_constructible_v<Lambda>,
                    "Task lambdas must be either move or copy constructible. Please verify that all captured data is move or copy "
                    "constructible.");
            }
        }

        constexpr TaskDestroy_t ErasedDestroyer()
        {
            if constexpr (AZStd::is_trivially_destructible_v<Lambda>)
            {
                return nullptr;
            }
            else
            {
                return reinterpret_cast<TaskDestroy_t>(Destroyer);
            }
        }

    private:
        constexpr static void Invoker(Lambda* lambda)
        {
            lambda->operator()();
        }

        constexpr static void Mover(Lambda* dst, Lambda* src)
        {
            new (dst) Lambda{ AZStd::move(*src) };
        }

        constexpr static void Copier(Lambda* dst, Lambda* src)
        {
            new (dst) Lambda{ *src };
        }

        constexpr static void Destroyer(Lambda* lambda)
        {
            lambda->~Lambda();
        }
    };

    // The Task encapsulates member function pointers to store in a homogeneously-typed container
    // The function signature of all lambdas encoded in a Task is void(*)(). The lambdas can capture
    // data, in which case the data is inlined in this structure. Attempting to capture more data
    // will result in a compile failure, so use indirection and capture a pointer/reference to your
    // data if you run into this.
    class alignas(alignof(max_align_t)) Task final
    {
    public:
        AZ_CLASS_ALLOCATOR(Task, ThreadPoolAllocator);

        // The inline buffer allows the Task to span two cache lines. Lambdas can capture 56
        // bytes of data (7 pointers/references on a 64-bit machine).
        constexpr static size_t BufferSize =
            AZ_TRAIT_TASK_BYTE_SIZE - sizeof(size_t) * 5 - sizeof(uint32_t) - sizeof(TaskDescriptor) - sizeof(AZStd::atomic<uint32_t>);

        Task() = default;

        // Prevent binding lvalue references to lambdas
        // If you are encountering a compiler error here, please either move the lambda into the AddJob function with AZStd::move
        // or simply define the lambda directly as a parameter of AddJob
        template<typename Lambda>
        Task(TaskDescriptor const& desc, Lambda& lambda) = delete;

        template<typename Lambda>
        Task(TaskDescriptor const& desc, Lambda&& lambda) noexcept;

        Task(Task&& other) noexcept;

        Task& operator=(Task&& other) noexcept;

        ~Task();

        void Link(Task& other);

        // Indicates if this task is a root of the graph (with no dependencies)
        bool IsRoot() const noexcept;

        // Prepare for dispatch (reset the dependency counter to the number of inbound edges)
        void Init() noexcept;

        // Invoke the embedded lambda function
        void Invoke();

        uint8_t GetPriorityNumber() const noexcept;

    private:
        friend class CompiledTaskGraph;
        friend class TaskWorker;

        // This relocation avoids branches needed if the lambda type is unknown
        template<typename Lambda>
        void TypedRelocate(Lambda&& lambda, char* destination);

        // Small buffer optimization for lambdas. We cover our bases here by enforcing alignment on the
        // class to equal the alignment of the largest scalar type available on the system (generally
        // 16 bytes).
        char m_lambda[BufferSize];
        AZStd::atomic<uint32_t> m_dependencyCount;

        // This value is an offset in a buffer that stores dependency tracking information.
        uint32_t m_successorOffset = 0;
        uint32_t m_inboundLinkCount = 0;
        uint32_t m_outboundLinkCount = 0;

        CompiledTaskGraph* m_graph = nullptr;

        TaskInvoke_t m_invoker;

        // If nullptr, the lambda is trivially relocatable (via memcpy). Otherwise, it must be invoked
        // when instances of this class are moved.
        TaskRelocate_t m_relocator;
        TaskDestroy_t m_destroyer;

        TaskDescriptor m_descriptor;
    };
} // namespace AZ::Internal

#include <AzCore/Task/Internal/Task.inl>
