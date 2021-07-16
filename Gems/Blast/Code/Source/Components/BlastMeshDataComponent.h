/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>

namespace Blast
{
    class EditorBlastMeshDataComponent;

    //! An interface that is responsible for providing meshes of chunks.
    class BlastMeshData
    {
    public:
        virtual ~BlastMeshData() = default;
        virtual const AZ::Data::Asset<AZ::RPI::ModelAsset>& GetMeshAsset(size_t index) const = 0;
        virtual const AZStd::vector<AZ::Data::Asset<AZ::RPI::ModelAsset>>& GetMeshAssets() const = 0;
    };

    //! Component that stores meshes for the blast family to use during game time.
    class BlastMeshDataComponent
        : public AZ::Component
        , public BlastMeshData
    {
    public:
        AZ_COMPONENT(BlastMeshDataComponent, "{8910FB8D-D474-443B-93EC-84E4A595ADDF}", AZ::Component);

        BlastMeshDataComponent() = default;
        BlastMeshDataComponent(const AZStd::vector<AZ::Data::Asset<AZ::RPI::ModelAsset>>& meshAssets);
        ~BlastMeshDataComponent() = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // AZ::Component interface implementation
        void Activate() override {}
        void Deactivate() override {}

        const AZ::Data::Asset<AZ::RPI::ModelAsset>& GetMeshAsset(size_t index) const override;
        const AZStd::vector<AZ::Data::Asset<AZ::RPI::ModelAsset>>& GetMeshAssets() const override;

    private:
        // Reflected data
        AZStd::vector<AZ::Data::Asset<AZ::RPI::ModelAsset>> m_meshAssets;
    };
} // namespace Blast
