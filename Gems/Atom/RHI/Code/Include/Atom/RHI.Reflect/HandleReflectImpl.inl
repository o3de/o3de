/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace RHI
    {
        template <typename T, typename NamespaceType>
        void Handle<T, NamespaceType>::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<Handle<T, NamespaceType>>()
                    ->Version(1)
                    ->Field("m_index", &Handle<T, NamespaceType>::m_index);
            }

            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<Handle<T, NamespaceType>>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "RHI")
                    ->Attribute(AZ::Script::Attributes::Module, "rhi")
                    ->Method("IsValid", &Handle<T, NamespaceType>::IsValid)
                    ;
            }
        }

    }
}
