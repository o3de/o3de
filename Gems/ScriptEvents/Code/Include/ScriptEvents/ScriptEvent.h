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

#pragma once


#include <AzCore/PlatformIncl.h>
#include <AzCore/Component/TickBus.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "ScriptEventsAsset.h"

#include <ScriptEvents/Internal/BehaviorContextBinding/BehaviorContextFactoryMethods.h>
#include <ScriptEvents/Internal/BehaviorContextBinding/ScriptEventBinding.h>
#include <ScriptEvents/Internal/VersionedProperty.h>

namespace ScriptEvents
{
    class Parameter;

    namespace Internal
    {
        class Utils
        {
        public:
            static void BehaviorParameterFromType(AZ::Uuid typeId, bool addressable, AZ::BehaviorParameter& outParameter);
            static void BehaviorParameterFromParameter(AZ::BehaviorContext* behaviorContext, const Parameter& parameter, const char* name, AZ::BehaviorParameter& outParameter);

            static AZ::BehaviorEBus* ConstructAndRegisterScriptEventBehaviorEBus(const ScriptEvents::ScriptEvent& definition);
            static bool DestroyScriptEventBehaviorEBus(AZStd::string_view ebusName);
        };
    }
}
