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

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>

namespace PhysX
{
    namespace Pipeline
    {
        //! Optional collider configuration data that is stored in the asset.
        //! All the fields here are optional. If you set them at the asset building stage
        //! that data will then be used in the collider
        class AssetColliderConfiguration
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetColliderConfiguration, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(MeshAssetData, "{463AA6A7-8A1A-42B6-B103-F6939CC7A8A5}");

            static void Reflect(AZ::ReflectContext* context);
            void UpdateColliderConfiguration(Physics::ColliderConfiguration& colliderConfiguration) const;

            AZStd::optional<AzPhysics::CollisionLayer> m_collisionLayer; //!< Which collision layer is this collider on.
            AZStd::optional<AzPhysics::CollisionGroups::Id> m_collisionGroupId; //!< Which layers does this collider collide with.
            AZStd::optional<bool> m_isTrigger; //!< Should this shape act as a trigger shape.
            AZStd::optional<AZ::Transform> m_transform; //! Shape offset relative to the connected rigid body.
            AZStd::optional<AZStd::string> m_tag; //!< Identification tag for the collider
        };

        //! Physics Asset data structure. 
        class MeshAssetData
        {
        public:
            AZ_CLASS_ALLOCATOR(MeshAssetData, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(MeshAssetData, "{958C8530-DF1F-4B68-800B-E92056708127}");

            static void Reflect(AZ::ReflectContext* context);

            // Reserved material index indicating that cooked mesh itself stores the indices.
            // Wrapping the numeric_limits<AZ::u16>::max function in parenthesis to get around the issue with windows.h defining max as a macro.
            static constexpr AZ::u16 TriangleMeshMaterialIndex = (std::numeric_limits<AZ::u16>::max)();

            using ShapeConfigurationPair = AZStd::pair<AZStd::shared_ptr<AssetColliderConfiguration>, 
                AZStd::shared_ptr<Physics::ShapeConfiguration>>; // Have to use shared_ptr here because Physics::ShapeConfigurationList uses it
            using ShapeConfigurationList = AZStd::vector<ShapeConfigurationPair>;

            ShapeConfigurationList m_colliderShapes; //!< Shapes data with optional collider configuration override.
            AZStd::vector<AZStd::string> m_surfaceNames; //!< List of all surface names.
            AZStd::vector<AZStd::string> m_materialNames; //!< List of all material names.
            AZStd::vector<AZ::u16> m_materialIndexPerShape; //!< An index of the material in m_materialNames for each shape.
        };

        //! Represents a PhysX mesh asset. This is an AZ::Data::AssetData wrapper around MeshAssetData
        class MeshAsset
            : public AZ::Data::AssetData
        {
        public:
            AZ_CLASS_ALLOCATOR(MeshAsset, AZ::SystemAllocator, 0);
            AZ_RTTI(MeshAsset, "{7A2871B9-5EAB-4DE0-A901-B0D2C6920DDB}", AZ::Data::AssetData);

            MeshAssetData m_assetData;
        };
    } // namespace Pipeline
} // namespace PhysX
