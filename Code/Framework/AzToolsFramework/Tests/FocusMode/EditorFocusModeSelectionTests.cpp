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

#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/DirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>

#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

namespace AzToolsFramework
{
    static const AzToolsFramework::ManipulatorManagerId TestManipulatorManagerId =
        AzToolsFramework::ManipulatorManagerId(AZ::Crc32("TestManipulatorManagerId"));

    class EditorFocusModeSelectionFixture : public EditorFocusModeFixture
    {
    protected:
        void SetUp() override
        {
            EditorFocusModeFixture::SetUp();

            // Add meshes and change position of entities to be able to select them via the viewport

            // TODO - turn into helper so that it can be done more easily
            AZStd::vector<AZStd::string> componentNames;
            componentNames.push_back("Mesh");

            AZStd::vector<AZ::Uuid> componentTypeIds;
            EditorComponentAPIBus::BroadcastResult(
                componentTypeIds, &EditorComponentAPIRequests::FindComponentTypeIdsByEntityType,
                componentNames, EditorComponentAPIRequests::EntityType::Game);
            ASSERT_EQ(componentTypeIds.size(), 1);

            EditorComponentAPIRequests::AddComponentsOutcome outcome;
            EditorComponentAPIBus::BroadcastResult(
                outcome, &EditorComponentAPIRequests::AddComponentOfType,
                m_entityMap[CarEntityName], componentTypeIds[0]);
            ASSERT_TRUE(outcome.IsSuccess());

            // TODO - move to constant
            AZ::TransformBus::Event(m_entityMap[CarEntityName], &AZ::TransformBus::Events::SetWorldTranslation, CarPosition);

            m_manipulatorManager = AZStd::make_unique<AzToolsFramework::ManipulatorManager>(TestManipulatorManagerId);
        }

        void TearDown() override
        {
            m_manipulatorManager.reset();

            EditorFocusModeFixture::TearDown();
        }

    private:
        AZStd::unique_ptr<AzToolsFramework::ManipulatorManager> m_manipulatorManager;

        inline static const AZ::Vector3 CarPosition = AZ::Vector3(1.2f, 3.5f, 6.7f);
    };

    TEST_F(EditorFocusModeSelectionFixture, EditorFocusModeSelectionTests_SelectEntity)
    {
        // The Car entity has been moved to CarPosition and a Mesh component was added to it (but no Mesh - would that be necessary for selection?)
        // The intention is to click on the viewport to select the entity, and then verify that the selection has just 1 element and it's the Car's Id.

    }
}
