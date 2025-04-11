/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Object.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/std/parallel/atomic.h>

namespace AZ::RHI
{
    void Object::SetName(const Name& name)
    {
        m_name = name;
        SetNameInternal(m_name.GetStringView());
    }

    void Object::SetNameInternal(const AZStd::string_view& name)
    {
        (void)name;
    }

    const Name& Object::GetName() const
    {
        return m_name;
    }

    void Object::add_ref() const
    {
        AZ_Assert(m_useCount >= 0, "m_useCount is negative");
        if (m_useCount < 0)
        {
            // We output to the debugger here because if we're in this situation, then there is a good chance the application is going
            // to crash before the output from the assert is flushed anywhere.
            Debug::Platform::OutputToDebugger(
                "",
                AZStd::string::format("Assert: %s:%d (%s): m_useCount is negative. RHI::Object is managed by intrusive_ptr, so this means some system is "
                "caching a raw pointer somewhere but not accounting for thread saftey.\n",
                __FILE__,
                __LINE__,
                AZ_FUNCTION_SIGNATURE));
        }
        ++m_useCount;
    }

    void Object::release() const
    {
        [[maybe_unused]] int useCount = --m_useCount;
        AZ_Assert(useCount >= 0, "Releasing an already released object");

        int expectedRefCount = 0;
        if (m_useCount.compare_exchange_strong(expectedRefCount, -1))
        {
            // Get a mutable pointer to shutdown the object before deleting
            Object* object = const_cast<Object*>(this);
            object->Shutdown();
            delete object;
        }
    }
}
