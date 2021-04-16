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
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>

#include <Assets/HairAsset.h>
#include <Builders/HairAssetBuilder.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            class HairBuilderComponent final
                : public Component
            {
            public:
                AZ_COMPONENT(HairBuilderComponent, "{88233F79-98DA-4DC6-A60B-0405BD810479}");
                HairBuilderComponent() = default;
                ~HairBuilderComponent() = default;

                static void Reflect(ReflectContext* context);

                static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
                static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);

            private:
                HairBuilderComponent(const HairBuilderComponent&) = delete;

                ////////////////////////////////////////////////////////////////////////
                // Component interface implementation
                void Activate() override;
                void Deactivate() override;
                ////////////////////////////////////////////////////////////////////////

                HairAssetBuilder m_hairAssetBuilder;
                HairAssetHandler m_hairAssetHandler;
            };
        } // namespace Hair
    } // End Render namespace
} // End AZ namespace
