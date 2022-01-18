/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/InputDeviceId.h>

#include <AzCore/RTTI/BehaviorContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceId::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<InputDeviceId>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "Input")
                ->Constructor<const char*>()
                ->Constructor<const char*, AZ::u32>()
                ->Property("name", [](InputDeviceId* thisPtr) { return thisPtr->GetName(); }, nullptr)
                ->Property("index", BehaviorValueProperty(&InputDeviceId::m_index))
            ;
        }
    }
} // namespace AzFramework
