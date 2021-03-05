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
