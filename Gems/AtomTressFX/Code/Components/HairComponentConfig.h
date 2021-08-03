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

#include <AzCore/Component/Component.h>
#include <Assets/HairAsset.h>
#include <TressFX/TressFXSettings.h>
#include <Rendering/HairGlobalSettings.h>

// #include <Atom/Feature/ParamMacros/ParamMacrosHowTo.inl>    // for education purposes
//#include <Components/HairSettingsInterface.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            class EditorHairComponent;

            // The component config file containing the editor settings / configuration data of the current hair.
            class HairComponentConfig final :
                public ComponentConfig
            {
                friend class EditorHairComponent;

            public:
                AZ_RTTI(AZ::Render::HairComponentConfig, "{AF2C2F26-0C01-4EAD-A81C-4304BD751EDF}", AZ::ComponentConfig);

                static void Reflect(ReflectContext* context);

                // HairComponentConfigInterface overrides...
                void SetEnabled(bool value)
                {
                    m_enabled = value;
                }
                bool GetIsEnabled()
                {
                    return m_enabled;
                }

                // TressFX settings
                AMD::TressFXSimulationSettings m_simulationSettings;
                AMD::TressFXRenderingSettings m_renderingSettings;

                HairGlobalSettings m_hairGlobalSettings;

            private:
                bool m_enabled = false;
            };

        } // namespace Hair
    } // namespace Render
} // namespace AZ
