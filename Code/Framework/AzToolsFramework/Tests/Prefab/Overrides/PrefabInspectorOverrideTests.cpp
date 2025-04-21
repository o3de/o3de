/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabPropertyEditorNodes.h>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <Prefab/Overrides/PrefabInspectorOverrideTestFixture.h>

namespace UnitTest
{
    using PrefabInspectorOverrideTest = PrefabInspectorOverrideTestFixture;

    // These index paths depend on multiple factors like the data in the component, how its reflected to serialize and edit contexts,
    // how different DPE adapters likes ReflectionAdapter and PrefabAdapter construct the DPE DOM etc. Therefore, these may
    // change in the future if the data stored in the DPE DOM itself changes and need to be modified accordingly to prevent test failures.
    constexpr AZStd::string_view pathToTranslateRow = "/1/2";

    TEST_F(PrefabInspectorOverrideTest, ValidatePresenceOfOverrideProperty)
    {
        AZ::EntityId newEntityId, parentContainerId, grandparentContainerId;
        CreateEntityInNestedPrefab(newEntityId, parentContainerId, grandparentContainerId);

        CreateAndValidateEditEntityOverride(newEntityId, grandparentContainerId);

        GenerateComponentAdapterDoms(newEntityId);

        AzToolsFramework::ComponentEditor::VisitComponentAdapterContentsCallback callback = [](const AZ::Dom::Value& adapterContents)
        {
            using AZ::DocumentPropertyEditor::Nodes::PropertyEditor;
            using AzToolsFramework::Prefab::PrefabPropertyEditorNodes::PrefabOverrideLabel;

            EXPECT_FALSE(adapterContents.IsArrayEmpty());

            // Validate the translate row has overridden data.
            AZ::Dom::Path domPathToTranslateRow(pathToTranslateRow);
            AZ::Dom::Value translateRow = adapterContents[domPathToTranslateRow];
            EXPECT_EQ(translateRow.GetType(), AZ::Dom::Type::Node);
            EXPECT_EQ(translateRow.ArraySize(), 2);

            AZ::Dom::Value labelPropertyEditor = translateRow[0];
            EXPECT_EQ(labelPropertyEditor[PropertyEditor::Type.GetName()].GetString(), PrefabOverrideLabel::Name);
            EXPECT_EQ(labelPropertyEditor[PrefabOverrideLabel::Value.GetName()].GetString(), "Translate");
            EXPECT_FALSE(labelPropertyEditor[PrefabOverrideLabel::RelativePath.GetName()].GetString().empty());
            EXPECT_FALSE(labelPropertyEditor[PrefabOverrideLabel::RevertOverride.GetName()].IsNull());

            EXPECT_TRUE(labelPropertyEditor[PrefabOverrideLabel::IsOverridden.GetName()].GetBool());

            AZ::Dom::Value valuePropertyEditor = translateRow[1];
            EXPECT_EQ(valuePropertyEditor[PropertyEditor::Value.GetName()][0].GetDouble(), 10.0);
        };

        ValidateComponentEditorDomContents(callback);
    }

    TEST_F(PrefabInspectorOverrideTest, ValidateAbsenceOfOverrideProperty)
    {
        AZ::EntityId newEntityId, parentContainerId, grandparentContainerId;
        CreateEntityInNestedPrefab(newEntityId, parentContainerId, grandparentContainerId);

        EditEntityAndValidateNoOverride(newEntityId);
        m_prefabFocusPublicInterface->FocusOnOwningPrefab(grandparentContainerId);

        GenerateComponentAdapterDoms(newEntityId);

        AzToolsFramework::ComponentEditor::VisitComponentAdapterContentsCallback callback = [](const AZ::Dom::Value& adapterContents)
        {
            using AZ::DocumentPropertyEditor::Nodes::PropertyEditor;
            using AzToolsFramework::Prefab::PrefabPropertyEditorNodes::PrefabOverrideLabel;

            EXPECT_FALSE(adapterContents.IsArrayEmpty());

            // Validate the translate row has no overridden data.
            AZ::Dom::Path domPathToTranslateRow(pathToTranslateRow);
            AZ::Dom::Value translateRow = adapterContents[domPathToTranslateRow];
            EXPECT_EQ(translateRow.GetType(), AZ::Dom::Type::Node);
            EXPECT_EQ(translateRow.ArraySize(), 2);

            AZ::Dom::Value labelPropertyEditor = translateRow[0];
            EXPECT_EQ(labelPropertyEditor[PropertyEditor::Type.GetName()].GetString(), PrefabOverrideLabel::Name);
            EXPECT_EQ(labelPropertyEditor[PrefabOverrideLabel::Value.GetName()].GetString(), "Translate");
            EXPECT_FALSE(labelPropertyEditor[PrefabOverrideLabel::RelativePath.GetName()].GetString().empty());
            EXPECT_FALSE(labelPropertyEditor[PrefabOverrideLabel::RevertOverride.GetName()].IsNull());

            EXPECT_FALSE(labelPropertyEditor[PrefabOverrideLabel::IsOverridden.GetName()].GetBool());

            AZ::Dom::Value valuePropertyEditor = translateRow[1];
            EXPECT_EQ(valuePropertyEditor[PropertyEditor::Value.GetName()][0].GetDouble(), 10.0);
        };

        ValidateComponentEditorDomContents(callback);
    }

} // namespace UnitTest
