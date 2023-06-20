/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/API/EntityPropertyEditorRequestsBus.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityIdQLineEdit.h>

#include <QApplication>

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AzToolsFramework;

    // Test widget to store an EntityIdQLineEdit
    class TestEntityIdParentWidget
        : public QWidget
    {
    public:
        explicit TestEntityIdParentWidget(QWidget* parent = nullptr)
            : QWidget(nullptr)
        {
            AZ_UNUSED(parent);
            // ensure TestWidget can intercept and filter any incoming events itself
            installEventFilter(this);

            m_testLineEdit = new EntityIdQLineEdit(this);
        }

        EntityIdQLineEdit* m_testLineEdit = nullptr;

    };

    class EntityIdQLineEditTests
        : public ToolsApplicationFixture<>
    {
    };

    TEST_F(EntityIdQLineEditTests, DoubleClickWontSelectInvalidEntity)
    {
        AZ::Entity* entity = aznew AZ::Entity();
        ASSERT_TRUE(entity != nullptr);
        entity->Init();
        entity->Activate();

        AZ::EntityId entityId = entity->GetId();
        ASSERT_TRUE(entityId.IsValid());

        TestEntityIdParentWidget* widget = new TestEntityIdParentWidget(nullptr);
        ASSERT_TRUE(widget != nullptr);
        ASSERT_TRUE(widget->m_testLineEdit != nullptr);

        // Set a valid EntityId
        widget->m_testLineEdit->setFocus();
        widget->m_testLineEdit->SetEntityId(entityId, {});

        // Simulate double click which will cause the EntityId to be set as selected
        QTest::mouseDClick(widget->m_testLineEdit, Qt::LeftButton);

        // If successful we expect the label's entity to be selected.
        EntityIdList selectedEntities;
        ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);

        EXPECT_EQ(selectedEntities.size(), 1) << "Double clicking on an EntityIdQLabel should only select a single entity";
        EXPECT_TRUE(selectedEntities[0] == entityId) << "The selected entity is not the one that was double clicked";

        selectedEntities.clear();
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, selectedEntities);

        // Now set an invalid EntityId
        widget->m_testLineEdit->SetEntityId(AZ::EntityId(), {});

        // Simulate double clicking again, which should not trigger a selection change since the EntityId is invalid
        QTest::mouseDClick(widget->m_testLineEdit, Qt::LeftButton);

        ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);
        EXPECT_TRUE(selectedEntities.empty()) << "Double clicking on an EntityIdQLabel with an invalid entity ID shouldn't change anything";

        delete entity;
        delete widget;
    }
}
