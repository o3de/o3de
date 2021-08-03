/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/Internal/JobTypeEraser.h>

namespace AZ::Internal
{
    TypeErasedJob::TypeErasedJob(TypeErasedJob&& other) noexcept
    {
        if (!other.m_relocator || other.m_lambda != other.m_buffer)
        {
            // The type-erased lambda is trivially relocatable OR, the lambda is heap allocated
            memcpy(this, &other, sizeof(TypeErasedJob));

            if (other.m_lambda == other.m_buffer)
            {
                m_lambda = m_buffer;
            }

            // Prevent deletion in the event the lambda had spilled to the heap
            other.m_lambda = nullptr;
            return;
        }

        // At this point, we know the lambda was inlined
        m_lambda = m_buffer;

        m_invoker = other.m_invoker;
        m_relocator = other.m_relocator;
        m_destroyer = other.m_destroyer;

        // We now own the lambda, so clear the moved-from job's destroyer
        other.m_destroyer = nullptr;
        other.m_invoker = nullptr;

        m_relocator(m_buffer, other.m_buffer);
    }

    TypeErasedJob& TypeErasedJob::operator=(TypeErasedJob&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        this->~TypeErasedJob();

        new (this) TypeErasedJob{ AZStd::move(other) };

        return *this;
    }

    TypeErasedJob::~TypeErasedJob()
    {
        if (m_lambda)
        {
            if (m_destroyer)
            {
                // The presence of m_destroyer indicates that the lambda is not trivially destructible
                m_destroyer(m_lambda);
            }

            if (m_lambda != m_buffer)
            {
                // We've spilled the lambda into the heap, free its memory
                azfree(m_lambda);
            }
        }
    }

} // namespace AZ::Internal
