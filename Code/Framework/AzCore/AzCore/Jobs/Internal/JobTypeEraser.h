/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Jobs/JobDescriptor.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/typetraits/is_assignable.h>
#include <AzCore/std/typetraits/is_destructible.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ::Internal
{
    using JobInvoke_t = void (*)(void* lambda);
    using JobRelocate_t = void (*)(void* dst, void* src);
    using JobDestroy_t = void (*)(void* obj);

    class CompiledJobGraph;

    // Lambdas are opaque types and we cannot extract any member function pointers. In order to store lambdas in a
    // type erased fashion, we instead use a single function call indirection, invoking the lambda function in a
    // static class function which has a stable address in memory. The Erased* methods return addresses to the
    // indirect callers of the lambda copy/move assignment operators, call operator, and destructor.
    //
    // For lambdas that are trivially relocatable, both the returned move and copy assignment function pointers
    // will be nullptr.
    //
    // Lambdas that are trivially destructible will result in a nullptr returned JobDestroy_t pointer.
    //
    // The class will check that the lambda is copy assignable or movable.
    template<typename Lambda>
    class JobTypeEraser final
    {
    public:
        constexpr JobInvoke_t ErasedInvoker()
        {
            return reinterpret_cast<JobInvoke_t>(Invoker);
        }

        constexpr JobRelocate_t ErasedRelocator()
        {
            if constexpr (AZStd::is_trivially_move_constructible_v<Lambda>)
            {
                return nullptr;
            }
            else if constexpr (AZStd::is_move_constructible_v<Lambda>)
            {
                return reinterpret_cast<JobRelocate_t>(Mover);
            }
            else if constexpr (AZStd::is_copy_constructible_v<Lambda>)
            {
                return reinterpret_cast<JobRelocate_t>(Copyer);
            }
            else
            {
                static_assert(
                    false,
                    "Job lambdas must be either move or copy constructible. Please verify that all captured data is move or copy "
                    "constructible.");
            }
        }

        constexpr JobDestroy_t ErasedDestroyer()
        {
            if constexpr (AZStd::is_trivially_destructible_v<Lambda>)
            {
                return nullptr;
            }
            else
            {
                return reinterpret_cast<JobDestroy_t>(Destroyer);
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

        constexpr static void Copyer(Lambda* dst, Lambda* src)
        {
            new (dst) Lambda{ *src };
        }

        constexpr static void Destroyer(Lambda* lambda)
        {
            lambda->~Lambda();
        }
    };

    // The TypeErasedJob encapsulates member function pointers to store in a homogeneously-typed container
    // The function signature of all lambdas encoded in a TypeErasedJob is void(*)(). The lambdas can capture
    // data, in which case the data is inlined in this structure if the payload is less than or equal to the
    // buffer size. Otherwise, the data is heap allocated.
    class alignas(alignof(max_align_t)) TypeErasedJob final
    {
    public:
        // The inline buffer allows the TypeErasedJob to span two cache lines. Lambdas can capture 48
        // bytes of data (6 pointers/references on a 64-bit machine) before spilling to the heap.
        constexpr static size_t BufferSize =
            128 - sizeof(size_t) * 6 - sizeof(uint32_t) - sizeof(JobDescriptor) - sizeof(AZStd::atomic<uint32_t>);

        TypeErasedJob() = default;

        template<typename Lambda>
        TypeErasedJob(JobDescriptor const& desc, Lambda&& lambda) noexcept
            : m_descriptor{ desc }
        {
            JobTypeEraser<Lambda> eraser;
            m_invoker = eraser.ErasedInvoker();
            m_relocator = eraser.ErasedRelocator();
            m_destroyer = eraser.ErasedDestroyer();

            // NOTE: This code is conservative in that extended alignment requirements result in a heap
            // spill, even if the lambda could have occupied a portion of the inline buffer with a base
            // pointer adjustment.
            if constexpr (sizeof(Lambda) <= BufferSize && alignof(Lambda) <= alignof(max_align_t))
            {
                TypedRelocate(AZStd::forward<Lambda>(lambda), m_buffer);
                m_lambda = m_buffer;
            }
            else
            {
                // Lambda has spilled to the heap (or requires extended alignment)
                m_lambda = reinterpret_cast<char*>(azmalloc(sizeof(Lambda), alignof(Lambda)));
                TypedRelocate(AZStd::forward<Lambda>(lambda), m_lambda);
            }
        }

        TypeErasedJob(TypeErasedJob&& other) noexcept;

        TypeErasedJob& operator=(TypeErasedJob&& other) noexcept;

        ~TypeErasedJob();

        void Link(TypeErasedJob& other);

        // Indicates if this job is a root of the graph (with no dependencies)
        bool IsRoot();

        void Init() noexcept
        {
            m_dependencyCount = m_inboundLinkCount;
        }

        void Invoke()
        {
            m_invoker(m_lambda);
        }

        uint8_t GetPriorityNumber() const
        {
            return static_cast<uint8_t>(m_descriptor.priority);
        }

    private:
        friend class CompiledJobGraph;
        friend class JobWorker;

        // This relocation avoids branches needed if the lambda type is unknown
        template<typename Lambda>
        void TypedRelocate(Lambda&& lambda, char* destination)
        {
            if constexpr (AZStd::is_trivially_move_constructible_v<Lambda>)
            {
                memcpy(destination, reinterpret_cast<char*>(&lambda), sizeof(Lambda));
            }
            else if constexpr (AZStd::is_move_constructible_v<Lambda>)
            {
                new (destination) Lambda{ AZStd::move(lambda) };
            }
            else if constexpr (AZStd::is_copy_constructible_v<Lambda>)
            {
                new (destination) Lambda{ lambda };
            }
            else
            {
                static_assert(
                    false,
                    "Job lambdas must be either move or copy constructible. Please verify that all captured data is move or copy "
                    "constructible.");
            }
        }

        // Small buffer optimization for lambdas. We cover our bases here by enforcing alignment on the
        // class to equal the alignment of the largest scalar type available on the system (generally
        // 16 bytes).
        char m_buffer[BufferSize];
        AZStd::atomic<uint32_t> m_dependencyCount;

        // This value is an offset in a buffer that stores dependency tracking information.
        uint32_t m_successorOffset = 0;
        uint32_t m_inboundLinkCount = 0;
        uint32_t m_outboundLinkCount = 0;

        // May point to the inlined payload buffer, or heap
        char* m_lambda = nullptr;

        CompiledJobGraph* m_graph = nullptr;

        JobInvoke_t m_invoker;

        // If nullptr, the lambda is trivially relocatable (via memcpy). Otherwise, it must be invoked
        // when instances of this class are moved.
        JobRelocate_t m_relocator;
        JobDestroy_t m_destroyer;

        JobDescriptor m_descriptor;
    };

    inline void TypeErasedJob::Link(TypeErasedJob& other)
    {
        ++m_outboundLinkCount;
        ++other.m_inboundLinkCount;
    }

    inline bool TypeErasedJob::IsRoot()
    {
        return m_inboundLinkCount == 0;
    }
} // namespace AZ::Internal
