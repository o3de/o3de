/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
