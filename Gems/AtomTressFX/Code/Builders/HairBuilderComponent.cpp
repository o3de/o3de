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

