/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystemInterface.h>

namespace AZ::DocumentPropertyEditor::Nodes
{
    bool Adapter::CanAddToParentNode([[maybe_unused]] const Dom::Value& parentNode)
    {
        // Adapters are root nodes, they can't be parented to anything
        return false;
    }

    bool Adapter::CanBeParentToValue(const Dom::Value& value)
    {
        // Adapters can only be parents to rows
        return value.IsNode() && value.GetNodeName() == GetNodeName<Row>();
    }

    bool Row::CanAddToParentNode(const Dom::Value& parentNode)
    {
        // Rows may only be children of other rows or the Adapter element
        const AZ::Name nodeName = parentNode.GetNodeName();
        return nodeName == GetNodeName<Row>() || nodeName == GetNodeName<Adapter>();
    }

    bool Row::CanBeParentToValue(const Dom::Value& value)
    {
        // Rows may only contain nodes, not arbitrary values
        return value.IsNode();
    }

    void Reflect(PropertyEditorSystemInterface* system)
    {
        system->RegisterNode<Adapter>();
        system->RegisterNode<Row>();
        system->RegisterNode<Label>();
        system->RegisterNode<PropertyEditor>();
        system->RegisterAttribute<PropertyEditor>(PropertyEditor::OnChanged);
        system->RegisterAttribute<PropertyEditor>(PropertyEditor::Type);

        system->RegisterPropertyEditor<NumericEditor<>>();
        system->RegisterAttribute<NumericEditor<>>(NumericEditor<>::Min);
        system->RegisterAttribute<NumericEditor<>>(NumericEditor<>::Max);
        system->RegisterAttribute<NumericEditor<>>(NumericEditor<>::Step);
        system->RegisterAttribute<NumericEditor<>>(NumericEditor<>::Suffix);
        system->RegisterAttribute<NumericEditor<>>(NumericEditor<>::SoftMin);
        system->RegisterAttribute<NumericEditor<>>(NumericEditor<>::SoftMax);
        system->RegisterAttribute<NumericEditor<>>(NumericEditor<>::Decimals);
        system->RegisterAttribute<NumericEditor<>>(NumericEditor<>::DisplayDecimals);

        system->RegisterPropertyEditor<Slider<>, NumericEditor<>>();
        system->RegisterPropertyEditor<SpinBox<>, NumericEditor<>>();
        system->RegisterPropertyEditor<Button>();
        system->RegisterPropertyEditor<CheckBox>();
        system->RegisterPropertyEditor<Color>();
        system->RegisterPropertyEditor<ComboBox>();
        system->RegisterPropertyEditor<RadioButton>();
        system->RegisterPropertyEditor<EntityId>();
        system->RegisterPropertyEditor<LayoutPadding>();
        system->RegisterPropertyEditor<LineEdit>();
        system->RegisterPropertyEditor<MultiLineEdit>();
        system->RegisterPropertyEditor<Quaternion>();
        system->RegisterPropertyEditor<Crc>();
        system->RegisterPropertyEditor<Vector2>();
        system->RegisterPropertyEditor<Vector3>();
        system->RegisterPropertyEditor<Vector4>();
    }
} // namespace AZ::DocumentPropertyEditor::Nodes
