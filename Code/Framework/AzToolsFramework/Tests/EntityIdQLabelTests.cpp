/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzFramework/Application/Application.h>
#include <AzFramework/Entity/EntityContext.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityIdQLabel.hxx>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>
#include <AzToolsFramework/Viewport/ActionBus.h>

#include <QtTest/QtTest>

using namespace AzToolsFramework;

namespace UnitTest
{

    // Test widget to store a EntityIdQLabel
    class EntityIdQLabel_TestWidget
        : public QWidget
    {
    public:
        explicit EntityIdQLabel_TestWidget(QWidget* parent = nullptr)
            : QWidget(nullptr)
        {
            AZ_UNUSED(parent);
            // ensure EntityIdQLabel_TestWidget can intercept and filter any incoming events itself
            installEventFilter(this);

            m_testLabel = new EntityIdQLabel(this);
        }

        EntityIdQLabel* m_testLabel = nullptr;

    };

    // Used to simulate a system implementing the EditorRequests bus to validate that the double click will
    // result in a GoToSelectedEntitiesInViewports event
    class EditorRequestHandlerTest : AzToolsFramework::EditorRequests::Bus::Handler
    {
    public:
        EditorRequestHandlerTest()
        {
            AzToolsFramework::EditorRequests::Bus::Handler::BusConnect();
        }

        ~EditorRequestHandlerTest() override
        {
            AzToolsFramework::EditorRequests::Bus::Handler::BusDisconnect();
        }

        void BrowseForAssets(AssetBrowser::AssetSelectionModel& /*selection*/) override {}
        int GetIconTextureIdFromEntityIconPath(const AZStd::string& entityIconPath) override { AZ_UNUSED(entityIconPath);  return 0; }
        bool DisplayHelpersVisible() override { return false; }

        void GoToSelectedEntitiesInViewports() override 
        {
            m_wentToSelectedEntitiesInViewport = true;
        }

        bool m_wentToSelectedEntitiesInViewport = false;

    };

    // Fixture to support testing EntityIdQLabel functionality
    class EntityIdQLabelTest
        : public AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            m_app.Start(AzFramework::Application::Descriptor());
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }

        void TearDown() override
        {
            m_app.Stop();
        }

        EntityIdQLabel_TestWidget* m_widget = nullptr;

    private:

        ToolsTestApplication m_app{ "EntityIdQLabelTest" };

    };

    TEST_F(EntityIdQLabelTest, DoubleClickEntitySelectionTest)
    {
        AZ::Entity* entity = aznew AZ::Entity();
        ASSERT_TRUE(entity != nullptr);
        entity->Init();
        entity->Activate();

        AZ::EntityId entityId = entity->GetId();
        ASSERT_TRUE(entityId.IsValid());

        EntityIdQLabel_TestWidget* widget = new EntityIdQLabel_TestWidget(nullptr);
        ASSERT_TRUE(widget != nullptr);
        ASSERT_TRUE(widget->m_testLabel != nullptr);

        widget->m_testLabel->setFocus();
        widget->m_testLabel->SetEntityId(entityId, {});

        EditorRequestHandlerTest editorRequestHandler;

        // Simulate double clicking the label
        QTest::mouseDClick(widget->m_testLabel, Qt::LeftButton);

        // If successful we expect the label's entity to be selected.
        EntityIdList selectedEntities;
        ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);

        EXPECT_FALSE(selectedEntities.empty()) << "Double clicking on an EntityIdQLabel should select the entity";
        EXPECT_TRUE(selectedEntities[0] == entityId) << "The selected entity is not the one that was double clicked";
        
        selectedEntities.clear();
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, selectedEntities);

        widget->m_testLabel->SetEntityId(AZ::EntityId(), {});

        ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &ToolsApplicationRequests::GetSelectedEntities);
        EXPECT_TRUE(selectedEntities.empty()) << "Double clicking on an EntityIdQLabel with an invalid entity ID shouldn't change anything";

        EXPECT_TRUE(editorRequestHandler.m_wentToSelectedEntitiesInViewport) << "Double clicking an EntityIdQLabel should result in a GoToSelectedEntitiesInViewports call";

        delete entity;
        delete widget;
    }
}
