/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzCore/Module/Module.h>
#include <Builders/MeshletsBuildersSystemComponent.h>
#include <Builders/SceneApiMeshletPackExporter.h>
#include <Builders/MeshletPackRuleBehavior.h>

namespace AZ::Meshlets::Builders
{
    class MeshletsBuildersModule : public AZ::Module
    {
    public:
        AZ_RTTI(MeshletsBuildersModule, "{2C8D5B9A-4F7E-41A6-95B3-7D2E9C1F5A48}", AZ::Module);

        MeshletsBuildersModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                MeshletsBuildersSystemComponent::CreateDescriptor(),
                SceneApiMeshletPackExporter::CreateDescriptor(),
                MeshletPackRuleBehavior::CreateDescriptor(),
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<MeshletsBuildersSystemComponent>(),
            };
        }
    };

} // namespace AZ::Meshlets::Builders

AZ_DECLARE_MODULE_CLASS(Gem_Meshlets_Builders, AZ::Meshlets::Builders::MeshletsBuildersModule)
