/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZCORE_EXCLUDE_LUA)

#include <AzCore/Script/ScriptTimePoint.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/Component.h>

namespace AZ
{
    void ScriptTimePoint::Reflect(ReflectContext* reflection)
    {
        BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->Class<ScriptTimePoint>()->
                Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)->
                Attribute(AZ::Script::Attributes::Module, "script")->
                Method("ToString", &ScriptTimePoint::ToString)->
                Method("GetSeconds", &ScriptTimePoint::GetSeconds)->
                Method("GetMilliseconds", &ScriptTimePoint::GetMilliseconds)
            ;
        }

        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<AZStd::chrono::steady_clock::time_point>()
                ;

            serializeContext->Class<ScriptTimePoint>()
                ->Field("m_timePoint", &ScriptTimePoint::m_timePoint);
        }
    }
}

#endif // #if !defined(AZCORE_EXCLUDE_LUA)
