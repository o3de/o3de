/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ComponentModeTestDoubles.h"
#include "ComponentModeTestFixture.h"

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/ViewportUi/ButtonGroup.h>
#include <AzToolsFramework/ViewportUi/ViewportUiSwitcher.h>
#include <AzToolsFramework/ComponentMode/ComponentModeSwitcher.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Application/ToolsApplication.h>



namespace UnitTest
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::ComponentModeFramework;

    using ComponentModeSwitcher = AzToolsFramework::ComponentModeFramework::ComponentModeSwitcher;
    using Switcher = AzToolsFramework::ComponentModeFramework::Switcher;
    using ViewportUiDisplay = AzToolsFramework::ViewportUi::Internal::ViewportUiDisplay;
    using ViewportUiElementId = AzToolsFramework::ViewportUi::ViewportUiElementId;
    using ButtonGroup = AzToolsFramework::ViewportUi::Internal::ButtonGroup;
    using Button = AzToolsFramework::ViewportUi::Internal::Button;
    using ButtonId = AzToolsFramework::ViewportUi::ButtonId;

    class ViewportUiManagerTestable : public AzToolsFramework::ViewportUi::ViewportUiManager
    {
    public:
        ViewportUiManagerTestable() = default;
        ~ViewportUiManagerTestable() override = default;

        const AZStd::unordered_map<AzToolsFramework::ViewportUi::ClusterId, AZStd::shared_ptr<ButtonGroup>>& GetClusterMap()
        {
            return m_clusterButtonGroups;
        }

        const AZStd::unordered_map<AzToolsFramework::ViewportUi::SwitcherId, AZStd::shared_ptr<ButtonGroup>>& GetSwitcherMap()
        {
            return m_switcherButtonGroups;
        }

        ViewportUiDisplay* GetViewportUiDisplay()
        {
            return m_viewportUi.get();
        }
    };

    class ComponentModeSwitcherTestable : public AzToolsFramework::ComponentModeFramework::ComponentModeSwitcher
    {
    public:
        ComponentModeSwitcherTestable() = default;
        ~ComponentModeSwitcherTestable() override = default;

        const Switcher& GetSwitcher()
        {
            return m_switcher;
        }
    };

    class ViewportManagerWrapper
    {
    public:
        void Create()
        {
            m_viewportManager = AZStd::make_unique<ViewportUiManagerTestable>();
            m_viewportManager->ConnectViewportUiBus(AzToolsFramework::ViewportUi::DefaultViewportId);
            m_mockRenderOverlay = AZStd::make_unique<QWidget>();
            m_parentWidget = AZStd::make_unique<QWidget>();
            m_viewportManager->InitializeViewportUi(m_parentWidget.get(), m_mockRenderOverlay.get());
            m_componentModeSwitcher = AZStd::make_unique<ComponentModeSwitcherTestable>();

        }

        void Destroy()
        {
            m_viewportManager->DisconnectViewportUiBus();
            m_viewportManager.reset();
            m_mockRenderOverlay.reset();
            m_parentWidget.reset();
        }

        ViewportUiManagerTestable* GetViewportManager()
        {
            return m_viewportManager.get();
        }

        ComponentModeSwitcherTestable* GetComponentModeSwitcher()
        {
            return m_componentModeSwitcher.get();
        }

        QWidget* GetMockRenderOverlay()
        {
            return m_mockRenderOverlay.get();
        }

    private:
        AZStd::unique_ptr<ViewportUiManagerTestable> m_viewportManager;
        AZStd::unique_ptr<ComponentModeSwitcherTestable> m_componentModeSwitcher;
        AZStd::unique_ptr<QWidget> m_parentWidget;
        AZStd::unique_ptr<QWidget> m_mockRenderOverlay;
    };

    // sets up a parent widget and render overlay to attach the Viewport UI to
    // as well as a cluster with one button

    /*class ComponentModeSwitcherTestFixture : public::testing::Test
    {
    public:
        ComponentModeSwitcherTestFixture() = default;

        ViewportManagerWrapper m_viewportManagerWrapper;

        void SetUp() override
        {
            m_viewportManagerWrapper.Create();
        }

        void TearDown() override
        {
            m_viewportManagerWrapper.Destroy();
        }
    };*/

    class ComponentModeSwitcherTestFixture : public ComponentModeTestFixture
    {
    };
 

    /*TEST_F(ComponentModeSwitcherTestFixture, ComponentModeSwitcherCreatesSwitcher)
    {
        auto switcherId = m_viewportManagerWrapper.GetComponentModeSwitcher()->GetSwitcher().m_switcherId;
        auto switcherEntry = m_viewportManagerWrapper.GetViewportManager()->GetSwitcherMap().find(switcherId);

        EXPECT_TRUE(switcherEntry != m_viewportManagerWrapper.GetViewportManager()->GetSwitcherMap().end());
        EXPECT_TRUE(switcherEntry->second.get() != nullptr);
    }*/

    TEST_F(ComponentModeSwitcherTestFixture, AddComponentModeComponentAddsComponentToSwitcher)
    {
        AZStd::shared_ptr<ComponentModeSwitcher> a = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(), &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher, a);
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();

        // add two placeholder Components (each with their own Component Mode)
        const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        const AZ::Component* placeholder2 = entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        //const AZ::Component* placeholder2 = entity->CreateComponent<PlaceholderEditorComponent>();

        entity->Activate();
        // mimic selecting the entity in the viewport (after selection the ComponentModeDelegate
        // connects to the ComponentModeDelegateRequestBus on the entity/component pair address)
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);
        //auto componentModeSwitcher = m_viewportManagerWrapper.GetComponentModeSwitcher();
        
        
        const auto pairId = AZ::EntityComponentIdPair(entityId, placeholder1->GetId());
        a->AddComponentButton(pairId);

        EXPECT_TRUE(a->GetComponentCount() == 1);

        const auto pairId2 = AZ::EntityComponentIdPair(entityId, placeholder2->GetId());
        a->AddComponentButton(pairId2);

        EXPECT_TRUE(a->GetComponentCount() == 2);
    }

   /* TEST_F(ComponentModeSwitcherTestFixture, RemoveComponentModeComponent)
    {
    }

    TEST_F(ComponentModeSwitcherTestFixture, Update)
    {
    }

    TEST_F(ComponentModeSwitcherTestFixture, Update)
    {
    }*/


} // namespace UnitTest
