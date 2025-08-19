/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/EditorSystemComponent.h>

#include <AzCore/Name/Name.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <OpenParticleSystem/Serializer/ParticleSourceData.h>

#include <Atom/RHI/Factory.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>

#include <QAction>
#include <Editor/QtViewPaneManager.h>

#include "OpenParticleSystem/ParticleEditDataConfig.h"

namespace OpenParticle
{
    void EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSystemComponent, AZ::Component>()->Version(0);
        }
        ParticleEditDataConfig::Reflect(context);
        ParticleSourceData::Reflect(context);
    }

    void EditorSystemComponent::Activate()
    {
        AzToolsFramework::EditorMenuNotificationBus::Handler::BusConnect();
        EditorParticleSystemComponentRequestBus::Handler::BusConnect();
        m_particleBrowserInteractions.reset(aznew ParticleBrowserInteractions);
    }

    void EditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorMenuNotificationBus::Handler::BusDisconnect();
        EditorParticleSystemComponentRequestBus::Handler::BusDisconnect();
        ResetMenu();
        m_particleBrowserInteractions.reset();
    }

    void EditorSystemComponent::ResetMenu()
    {
        if (m_openParticleEditorAction != nullptr)
        {
            delete m_openParticleEditorAction;
            m_openParticleEditorAction = nullptr;
        }
    }

    void EditorSystemComponent::OpenParticleEditor(const AZStd::string& sourcePath)
    {
        AZ_TracePrintf("ParticleSystem", "Launching ParticleSystem Editor");

        QtViewPaneManager::instance()->OpenPane("OpenParticle Editor");
        EBUS_EVENT(EditorParticleOpenParticleRequestsBus, OpenParticleFile, sourcePath.data());
    }

    void EditorSystemComponent::OnResetToolMenuItems()
    {
    }

    void EditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("OpenParticleEditorService"));
    }

    void EditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("OpenParticleEditorService"));
    }

    void EditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("OpenParticleService"));
    }

    AZ::TypeId EditorSystemComponent::GetParticleSystemConfigType() const
    {
        return azrtti_typeid<OpenParticle::SystemConfig>();
    }

    AZ::TypeId EditorSystemComponent::GetEmitterConfigType() const
    {
        return azrtti_typeid<OpenParticle::EmitterConfig>();
    }

    AZStd::vector<AZ::TypeId> EditorSystemComponent::GetEmitTypes() const
    {
        return AZStd::vector<AZ::TypeId>{
            azrtti_typeid<OpenParticle::EmitBurstList>(),
            azrtti_typeid<OpenParticle::EmitSpawn>(),
            azrtti_typeid<OpenParticle::EmitSpawnOverMoving>(),
            azrtti_typeid<OpenParticle::ParticleEventHandler>(),
            azrtti_typeid<OpenParticle::InheritanceHandler>()
        };
    }

    AZStd::vector<AZ::TypeId> EditorSystemComponent::GetSpawnTypes() const
    {
        return AZStd::vector<AZ::TypeId>{
            azrtti_typeid<OpenParticle::SpawnColor>(),
            azrtti_typeid<OpenParticle::SpawnLifetime>(),
            azrtti_typeid<OpenParticle::SpawnLocBox>(),
            azrtti_typeid<OpenParticle::SpawnLocPoint>(),
            azrtti_typeid<OpenParticle::SpawnLocSphere>(),
            azrtti_typeid<OpenParticle::SpawnLocCylinder>(),
            azrtti_typeid<OpenParticle::SpawnLocSkeleton>(),
            azrtti_typeid<OpenParticle::SpawnLocTorus>(),
            azrtti_typeid<OpenParticle::SpawnSize>(),
            azrtti_typeid<OpenParticle::SpawnVelDirection>(),
            azrtti_typeid<OpenParticle::SpawnVelSector>(),
            azrtti_typeid<OpenParticle::SpawnVelCone>(),
            azrtti_typeid<OpenParticle::SpawnVelSphere>(),
            azrtti_typeid<OpenParticle::SpawnVelConcentrate>(),
            azrtti_typeid<OpenParticle::SpawnRotation>(),
            azrtti_typeid<OpenParticle::SpawnLightEffect>(),
            azrtti_typeid<OpenParticle::SpawnLocationEvent>()
        };
    }

    AZStd::vector<AZ::TypeId> EditorSystemComponent::GetUpdateTypes() const
    {
        return AZStd::vector<AZ::TypeId>{
            azrtti_typeid<OpenParticle::UpdateConstForce>(),
            azrtti_typeid<OpenParticle::UpdateDragForce>(),
            azrtti_typeid<OpenParticle::UpdateVortexForce>(),
            azrtti_typeid<OpenParticle::UpdateCurlNoiseForce>(),
            azrtti_typeid<OpenParticle::UpdateColor>(),
            azrtti_typeid<OpenParticle::UpdateLocationEvent>(),
            azrtti_typeid<OpenParticle::UpdateDeathEvent>(),
            azrtti_typeid<OpenParticle::UpdateCollisionEvent>(),
            azrtti_typeid<OpenParticle::UpdateInheritanceEvent>(),
            azrtti_typeid<OpenParticle::UpdateSizeLinear>(),
            azrtti_typeid<OpenParticle::UpdateSizeByVelocity>(),
            azrtti_typeid<OpenParticle::SizeScale>(),
            azrtti_typeid<OpenParticle::UpdateSubUv>(),
            azrtti_typeid<OpenParticle::UpdateRotateAroundPoint>(),
            azrtti_typeid<OpenParticle::UpdateVelocity>(),
            azrtti_typeid<OpenParticle::ParticleCollision>()
        };
    }

    AZStd::vector<AZ::TypeId> EditorSystemComponent::GetRenderTypes() const
    {
        return AZStd::vector<AZ::TypeId>{
            azrtti_typeid<OpenParticle::SpriteConfig>(),
            azrtti_typeid<OpenParticle::MeshConfig>(),
            azrtti_typeid<OpenParticle::RibbonConfig>()
        };
    }

    AZ::TypeId EditorSystemComponent::GetDefaultEmitType() const
    {
        return azrtti_typeid<OpenParticle::EmitSpawn>();
    }

    AZStd::vector<AZ::TypeId> EditorSystemComponent::GetDefaultSpawnTypes() const
    {
        return AZStd::vector<AZ::TypeId>{
            azrtti_typeid<OpenParticle::SpawnLifetime>(),
            azrtti_typeid<OpenParticle::SpawnLocPoint>(),
            azrtti_typeid<OpenParticle::SpawnVelDirection>()
        };
    }

    AZ::TypeId EditorSystemComponent::GetDefaultRenderType() const
    {
        return azrtti_typeid<OpenParticle::SpriteConfig>();
    }

} // namespace OpenParticle
