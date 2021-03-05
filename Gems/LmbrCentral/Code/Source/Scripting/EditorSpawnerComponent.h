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

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <AzCore/Slice/SliceAsset.h>

namespace LmbrCentral
{
    /**
     * Editor spawner component
     * Spawns the entities from a ".dynamicslice" asset at runtime.
     */
    class EditorSpawnerComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorSpawnerComponent, "{77CDE991-EC1A-B7C1-B112-7456ABAC81A1}", EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType&);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType&);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType&);

        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    protected:

        //////////////////////////////////////////////////////////////////////////
        // Data changed validation methods
        AZ::u32 SliceAssetChanged();
        AZ::u32 SpawnOnActivateChanged();
        bool HasInfiniteLoop();

        //////////////////////////////////////////////////////////////////////////
        // Serialized members
        AZ::Data::Asset<AZ::DynamicSliceAsset> m_sliceAsset{ AZ::Data::AssetLoadBehavior::PreLoad };
        bool m_spawnOnActivate = false;
        bool m_destroyOnDeactivate = false;
    };

} // namespace LmbrCentral
