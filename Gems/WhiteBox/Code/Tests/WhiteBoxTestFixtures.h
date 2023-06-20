/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkUtils.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ViewportInteraction.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <WhiteBox/WhiteBoxToolApi.h>
#include <tuple>

#include "EditorWhiteBoxComponent.h"
#include "Rendering/WhiteBoxRenderData.h"
#include "WhiteBoxComponent.h"

namespace UnitTest
{
    class WhiteBoxTestFixture
        : public LeakDetectionFixture
        , private TraceBusRedirector
    {
    public:
        void SetUp() override final
        {
            LeakDetectionFixture::SetUp();
            AZ::Debug::TraceMessageBus::Handler::BusConnect();

            m_whiteBox = WhiteBox::Api::CreateWhiteBoxMesh();

            SetUpEditorFixtureImpl();
        }

        void TearDown() override final
        {
            TearDownEditorFixtureImpl();

            m_whiteBox.reset();

            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
            LeakDetectionFixture::TearDown();
        }

        virtual void SetUpEditorFixtureImpl() {}
        virtual void TearDownEditorFixtureImpl() {}

        WhiteBox::Api::WhiteBoxMeshPtr m_whiteBox;
    };

    class EditorWhiteBoxComponentTestFixture : public ToolsApplicationFixture<>
    {
    public:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        AZ::EntityId m_whiteBoxEntityId;
        WhiteBox::EditorWhiteBoxComponent* m_whiteBoxComponent = nullptr;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorWhiteBoxComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_whiteBoxComponentDescriptor;
    };

    struct EditorWhiteBoxEntityAndComponent
    {
        AZ::Entity* m_entity = nullptr;
        WhiteBox::EditorWhiteBoxComponent* m_editorWhiteBoxComponent = nullptr;

        AZ::EntityId GetEntityId() const
        {
            return m_entity->GetId();
        }
    };

    inline EditorWhiteBoxEntityAndComponent CreateEditorEntityWithEditorWhiteBoxComponent()
    {
        AZ::Entity* whiteBoxEntity = nullptr;
        CreateDefaultEditorEntity("WhiteBox", &whiteBoxEntity);

        whiteBoxEntity->Deactivate();
        auto* editorWhiteBoxComponent = whiteBoxEntity->CreateComponent<WhiteBox::EditorWhiteBoxComponent>();
        whiteBoxEntity->Activate();

        return {whiteBoxEntity, editorWhiteBoxComponent};
    }

    inline void EditorWhiteBoxComponentTestFixture::SetUpEditorFixtureImpl()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        m_editorWhiteBoxComponentDescriptor =
            AZStd::unique_ptr<AZ::ComponentDescriptor>(WhiteBox::EditorWhiteBoxComponent::CreateDescriptor());
        m_editorWhiteBoxComponentDescriptor->Reflect(serializeContext);

        m_whiteBoxComponentDescriptor =
            AZStd::unique_ptr<AZ::ComponentDescriptor>(WhiteBox::WhiteBoxComponent::CreateDescriptor());
        m_whiteBoxComponentDescriptor->Reflect(serializeContext);

        auto editorEntityAndWhiteBox = CreateEditorEntityWithEditorWhiteBoxComponent();
        m_whiteBoxEntityId = editorEntityAndWhiteBox.GetEntityId();
        m_whiteBoxComponent = editorEntityAndWhiteBox.m_editorWhiteBoxComponent;
    }

    inline void EditorWhiteBoxComponentTestFixture::TearDownEditorFixtureImpl()
    {
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, m_whiteBoxEntityId);

        m_whiteBoxComponentDescriptor.reset();
        m_editorWhiteBoxComponentDescriptor.reset();
    }

    using EditorWhiteBoxModifierTestFixture =
        IndirectCallManipulatorViewportInteractionFixtureMixin<EditorWhiteBoxComponentTestFixture>;

    struct FaceTestData
    {
        std::vector<AZ::Vector3> m_positions;
        size_t m_numCulledFaces;
    };

    class WhiteBoxVertexDataTestFixture
        : public WhiteBoxTestFixture
        , public testing::WithParamInterface<FaceTestData>
    {
    public:
        WhiteBox::WhiteBoxFaces ConstructFaceData(const FaceTestData& inFaces)
        {
            const auto& positionData = inFaces.m_positions;
            const size_t numVertices = positionData.size();
            const size_t numFaces = numVertices / 3;
            WhiteBox::WhiteBoxFaces outFaces;
            outFaces.resize(numFaces);

            // assemble the triangle primitives from the position data
            // (normals and uvs will be undefined)
            for (size_t faceIndex = 0, vertexIndex = 0; faceIndex < numFaces; faceIndex++, vertexIndex += 3)
            {
                WhiteBox::WhiteBoxFace& face = outFaces[faceIndex];
                face.m_v1.m_position = inFaces.m_positions[vertexIndex];
                face.m_v2.m_position = inFaces.m_positions[vertexIndex + 1];
                face.m_v3.m_position = inFaces.m_positions[vertexIndex + 2];
            }

            return outFaces;
        }
    };

    // AZ::Vector3 doesn't play ball with gtest's parameterized tests
    struct Vector3
    {
        float x;
        float y;
        float z;
    };

    // vector components to apply random noise to
    enum class NoiseSource
    {
        None,
        XComponent,
        YComponent,
        ZComponent,
        XYComponent,
        XZComponent,
        YZComponent,
        XYZComponent
    };

    // rotation of normal (45 degrees around each axis)
    enum class Rotation
    {
        Identity,
        XAxis,
        ZAxis,
        XZAxis
    };

    // noise and rotation parameters to be applied per permutation
    using WhiteBoxUVTestParams = std::tuple<Vector3, NoiseSource, Rotation>;

    class WhiteBoxUVTestFixture
        : public WhiteBoxTestFixture
        , public ::testing::WithParamInterface<WhiteBoxUVTestParams>
    {
    public:
    };

    const auto DefaultViewportSize = AzFramework::ScreenSize(1024, 576); // 16:9 ratio
} // namespace UnitTest
