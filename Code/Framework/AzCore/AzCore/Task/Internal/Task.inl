/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ::Internal
{
    template<typename Lambda>
    Task::Task(TaskDescriptor const& desc, Lambda&& lambda) noexcept
        : m_descriptor{ desc }
    {
        static_assert(
            sizeof(Lambda) <= BufferSize,
            "Task lambda has too much captured data, please capture no"
            "more than 56 bytes of data (likely by capturing a single reference/pointer to a container of data)");
        static_assert(
            alignof(Lambda) <= alignof(max_align_t),
            "Task lambda has extended alignment which isn't supported."
            "Please capture a reference/pointer to the data requiring an extended alignment instead");

        TaskTypeEraser<Lambda> eraser;
        m_invoker = eraser.ErasedInvoker();
        m_relocator = eraser.ErasedRelocator();
        m_destroyer = eraser.ErasedDestroyer();

        // NOTE: This code is conservative in that extended alignment requirements result in a heap
        // spill, even if the lambda could have occupied a portion of the inline buffer with a base
        // pointer adjustment.
        TypedRelocate(AZStd::forward<Lambda>(lambda), m_lambda);
    }

    template<typename Lambda>
    void Task::TypedRelocate(Lambda&& lambda, char* destination)
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
                AZStd::is_move_constructible_v<Lambda> || AZStd::is_copy_constructible_v<Lambda>,
                "Task lambdas must be either move or copy constructible. Please verify that all captured data is move or copy "
                "constructible.");
        }
    }

    inline void Task::Init() noexcept
    {
        m_dependencyCount = m_inboundLinkCount;
    }

    inline void Task::Invoke()
    {
        m_invoker(m_lambda);
    }

    inline uint8_t Task::GetPriorityNumber() const noexcept
    {
        return static_cast<uint8_t>(m_descriptor.priority);
    }

    inline void Task::Link(Task& other)
    {
        ++m_outboundLinkCount;
        ++other.m_inboundLinkCount;
    }

    inline bool Task::IsRoot() const noexcept
    {
        return m_inboundLinkCount == 0;
    }
} // namespace AZ::Internal
