/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

// This module mocks the default procedural prefab logic creates some Atom Editor components such as EditorMeshComponent
namespace AZ::Render
{
    struct EditorMeshComponent
        : public AZ::Component
    {
        AZ_COMPONENT(EditorMeshComponent, "{DCE68F6E-2E16-4CB4-A834-B6C2F900A7E9}");
        void Activate() override {}
        void Deactivate() override {}
        static void Reflect(AZ::ReflectContext* context);
    };

    struct EditorMeshComponentHelper
        : public AZ::ComponentDescriptorHelper<EditorMeshComponent>
    {
        AZ_CLASS_ALLOCATOR(EditorMeshComponentHelper, AZ::SystemAllocator);

        void Reflect(AZ::ReflectContext* reflection) const override;
    };
}

namespace AZ::SceneAPI::Containers
{
    class Scene;
}

namespace UnitTest
{
    struct MockTransform
        : public AZ::SceneAPI::DataTypes::ITransform
    {
        AZ::Matrix3x4 m_matrix = AZ::Matrix3x4::CreateIdentity();

        AZ::Matrix3x4& GetMatrix() override
        {
            return m_matrix;
        }

        const AZ::Matrix3x4& GetMatrix() const
        {
            return m_matrix;
        }
    };

    AZStd::unique_ptr<AZ::SceneAPI::Containers::Scene> CreateMockScene(
        const AZStd::string_view manifestFilename = "ManifestFilename",
        const AZStd::string_view sourceFileName = "Source",
        const AZStd::string_view watchFolder = "WatchFolder");

    AZStd::unique_ptr<AZ::SceneAPI::Containers::Scene> CreateEmptyMockSceneWithRoot(
        const AZStd::string_view manifestFilename = "ManifestFilename",
        const AZStd::string_view sourceFileName = "Source",
        const AZStd::string_view watchFolder = "WatchFolder");

    void BuildMockScene(AZ::SceneAPI::Containers::Scene& scene);
    
}
