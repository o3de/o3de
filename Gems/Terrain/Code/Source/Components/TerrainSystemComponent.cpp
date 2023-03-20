/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Components/TerrainSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <TerrainSystem/TerrainSystem.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <TerrainRenderer/Passes/TerrainClipmapComputePass.h>
#include <TerrainRenderer/Passes/TerrainClipmapDebugPass.h>

namespace Terrain
{
    void TerrainSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<TerrainSystemComponent>("Terrain", "The Terrain System Component enables Terrain.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }
    }

    void TerrainSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("TerrainService"));
    }

    void TerrainSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("TerrainService"));
    }

    void TerrainSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void TerrainSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void TerrainSystemComponent::Init()
    {
    }

    void TerrainSystemComponent::Activate()
    {
        // Currently, the Terrain System Component owns the Terrain System instance because the Terrain World component gets recreated
        // every time an entity is added or removed to a level.  If this ever changes, the Terrain System ownership could move into
        // the level component.
        m_terrainSystem = new TerrainSystem();

        auto* passSystem = AZ::RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");

        // Setup handler for load pass templates mappings
        m_loadTemplatesHandler = AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler([this]() { this->LoadPassTemplateMappings(); });
        passSystem->ConnectEvent(m_loadTemplatesHandler);

        // Register terrain system related passes
        passSystem->AddPassCreator(AZ::Name("TerrainMacroClipmapGenerationPass"), &TerrainMacroClipmapGenerationPass::Create);
        passSystem->AddPassCreator(AZ::Name("TerrainDetailClipmapGenerationPass"), &TerrainDetailClipmapGenerationPass::Create);
        passSystem->AddPassCreator(AZ::Name("TerrainClipmapDebugPass"), &TerrainClipmapDebugPass::Create);
    }

    void TerrainSystemComponent::Deactivate()
    {
        m_loadTemplatesHandler.Disconnect();

        delete m_terrainSystem;
        m_terrainSystem = nullptr;
    }

    void TerrainSystemComponent::LoadPassTemplateMappings()
    {
        auto* passSystem = AZ::RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");

        const char* passTemplatesFile = "Passes/TerrainPassTemplates.azasset";
        passSystem->LoadPassTemplateMappings(passTemplatesFile);
    }
}
