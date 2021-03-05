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

#include <Asset/BlastAsset.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Physics/Material.h>
#include <Blast/BlastDebug.h>
#include <Blast/BlastMaterial.h>

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

        /// Returns the material library asset id. Used to supply MaterialIdWidget with material library.
        AZ::Data::AssetId GetMaterialLibraryAssetId() const;

        /// Return the physics material library asset id. Used to supply MaterialIdWidget with material library.
        AZ::Data::AssetId GetPhysicsMaterialLibraryAssetId() const;

        // Configurations
        AZ::Data::Asset<BlastAsset> m_blastAsset;
        Blast::BlastMaterialId m_materialId;
        Physics::MaterialId m_physicsMaterialId;
        BlastActorConfiguration m_actorConfiguration;
    };
} // namespace Blast
