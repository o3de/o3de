/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataRegistration.h>

namespace AZ
{
    namespace RPI
    {
        /**
         * This is a helper function for creating an asset builder unique_ptr instance and registering it.
         */
        template<class T>
        AZStd::unique_ptr<T> MakeAssetBuilder()
        {
            static_assert((AZStd::is_base_of<AssetBuilderSDK::AssetBuilderCommandBus::Handler, T>::value),
                "Can only specify desired type if it's an AssetBuilderCommandBus::Handler");

            auto assetWorker = AZStd::make_unique<T>();
            assetWorker->RegisterBuilder();
            return AZStd::move(assetWorker);
        }

        class BuilderComponent final
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(BuilderComponent, "{7B959232-A812-4F3F-817D-25FEEA844E7C}");
            static void Reflect(AZ::ReflectContext* context);

            BuilderComponent();
            ~BuilderComponent() override;

            // Expose the RPI builder service and add dependent component services as well
            void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            // AZ::Component overrides...
            void Activate() override;
            void Deactivate() override;


        private:
            BuilderComponent(const BuilderComponent&) = delete;

            using AssetWorker = AssetBuilderSDK::AssetBuilderCommandBus::Handler;
            AZStd::vector< AZStd::unique_ptr<AssetWorker> > m_assetWorkers;

            AZStd::vector< AZStd::unique_ptr<AZ::Data::AssetHandler> > m_assetHandlers;

            MaterialFunctorSourceDataRegistration m_materialFunctorRegistration;
        };
    } // namespace RPI
} // namespace AZ
