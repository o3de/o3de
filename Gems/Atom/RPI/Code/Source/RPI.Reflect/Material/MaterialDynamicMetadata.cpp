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
