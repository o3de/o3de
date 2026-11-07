/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssImpImporterComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneBuilder/ImportContextRegistry.h>
#include <SceneAPI/SceneBuilder/ImportContexts/AssImpImportContextProvider.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace AZ::AssImp
{
    void AssImpImporterComponent::Activate()
    {
        if (auto* registry = SceneAPI::SceneBuilder::ImportContextRegistryInterface::Get())
        {
            // Create and register the ImportContextProvider for AssImp. It serves as the
            // fallback importer when no specialized provider claims a file extension.
            m_provider = aznew SceneAPI::SceneBuilder::AssImpImportContextProvider();
            registry->RegisterContextProvider(m_provider);
            AZ_Info("AssImp", "AssImp Import Context was registered.\n");
        }
        else
        {
            AZ_Error("AssImp", false, "ImportContextRegistryInterface not found. AssImp Import Context was not registered.");
        }
    }

    void AssImpImporterComponent::Deactivate()
    {
        if (auto* registry = SceneAPI::SceneBuilder::ImportContextRegistryInterface::Get(); registry && m_provider)
        {
            registry->UnregisterContextProvider(m_provider);
        }
        m_provider = nullptr;
    }

    void AssImpImporterComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        // Marker service so that other import providers can order their activation after
        // this component via GetDependentServices (providers registered later take
        // precedence for the extensions they claim).
        provided.push_back(AZ_CRC_CE("AssImpImportContextProviderService"));
    }

    void AssImpImporterComponent::GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent)
    {
        // Activate after the registry component when it is present on the same entity.
        // Declared as a dependency rather than a requirement so that entities without
        // the registry component can still activate this component.
        dependent.push_back(AZ_CRC_CE("ImportContextRegistryService"));
    }

    void AssImpImporterComponent::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<AssImpImporterComponent, SceneAPI::SceneCore::SceneSystemComponent>()
                ->Version(1)
                ->Attribute(
                    Edit::Attributes::SystemComponentTags,
                    AZStd::vector<Crc32>({ SceneAPI::Events::AssetImportRequest::GetAssetImportRequestComponentTag() }));
        }
    }
} // namespace AZ::AssImp
