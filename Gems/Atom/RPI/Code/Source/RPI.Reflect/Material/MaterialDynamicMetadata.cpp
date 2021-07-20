/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialDynamicMetadata.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace RPI
    {
        void ReflectMaterialDynamicMetadata(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Enum<MaterialPropertyVisibility>()
                    ->Value("Enabled", MaterialPropertyVisibility::Enabled)
                    ->Value("Disabled", MaterialPropertyVisibility::Disabled)
                    ->Value("Hidden", MaterialPropertyVisibility::Hidden)
                    ;
                
                serializeContext->Enum<MaterialPropertyGroupVisibility>()
                    ->Value("Enabled", MaterialPropertyGroupVisibility::Enabled)
                    ->Value("Hidden", MaterialPropertyGroupVisibility::Hidden)
                    ;
            }

            if (auto* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext
                    ->Enum<(int)MaterialPropertyVisibility::Enabled>("MaterialPropertyVisibility_Enabled")
                    ->Enum<(int)MaterialPropertyVisibility::Disabled>("MaterialPropertyVisibility_Disabled")
                    ->Enum<(int)MaterialPropertyVisibility::Hidden>("MaterialPropertyVisibility_Hidden");
                
                behaviorContext
                    ->Enum<(int)MaterialPropertyGroupVisibility::Enabled>("MaterialPropertyGroupVisibility_Enabled")
                    ->Enum<(int)MaterialPropertyGroupVisibility::Hidden>("MaterialPropertyGroupVisibility_Hidden");
            }
        }
        
    } // namespace RPI
} // namespace AZ
