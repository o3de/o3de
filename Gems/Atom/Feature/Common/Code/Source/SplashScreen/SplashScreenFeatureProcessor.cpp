/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SplashScreen/SplashScreenFeatureProcessor.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>

namespace AZ::Render
{
    void SplashScreenFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext
                ->Class<SplashScreenFeatureProcessor, FeatureProcessor>()
                ->Version(1);
        }
    }

    void SplashScreenFeatureProcessor::AddRenderPasses(RPI::RenderPipeline* renderPipeline)
    {
        AZ::ApplicationTypeQuery appType;
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
        if (!appType.IsGame())
        {
            return;
        }

        bool enable = false;

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        static const AZStd::string setregPath = "/O3DE/Atom/Feature/SplashScreenEnable";

        if (settingsRegistry && settingsRegistry->GetObject(&enable, azrtti_typeid(enable), setregPath.c_str()))
        {
            if (enable)
            {
                AddPassRequestToRenderPipeline(renderPipeline, "Passes/SplashScreenPassRequest.azasset", "CopyToSwapChain", true);
            }
        }
    }
} // namespace AZ::Render
