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

    TEST_F(PrefabInspectorOverrideTest, ValidatePresenceOfOverrideProperty)
    {
        AZ::EntityId newEntityId, parentContainerId, grandparentContainerId;
        CreateEntityInNestedPrefab(newEntityId, parentContainerId, grandparentContainerId);

        CreateAndValidateEditEntityOverride(newEntityId, grandparentContainerId);

        GenerateComponentAdapterDoms(newEntityId);

        AzToolsFramework::ComponentEditor::GetComponentAdapterContentsCallback callback = [](const AZ::Dom::Value& adapterContents)
        {
            EXPECT_FALSE(adapterContents.IsArrayEmpty());

            // This path depends on multiple factors like the data in the component, how its reflected to serialize and edit contexts,
            // how different DPE adapters likes ReflectionAdapter and PrefabAdapter construct the DPE DOM etc. Therefore, this may
            // change in the future if the data stored in the DPE DOM itself changes. This path must be updated if that happens.
            AZ::Dom::Path pathToTranslateProperty("/0/3/3");
            [[maybe_unused]] AZ::Dom::Value translateRow = adapterContents[pathToTranslateProperty];
            EXPECT_EQ(translateRow.GetType(), AZ::Dom::Type::Node);
            EXPECT_EQ(translateRow.ArraySize(), 3);

            // First comes the PrefabOverrideProperty which will be used later by the PrefabAdapter to put up the PrefabOverrideIcon.
            EXPECT_EQ(
                translateRow[0][AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Type.GetName()].GetString(),
                AzToolsFramework::Prefab::PrefabPropertyEditorNodes::PrefabOverrideProperty::Name);

            // Then comes the label of the row.
            EXPECT_EQ(translateRow[1][AZ::DocumentPropertyEditor::Nodes::Label::Value.GetName()].GetString(), "Translate");

            // Then comes the actual property editor of the translate vector.
            AZ::Dom::Value translateVectorInDom = translateRow[2][AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Value.GetName()];
            EXPECT_EQ(translateVectorInDom[0].GetDouble(), 10.0);
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

        AzToolsFramework::ComponentEditor::GetComponentAdapterContentsCallback callback = [](const AZ::Dom::Value& adapterContents)
        {
            EXPECT_FALSE(adapterContents.IsArrayEmpty());
            AZ::Dom::Path pathToTranslateProperty("/0/3/2");
            [[maybe_unused]] AZ::Dom::Value translateRow = adapterContents[pathToTranslateProperty];
            EXPECT_EQ(translateRow.GetType(), AZ::Dom::Type::Node);
            EXPECT_EQ(translateRow.ArraySize(), 2);
            EXPECT_EQ(translateRow[0][AZ::DocumentPropertyEditor::Nodes::Label::Value.GetName()].GetString(), "Translate");

            AZ::Dom::Value translateVectorInDom = translateRow[1][AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Value.GetName()];
            EXPECT_EQ(translateVectorInDom[0].GetDouble(), 10.0);
        };

        ValidateComponentEditorDomContents(callback);
    }

} // namespace UnitTest
