/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/Component.h>

#include <AssetBuilderSDK/AssetBuilderBusses.h>

namespace OpenParticle
{
    template<class T>
    AZStd::unique_ptr<T> MakeAssetBuilder()
    {
        static_assert(
            (AZStd::is_base_of<AssetBuilderSDK::AssetBuilderCommandBus::Handler, T>::value),
            "Can only specify desired type if it's an AssetBuilderCommandBus::Handler");

        auto assetWorker = AZStd::make_unique<T>();
        assetWorker->RegisterBuilder();
        return AZStd::move(assetWorker);
    }

    class BuilderComponent final
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BuilderComponent, "{575369ea-7f73-478e-8e0e-c5e62bcd12ac}");
        static void Reflect(AZ::ReflectContext* context);

        BuilderComponent();
        ~BuilderComponent() override;

        // AZ::Component overrides...
        void Activate() override;
        void Deactivate() override;

    private:
        BuilderComponent(const BuilderComponent&) = delete;

        using AssetWorker = AssetBuilderSDK::AssetBuilderCommandBus::Handler;
        AZStd::vector<AZStd::unique_ptr<AssetWorker>> m_assetWorkers;

        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
    };
} // namespace OpenParticle

