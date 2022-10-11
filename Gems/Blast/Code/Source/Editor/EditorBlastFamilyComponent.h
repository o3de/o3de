/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Asset/BlastAsset.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>
#include <AzFramework/Physics/Material/Legacy/LegacyPhysicsMaterialSelection.h>
#include <Blast/BlastActorConfiguration.h>
#include <Blast/BlastDebug.h>
#include <Material/BlastMaterialAsset.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace Blast
{
    class EditorBlastFamilyComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AZ::Data::AssetBus::MultiHandler
    {
    public:
        AZ_COMPONENT(
            EditorBlastFamilyComponent, "{ECB1689A-2B65-44D1-9227-9E62962A7FF7}",
            AzToolsFramework::Components::EditorComponentBase);

        EditorBlastFamilyComponent() = default;
        virtual ~EditorBlastFamilyComponent() = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        // AssetBus
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        AZ::Data::AssetId GetDefaultBlastAssetId() const;
        AZ::Data::AssetId GetDefaultPhysicsAssetId() const;

        // Configurations
        AZ::Data::Asset<BlastAsset> m_blastAsset;
        AZ::Data::Asset<MaterialAsset> m_blastMaterialAsset;
        BlastMaterialId m_legacyBlastMaterialId; // Kept to convert old blast material assets.
        AZ::Data::Asset<Physics::MaterialAsset> m_physicsMaterialAsset;
        PhysicsLegacy::MaterialId m_legacyPhysicsMaterialId; // Kept to convert old physics material assets.

        BlastActorConfiguration m_actorConfiguration;
    };
} // namespace Blast
