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
        system->RegisterNode<NodeWithVisiblityControl>();
        system->RegisterNodeAttribute<NodeWithVisiblityControl>(NodeWithVisiblityControl::Visibility);

        system->RegisterNode<Adapter, NodeWithVisiblityControl>();

        system->RegisterNode<Row, NodeWithVisiblityControl>();
        system->RegisterNodeAttribute<Row>(Row::AutoExpand);
        system->RegisterNodeAttribute<Row>(Row::ForceAutoExpand);

        system->RegisterNode<Label, NodeWithVisiblityControl>();
        system->RegisterNodeAttribute<Label>(Label::Value);

        system->RegisterNode<PropertyEditor, NodeWithVisiblityControl>();
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::OnChanged);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::Type);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::Value);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::ValueType);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumType);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumUnderlyingType);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumValue<char>);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumValue<AZ::s8>);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumValue<AZ::u8>);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumValue<AZ::s16>);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumValue<AZ::u16>);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumValue<int>);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumValue<AZ::s32>);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumValue<AZ::u32>);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumValue<AZ::s64>);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumValue<AZ::u64>);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumValues);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::ChangeNotify);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::AddNotify);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::RemoveNotify);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::ClearNotify);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::ValueHashed);

        system->RegisterPropertyEditor<UIElement>();
        system->RegisterNodeAttribute<UIElement>(UIElement::Handler);

        system->RegisterPropertyEditor<NumericEditor<>>();
        system->RegisterNodeAttribute<NumericEditor<>>(NumericEditor<>::Min);
        system->RegisterNodeAttribute<NumericEditor<>>(NumericEditor<>::Max);
        system->RegisterNodeAttribute<NumericEditor<>>(NumericEditor<>::Step);
        system->RegisterNodeAttribute<NumericEditor<>>(NumericEditor<>::Suffix);
        system->RegisterNodeAttribute<NumericEditor<>>(NumericEditor<>::SoftMin);
        system->RegisterNodeAttribute<NumericEditor<>>(NumericEditor<>::SoftMax);
        system->RegisterNodeAttribute<NumericEditor<>>(NumericEditor<>::Decimals);
        system->RegisterNodeAttribute<NumericEditor<>>(NumericEditor<>::DisplayDecimals);

        system->RegisterPropertyEditor<Slider<>, NumericEditor<>>();
        system->RegisterPropertyEditor<SpinBox<>, NumericEditor<>>();

        system->RegisterPropertyEditor<Button>();
        system->RegisterNodeAttribute<Button>(Button::ButtonText);

        system->RegisterPropertyEditor<ContainerActionButton>();
        system->RegisterNodeAttribute<ContainerActionButton>(ContainerActionButton::Action);
        system->RegisterNodeAttribute<ContainerActionButton>(ContainerActionButton::OnActivate);

        system->RegisterPropertyEditor<CheckBox>();
        system->RegisterPropertyEditor<Color>();
        system->RegisterPropertyEditor<ComboBox>();
        system->RegisterNodeAttribute<ComboBox>(ComboBox::StringList);

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
        system->RegisterPropertyEditor<FilePath>();
        system->RegisterPropertyEditor<Asset>();
        system->RegisterPropertyEditor<AudioControl>();
    }
} // namespace AZ::DocumentPropertyEditor::Nodes
