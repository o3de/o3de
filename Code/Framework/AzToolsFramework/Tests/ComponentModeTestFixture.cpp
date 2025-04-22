/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentModeTestFixture.h"
#include "ComponentModeTestDoubles.h"

#include <AzCore/UserSettings/UserSettingsComponent.h>

namespace UnitTest
{
    void ComponentModeTestFixture::SetUpEditorFixtureImpl()
    {
        namespace AztfCmf = AzToolsFramework::ComponentModeFramework;

        auto* app = GetApplication();

        app->RegisterComponentDescriptor(AztfCmf::PlaceholderEditorComponent::CreateDescriptor());
        app->RegisterComponentDescriptor(AztfCmf::AnotherPlaceholderEditorComponent::CreateDescriptor());
        app->RegisterComponentDescriptor(AztfCmf::DependentPlaceholderEditorComponent::CreateDescriptor());
        app->RegisterComponentDescriptor(
            AztfCmf::TestComponentModeComponent<AztfCmf::OverrideMouseInteractionComponentMode>::CreateDescriptor());
        app->RegisterComponentDescriptor(AztfCmf::IncompatiblePlaceholderEditorComponent::CreateDescriptor());
    }

    void ComponentModeTestFixture::Connect(AZ::EntityId entityId)
    {
        AzToolsFramework::EditorDisabledCompositionRequestBus::Handler::BusConnect(entityId);
        m_connectedEntity = entityId;
    }

    void ComponentModeTestFixture::Disconnect()
    {
        if (m_connectedEntity.IsValid())
        {
            AzToolsFramework::EditorDisabledCompositionRequestBus::Handler::BusDisconnect();
            m_connectedEntity.SetInvalid();
        }
    }

    void ComponentModeTestFixture::TearDownEditorFixtureImpl()
    {
        Disconnect();
    }

    void ComponentModeTestFixture::GetDisabledComponents(AZ::Entity::ComponentArrayType& components)
    {
         if (m_disabledComponent != nullptr)
         {
             components.push_back(m_disabledComponent);
         }
    }

    void ComponentModeTestFixture::AddDisabledComponentToBus(AZ::Component* component)
    {
        if (component != nullptr)
        {
            m_disabledComponent = component;
        }
    }

    void ComponentModeTestFixture::AddDisabledComponent([[maybe_unused]] AZ::Component* componentToAdd){};
    void ComponentModeTestFixture::RemoveDisabledComponent([[maybe_unused]] AZ::Component* componentToRemove){};
    bool ComponentModeTestFixture::IsComponentDisabled([[maybe_unused]] const AZ::Component* component)
    {
        return false;
    };
} // namespace UnitTest
