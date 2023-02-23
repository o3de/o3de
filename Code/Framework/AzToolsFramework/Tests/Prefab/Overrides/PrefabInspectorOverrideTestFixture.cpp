/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <Prefab/Overrides/PrefabInspectorOverrideTestFixture.h>

namespace UnitTest
{
    void PrefabInspectorOverrideTestFixture::SetUpEditorFixtureImpl()
    {
        // Enable feature flags for DPE and InspectorOverrideManagement
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        registry->Set("/O3DE/Preferences/Prefabs/EnableInspectorOverrideManagement", true);
        registry->Set("/O3DE/Autoexec/ConsoleCommands/ed_enableDPE", true);

        PrefabOverrideTestFixture::SetUpEditorFixtureImpl();

        m_testEntityPropertyEditor = AZStd::make_unique<AzToolsFramework::EntityPropertyEditor>(nullptr, Qt::WindowFlags(), false);
    }

    void PrefabInspectorOverrideTestFixture::TearDownEditorFixtureImpl()
    {
        m_testEntityPropertyEditor.reset();
        PrefabOverrideTestFixture::TearDownEditorFixtureImpl();
    }

    void PrefabInspectorOverrideTestFixture::GenerateComponentAdapterDoms(AZ::EntityId entityId)
    {
        // Calling UpdateContents will trigger the ReflectionAdapters of the components to build the DPE DOMs.
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, AzToolsFramework::EntityIdList{ entityId });
        QMetaObject::invokeMethod(m_testEntityPropertyEditor.get(), "UpdateContents", Qt::DirectConnection);
    }

    void PrefabInspectorOverrideTestFixture::ValidateComponentEditorDomContents(
        const AzToolsFramework::ComponentEditor::GetComponentAdapterContentsCallback& callback)
    {
        AZStd::vector<const AzToolsFramework::ComponentEditor*> componentEditors;

        AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
            &AzToolsFramework::EntityPropertyEditorRequestBus::Events::GetComponentEditors,
            [&componentEditors](const AzToolsFramework::ComponentEditor* componentEditor)
            {
                componentEditors.push_back(componentEditor);
                return true;
            });
        ASSERT_EQ(componentEditors.size(), 1);
        componentEditors.front()->GetComponentAdapterContents(callback);
    }

} // namespace UnitTest
