/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/FocusMode/EditorFocusModeFixture.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/string/string.h>

#include <AzFramework/Viewport/ViewportScreen.h>

#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzManipulatorTestFramework/DirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>

#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>


namespace AzToolsFramework
{
    using EditorFocusModeSelectionFixture = UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<EditorFocusModeFixture>;

    /*
    static const AzToolsFramework::ManipulatorManagerId TestManipulatorManagerId =
        AzToolsFramework::ManipulatorManagerId(AZ::Crc32("TestManipulatorManagerId"));

    class EditorFocusModeSelectionFixture
        : public EditorFocusModeFixture
    {
        using IndirectCallManipulatorViewportInteraction = AzManipulatorTestFramework::IndirectCallManipulatorViewportInteraction;
        using ImmediateModeActionDispatcher = AzManipulatorTestFramework::ImmediateModeActionDispatcher;

    protected:
        void PrepareEntityForPicking(AZ::EntityId entityId, AZ::Vector3 position)
        {
            AZ::Entity* entity = GetEntityById(entityId);
            AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTranslation, position);

            entity->Deactivate();
            entity->CreateComponent<BoundsTestComponent>();
            entity->Activate();
        }

        void SetUpEditorFixtureImpl() override
        {
            EditorFocusModeFixture::SetUpEditorFixtureImpl();

            m_viewportManipulatorInteraction = AZStd::make_unique<IndirectCallManipulatorViewportInteraction>();
            m_actionDispatcher = AZStd::make_unique<ImmediateModeActionDispatcher>(*m_viewportManipulatorInteraction);

            // register a simple component implementing BoundsRequestBus and EditorComponentSelectionRequestsBus
            GetApplication()->RegisterComponentDescriptor(BoundsTestComponent::CreateDescriptor());

            PrepareEntityForPicking(m_entityMap[CarEntityName], CarPosition);

            AzFramework::SetCameraTransform(
                m_cameraState,
                AZ::Transform::CreateFromQuaternionAndTranslation(
                    AZ::Quaternion::CreateFromEulerAnglesDegrees(AZ::Vector3(CameraPitch, 0.0f, CameraYaw)), CameraPosition));
        }

        void TearDownEditorFixtureImpl() override
        {
            m_actionDispatcher.reset();
            m_viewportManipulatorInteraction.reset();

            EditorFocusModeFixture::TearDownEditorFixtureImpl();
        }

    public:
        AzToolsFramework::EntityIdList GetSelectedEntities()
        {
            AzToolsFramework::EntityIdList selectedEntities;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                selectedEntities, &AzToolsFramework::ToolsApplicationRequestBus::Events::GetSelectedEntities);
            return selectedEntities;
        }

        AzFramework::CameraState m_cameraState;
        AZStd::unique_ptr<ImmediateModeActionDispatcher> m_actionDispatcher;
        AZStd::unique_ptr<IndirectCallManipulatorViewportInteraction> m_viewportManipulatorInteraction;
        AzToolsFramework::EditorVisibleEntityDataCache m_cache;

        inline static const AZ::Vector3 CameraPosition = AZ::Vector3(10.0f, 15.0f, 10.0f);
        inline static const float CameraPitch = 0.0f;
        inline static const float CameraYaw = 90.0f;
        inline static const AZ::Vector3 CarPosition = AZ::Vector3(5.0f, 15.0f, 10.0f);
    };
    */

    TEST_F(EditorFocusModeSelectionFixture, EditorFocusModeSelectionTests_SelectEntity)
    {
        auto selectedEntitiesBefore = GetSelectedEntities();
        EXPECT_TRUE(selectedEntitiesBefore.empty());

        // calculate the position in screen space of the initial entity position
        const auto carScreenPosition = AzFramework::WorldToScreen(CarEntityPosition, m_cameraState);

        // click the entity in the viewport
        m_actionDispatcher->SetStickySelect(true)
            ->CameraState(m_cameraState)
            ->MousePosition(carScreenPosition)
            ->MouseLButtonDown()
            ->MouseLButtonUp();

        // entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 1);
        EXPECT_EQ(selectedEntitiesAfter.front(), m_entityMap[CarEntityName]);
    }
}
