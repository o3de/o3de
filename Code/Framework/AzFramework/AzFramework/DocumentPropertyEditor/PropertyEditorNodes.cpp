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
        system->RegisterNodeAttribute<NodeWithVisiblityControl>(NodeWithVisiblityControl::ReadOnly);
        system->RegisterNodeAttribute<NodeWithVisiblityControl>(NodeWithVisiblityControl::NameLabelOverride);
        system->RegisterNodeAttribute<NodeWithVisiblityControl>(NodeWithVisiblityControl::SetTrueLabel);
        system->RegisterNodeAttribute<NodeWithVisiblityControl>(NodeWithVisiblityControl::SetFalseLabel);

        system->RegisterNode<Adapter, NodeWithVisiblityControl>();

        system->RegisterNode<Row, NodeWithVisiblityControl>();
        system->RegisterNodeAttribute<Row>(Row::AutoExpand);
        system->RegisterNodeAttribute<Row>(Row::ForceAutoExpand);

        system->RegisterNode<Label, NodeWithVisiblityControl>();
        system->RegisterNodeAttribute<Label>(Label::Value);
        system->RegisterNodeAttribute<Label>(Label::ValueText);

        system->RegisterNode<PropertyEditor, NodeWithVisiblityControl>();
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::OnChanged);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::Type);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::Value);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::ValueType);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumType);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::EnumUnderlyingType);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::InternalEnumValueKey);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::ChangeNotify);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::ChangeValidate);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::ValueHashed);
        system->RegisterNodeAttribute<PropertyEditor>(PropertyEditor::ParentValue);

        system->RegisterNode<Container>();
        system->RegisterNodeAttribute<PropertyEditor>(Container::AddNotify);
        system->RegisterNodeAttribute<PropertyEditor>(Container::RemoveNotify);
        system->RegisterNodeAttribute<PropertyEditor>(Container::ClearNotify);
        system->RegisterNodeAttribute<PropertyEditor>(Container::ContainerCanBeModified);
        system->RegisterNodeAttribute<PropertyEditor>(Container::IndexedChildNameLabelOverride);

        system->RegisterPropertyEditor<UIElement>();
        system->RegisterNodeAttribute<UIElement>(UIElement::Handler);

        system->RegisterPropertyEditor<DoubleNumericEditor>();
        system->RegisterNodeAttribute<DoubleNumericEditor>(DoubleNumericEditor::Min);
        system->RegisterNodeAttribute<DoubleNumericEditor>(DoubleNumericEditor::Max);
        system->RegisterNodeAttribute<DoubleNumericEditor>(DoubleNumericEditor::Step);
        system->RegisterNodeAttribute<DoubleNumericEditor>(DoubleNumericEditor::Suffix);
        system->RegisterNodeAttribute<DoubleNumericEditor>(DoubleNumericEditor::SoftMin);
        system->RegisterNodeAttribute<DoubleNumericEditor>(DoubleNumericEditor::SoftMax);
        system->RegisterNodeAttribute<DoubleNumericEditor>(DoubleNumericEditor::Decimals);
        system->RegisterNodeAttribute<DoubleNumericEditor>(DoubleNumericEditor::DisplayDecimals);

        system->RegisterPropertyEditor<IntNumericEditor>();
        system->RegisterNodeAttribute<IntNumericEditor>(IntNumericEditor::Min);
        system->RegisterNodeAttribute<IntNumericEditor>(IntNumericEditor::Max);
        system->RegisterNodeAttribute<IntNumericEditor>(IntNumericEditor::Step);
        system->RegisterNodeAttribute<IntNumericEditor>(IntNumericEditor::Suffix);
        system->RegisterNodeAttribute<IntNumericEditor>(IntNumericEditor::SoftMin);
        system->RegisterNodeAttribute<IntNumericEditor>(IntNumericEditor::SoftMax);
        system->RegisterNodeAttribute<IntNumericEditor>(IntNumericEditor::Decimals);
        system->RegisterNodeAttribute<IntNumericEditor>(IntNumericEditor::DisplayDecimals);

        system->RegisterPropertyEditor<UintNumericEditor>();
        system->RegisterNodeAttribute<UintNumericEditor>(UintNumericEditor::Min);
        system->RegisterNodeAttribute<UintNumericEditor>(UintNumericEditor::Max);
        system->RegisterNodeAttribute<UintNumericEditor>(UintNumericEditor::Step);
        system->RegisterNodeAttribute<UintNumericEditor>(UintNumericEditor::Suffix);
        system->RegisterNodeAttribute<UintNumericEditor>(UintNumericEditor::SoftMin);
        system->RegisterNodeAttribute<UintNumericEditor>(UintNumericEditor::SoftMax);
        system->RegisterNodeAttribute<UintNumericEditor>(UintNumericEditor::Decimals);
        system->RegisterNodeAttribute<UintNumericEditor>(UintNumericEditor::DisplayDecimals);

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

        system->RegisterPropertyEditor<GenericButton>();
        system->RegisterNodeAttribute<GenericButton>(GenericButton::OnActivate);

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
        system->RegisterNodeAttribute<LineEdit>(LineEdit::PlaceholderText);

        system->RegisterPropertyEditor<MultiLineEdit>();
        system->RegisterNodeAttribute<MultiLineEdit>(MultiLineEdit::PlaceholderText);

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
