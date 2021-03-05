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

#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Vector3.h>

namespace AZ
{
    void NonUniformScaleRequests::Reflect(ReflectContext* context)
    {
        if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->EBus<NonUniformScaleRequestBus>("NonUniformScaleRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(Script::Attributes::Category, "Entity")
                ->Attribute(Script::Attributes::Module, "entity")
                ->Event("GetScale", &NonUniformScaleRequestBus::Events::GetScale)
                ->Event("SetScale", &NonUniformScaleRequestBus::Events::SetScale)
                ;
        }
    }
} // namespace AZ
