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
    constexpr AZStd::string_view parentRowOrigin = "/0/3";
    constexpr AZStd::string_view translateRowOrigin = "/0/3/2";

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
            // <Row>
            //   <PropertyEditor Type="PrefabOverrideLabel" Text="Translate" IsOverridden="true" />
            //   <PropertyEditor ValueType="Vector3" Value=... />
            // </Row>
            AZ::Dom::Path pathToTranslateRow(translateRowOrigin);
            AZ::Dom::Value translateRow = adapterContents[pathToTranslateRow];
            EXPECT_EQ(translateRow.GetType(), AZ::Dom::Type::Node);
            EXPECT_EQ(translateRow.ArraySize(), 2);

            AZ::Dom::Value labelPropertyEditor = translateRow[0];
            EXPECT_EQ(labelPropertyEditor[PropertyEditor::Type.GetName()].GetString(), PrefabOverrideLabel::Name);
            EXPECT_EQ(labelPropertyEditor[PrefabOverrideLabel::Text.GetName()].GetString(), "Translate");
            EXPECT_TRUE(labelPropertyEditor[PrefabOverrideLabel::IsOverridden.GetName()].GetBool());

            AZ::Dom::Value valuePropertyEditor = translateRow[1];
            EXPECT_EQ(valuePropertyEditor[PropertyEditor::Value.GetName()][0].GetDouble(), 10.0);

            // Validate the parent row has overridden data since child rows have overridden data.
            // <Row>
            //   <PropertyEditor Type="PrefabOverrideLabel" Text="Values" IsOverridden="true" />
            //   <Row> ... </Row>  <-- translate row
            // </Row>
            AZ::Dom::Path pathToParentRow(parentRowOrigin);
            AZ::Dom::Value parentRow = adapterContents[pathToParentRow];
            EXPECT_EQ(parentRow.GetType(), AZ::Dom::Type::Node);

            AZ::Dom::Value labelPropertyEditorInParentRow = adapterContents[pathToParentRow][0];
            EXPECT_EQ(labelPropertyEditorInParentRow[PropertyEditor::Type.GetName()].GetString(), PrefabOverrideLabel::Name);
            EXPECT_EQ(labelPropertyEditorInParentRow[PrefabOverrideLabel::Text.GetName()].GetString(), "Values");
            EXPECT_TRUE(labelPropertyEditorInParentRow[PrefabOverrideLabel::IsOverridden.GetName()].GetBool());
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
            AZ::Dom::Path pathToTranslateRow(translateRowOrigin);
            AZ::Dom::Value translateRow = adapterContents[pathToTranslateRow];
            EXPECT_EQ(translateRow.GetType(), AZ::Dom::Type::Node);
            EXPECT_EQ(translateRow.ArraySize(), 2);

            AZ::Dom::Value labelPropertyEditor = translateRow[0];
            EXPECT_EQ(labelPropertyEditor[PropertyEditor::Type.GetName()].GetString(), PrefabOverrideLabel::Name);
            EXPECT_EQ(labelPropertyEditor[PrefabOverrideLabel::Text.GetName()].GetString(), "Translate");
            EXPECT_FALSE(labelPropertyEditor[PrefabOverrideLabel::IsOverridden.GetName()].GetBool());

            AZ::Dom::Value valuePropertyEditor = translateRow[1];
            EXPECT_EQ(valuePropertyEditor[PropertyEditor::Value.GetName()][0].GetDouble(), 10.0);

            // Validate the parent row has no overridden data since child rows have no overridden data.
            AZ::Dom::Path pathToParentRow(parentRowOrigin);
            AZ::Dom::Value parentRow = adapterContents[pathToParentRow];
            EXPECT_EQ(parentRow.GetType(), AZ::Dom::Type::Node);

            AZ::Dom::Value labelPropertyEditorInParentRow = adapterContents[pathToParentRow][0];
            EXPECT_EQ(labelPropertyEditorInParentRow[PropertyEditor::Type.GetName()].GetString(), PrefabOverrideLabel::Name);
            EXPECT_EQ(labelPropertyEditorInParentRow[PrefabOverrideLabel::Text.GetName()].GetString(), "Values");
            EXPECT_FALSE(labelPropertyEditorInParentRow[PrefabOverrideLabel::IsOverridden.GetName()].GetBool());
        };

        ValidateComponentEditorDomContents(callback);
    }

} // namespace UnitTest
