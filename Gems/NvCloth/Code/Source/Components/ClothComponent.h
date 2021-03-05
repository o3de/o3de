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

#include <AzCore/Component/Component.h>

#include <LmbrCentral/Rendering/MeshComponentBus.h>

#include <Components/ClothConfiguration.h>
#include <Components/ClothComponentMesh/ClothComponentMesh.h>

namespace NvCloth
{
    //! Class for runtime Cloth Component.
    class ClothComponent
        : public AZ::Component
        , public LmbrCentral::MeshComponentNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(ClothComponent, "{AC9B8FA0-A6DA-4377-8219-25BA7E4A22E9}");

        static void Reflect(AZ::ReflectContext* context);

        ClothComponent() = default;
        explicit ClothComponent(const ClothConfiguration& config);

        AZ_DISABLE_COPY_MOVE(ClothComponent);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        const ClothComponentMesh* GetClothComponentMesh() const { return m_clothComponentMesh.get(); }

    protected:
        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // LmbrCentral::MeshComponentNotificationBus::Handler overrides ...
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void OnMeshDestroyed() override;

    private:
        ClothConfiguration m_config;

        AZStd::unique_ptr<ClothComponentMesh> m_clothComponentMesh;
    };
} // namespace NvCloth
