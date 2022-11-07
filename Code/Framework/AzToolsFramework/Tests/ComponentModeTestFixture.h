/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>

#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class ComponentModeTestFixture
        : public ToolsApplicationFixture
        , public AzToolsFramework::EditorDisabledCompositionRequestBus::Handler
    {
    public:
        // EditorDisabledCompositionRequestBus overrides ...
        void GetDisabledComponents(AZStd::vector<AZ::Component*>& components) override;
        void AddDisabledComponent(AZ::Component* componentToAdd) override;
        void RemoveDisabledComponent(AZ::Component* componentToRemove) override;

        void Connect(AZ::EntityId entityId);
        void Disconnect();
        void AddDisabledComponentToBus(AZ::Component*);

    protected:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;
        
        AZ::EntityId m_connectedEntity;
        AZ::Component* m_disabledComponent = nullptr;
    };
} // namespace UnitTest
