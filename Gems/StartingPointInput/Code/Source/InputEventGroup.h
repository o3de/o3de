/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>

#include <InputEventMap.h>

namespace AZ
{
    class ReflectContext;
}

namespace StartingPointInput
{
    //////////////////////////////////////////////////////////////////////////
    /// This class holds all of the raw input handlers that generate events.
    //////////////////////////////////////////////////////////////////////////
    class InputEventGroup
    {
    public:
        AZ_RTTI(InputEventGroup, "{25143B7E-2FEC-4CC5-92FE-270B67E79734}");
        virtual ~InputEventGroup() = default;

        static void Reflect(AZ::ReflectContext* reflection);

        virtual void Activate(const AzFramework::LocalUserId& localUserId)
        {
        #if defined(RELEASE)
            if (m_excludeFromRelease)
            {
                return;
            }
        #endif // defined(RELEASE)

            InputEventNotificationId busId(localUserId, m_eventName.c_str());
            for (InputSubComponent* inputHandler : m_inputHandlers)
            {
                inputHandler->Activate(busId);
            }
        }
        virtual void Deactivate(const AzFramework::LocalUserId& localUserId)
        {
        #if defined(RELEASE)
            if (m_excludeFromRelease)
            {
                return;
            }
        #endif // defined(RELEASE)

            InputEventNotificationId busId(localUserId, m_eventName.c_str());
            for (InputSubComponent* inputHandler : m_inputHandlers)
            {
                inputHandler->Deactivate(busId);
            }
        }

        // Explicitly release our array of input handlers here.  There is no system that is currently cleaning up the Input objects we have in this array
        // We cannot do this in the destructor because of the allocation patterns of this object in the serializer that causes us to end up releasing invalid data during serialization load
        // I did not have success changing the raw pointer array of input handlers to a shared pointer of InputSubComponents.
        // The most straight forward resolution for now is to be explicit about when we release this data in order to prevent large memory leaks
        void Cleanup()
        {
            // Release the InputSubComponents opposite of how they were allocated during serialization in InstanceFactory::Create
            for (InputSubComponent* inputHandler : m_inputHandlers)
            {
                inputHandler->~InputSubComponent();
                azfree(static_cast<void*>(inputHandler), AZ::SystemAllocator);
            }
            m_inputHandlers.clear();
        }

    protected:
        virtual AZStd::string GetEditorText() const
        {
            return m_eventName.empty() ? "<Unspecified Event>" : m_eventName;
        }

        AZStd::vector<InputSubComponent*> m_inputHandlers;
        AZStd::string m_eventName;
        bool m_excludeFromRelease = false;
    };
} // namespace Input
