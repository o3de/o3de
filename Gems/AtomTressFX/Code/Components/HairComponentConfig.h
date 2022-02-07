/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <TressFX/TressFXSettings.h>

#include <Assets/HairAsset.h>
#include <Rendering/HairGlobalSettings.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            class EditorHairComponent;

            //! Reflects the TressFX settings and configuration data of the current hair object.
            class HairComponentConfig final :
                public ComponentConfig
            {
                friend class EditorHairComponent;

            public:
                AZ_RTTI(AZ::Render::HairComponentConfig, "{AF2C2F26-0C01-4EAD-A81C-4304BD751EDF}", AZ::ComponentConfig);

                static void Reflect(ReflectContext* context);

                void OnHairGlobalSettingsChanged();

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

                Data::Asset<HairAsset> m_hairAsset;
                HairGlobalSettings m_hairGlobalSettings;

            private:
                bool m_enabled = true;
            };

        } // namespace Hair
    } // namespace Render
} // namespace AZ
