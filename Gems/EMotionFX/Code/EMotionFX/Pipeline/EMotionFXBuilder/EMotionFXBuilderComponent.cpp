/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Pipeline/EMotionFXBuilder/EMotionFXBuilderComponent.h>

#include <EMotionFX/Source/Allocators.h>
#include <Integration/Assets/ActorAsset.h>
#include <Integration/Assets/MotionAsset.h>
#include <Integration/Assets/MotionSetAsset.h>
#include <Integration/Assets/AnimGraphAsset.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>

namespace EMotionFX
{
    namespace EMotionFXBuilder
    {
        void EMotionFXBuilderComponent::Activate()
        {
            m_motionSetBuilderWorker.RegisterBuilderWorker();
            m_animGraphBuilderWorker.RegisterBuilderWorker();

            // Initialize asset handlers.
            m_assetHandlers.emplace_back(aznew EMotionFX::Integration::ActorAssetHandler);
            m_assetHandlers.emplace_back(aznew EMotionFX::Integration::MotionAssetHandler);
            m_assetHandlers.emplace_back(aznew EMotionFX::Integration::MotionSetAssetBuilderHandler);
            m_assetHandlers.emplace_back(aznew EMotionFX::Integration::AnimGraphAssetBuilderHandler);

            // Add asset types and extensions to AssetCatalog.
            auto assetCatalog = AZ::Data::AssetCatalogRequestBus::FindFirstHandler();
            if (assetCatalog)
            {
                assetCatalog->EnableCatalogForAsset(azrtti_typeid<EMotionFX::Integration::ActorAsset>());
                assetCatalog->EnableCatalogForAsset(azrtti_typeid<EMotionFX::Integration::MotionAsset>());
                assetCatalog->EnableCatalogForAsset(azrtti_typeid<EMotionFX::Integration::MotionSetAsset>());
                assetCatalog->EnableCatalogForAsset(azrtti_typeid<EMotionFX::Integration::AnimGraphAsset>());

                assetCatalog->AddExtension("actor");        // Actor
                assetCatalog->AddExtension("motion");       // Motion
                assetCatalog->AddExtension("motionset");    // Motion set
                assetCatalog->AddExtension("animgraph");    // Anim graph
            }
        }

        void EMotionFXBuilderComponent::Deactivate()
        {
            m_motionSetBuilderWorker.BusDisconnect();
            m_animGraphBuilderWorker.BusDisconnect();

            m_assetHandlers.clear();
        }

        void EMotionFXBuilderComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EMotionFXBuilderComponent, AZ::Component>()
                    ->Version(1)
                    ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
            }
        }
    }
}
