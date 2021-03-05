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

#include "Vegetation_precompiled.h"
#include <Vegetation/EmptyInstanceSpawner.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Vegetation/Ebuses/DescriptorNotificationBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <Vegetation/InstanceData.h>

namespace Vegetation
{

    void EmptyInstanceSpawner::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<EmptyInstanceSpawner, InstanceSpawner>()
                ->Version(1)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<EmptyInstanceSpawner>(
                    "Empty Space", "Empty Space Instance")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<EmptyInstanceSpawner>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "vegetation")
                ->Constructor()
                // Dummy method needed for Python binding system to correctly register the type
                ->Method("IsEmpty", [](EmptyInstanceSpawner*) -> bool { return true; } )
                ;
        }
    }

    bool EmptyInstanceSpawner::DataIsEquivalent(const InstanceSpawner & baseRhs) const
    {
        if (const auto* rhs = azrtti_cast<const EmptyInstanceSpawner*>(&baseRhs))
        {
            return true;
        }

        // Not the same subtypes, so definitely not a data match.
        return false;
    }

} // namespace Vegetation
