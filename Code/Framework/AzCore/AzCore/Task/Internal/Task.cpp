/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Task/Internal/Task.h>

namespace AZ::Internal
{
    Task::Task(Task&& other) noexcept
    {
        if (!other.m_relocator)
        {
            // The type-erased lambda is trivially relocatable OR, the lambda is heap allocated
            memcpy(this, &other, sizeof(Task));

            // Prevent deletion in the event the lambda had spilled to the heap
            other.m_destroyer = nullptr;
            return;
        }

        m_invoker = other.m_invoker;
        m_relocator = other.m_relocator;
        m_destroyer = other.m_destroyer;

        // We now own the lambda, so clear the moved-from task's destroyer
        other.m_destroyer = nullptr;

        m_relocator(m_lambda, other.m_lambda);
    }

    Task& Task::operator=(Task&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        this->~Task();

        new (this) Task{ AZStd::move(other) };

        return *this;
    }

    Task::~Task()
    {
        if (m_destroyer)
        {
            // The presence of m_destroyer indicates that the lambda is not trivially destructible
            m_destroyer(m_lambda);
        }
    }

} // namespace AZ::Internal
