/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <Assets/HairAsset.h>
#include <Builders/HairBuilderComponent.h>
#include <Builders/HairAssetBuilder.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            void HairBuilderComponent::Reflect(ReflectContext* context)
            {
                if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
                {
                    serialize->Class<HairBuilderComponent, Component>()
                        ->Version(1)->Attribute(
                        AZ::Edit::Attributes::SystemComponentTags,
                        AZStd::vector<AZ::Crc32>({AssetBuilderSDK::ComponentTags::AssetBuilder}));
                        ;
                }
            }

            void HairBuilderComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("HairBuilderService"));
            }

            void HairBuilderComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC_CE("HairBuilderService"));
            }

            void HairBuilderComponent::Activate()
            {
                m_hairAssetBuilder.RegisterBuilder();
                m_hairAssetHandler.Register();

                // Add asset types and extensions to AssetCatalog.
                auto assetCatalog = AZ::Data::AssetCatalogRequestBus::FindFirstHandler();
                if (assetCatalog)
                {
                    assetCatalog->EnableCatalogForAsset(azrtti_typeid<HairAsset>());
                    assetCatalog->AddExtension(AMD::TFXCombinedFileExtension);
                }
            }

            void HairBuilderComponent::Deactivate()
            {
                m_hairAssetBuilder.BusDisconnect();
                m_hairAssetHandler.Unregister();
            }
        } // namespace Hair
    } // End Render namespace
} // End AZ namespace

