/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImportContextRegistryComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            void ImportContextRegistryComponent::Activate()
            {
            }

            void ImportContextRegistryComponent::Deactivate()
            {
            }

            void ImportContextRegistryComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("ImportContextRegistryService"));
            }

            void ImportContextRegistryComponent::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<ImportContextRegistryComponent, SceneCore::SceneSystemComponent>()->Version(1)->Attribute(
                        Edit::Attributes::SystemComponentTags,
                        AZStd::vector<Crc32>({ Events::AssetImportRequest::GetAssetImportRequestComponentTag() }));
                }
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
