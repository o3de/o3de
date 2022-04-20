/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentSchema.h>

namespace AZ::DocumentPropertyEditor::Nodes
{
    //! Adapter: The top-level tag for a DocumentAdapter that may contain any number of Rows.
    struct Adapter : NodeDefinition
    {
        static constexpr AZStd::string_view Name = "Adapter";
        static bool CanAddToParentNode(const Dom::Value& parentNode);
        static bool CanBeParentToValue(const Dom::Value& value);
    };

    //! Row: An adapter entry that may contain any number of child nodes or other Rows
    struct Row : NodeDefinition
    {
        static constexpr AZStd::string_view Name = "Row";
        static bool CanAddToParentNode(const Dom::Value& parentNode);
        static bool CanBeParentToValue(const Dom::Value& value);
    };

    //! Label: A textual label that shall render its contents as part of a Row.
    struct Label : NodeDefinition
    {
        static constexpr AZStd::string_view Name = "Label";
    };

    //! PropertyEditor: A property editor, of a type dictated by its "type" field,
    //! that can edit an associated value.
    struct PropertyEditor : NodeDefinition
    {
        static constexpr AZStd::string_view Name = "PropertyEditor";
        static constexpr auto Type = AttributeDefinition<AZStd::string_view>("Type");
        static constexpr auto OnChanged = CallbackAttributeDefinition<void(const Dom::Value&)>("OnChanged");
    };

    template<typename T>
    struct NumericEditor
    {
        static_assert(AZStd::is_floating_point_v<T> || AZStd::is_integral_v<T>, "Numeric editors must have a numeric type");
        static constexpr auto Min = AttributeDefinition<T>("Min");
        static constexpr auto Max = AttributeDefinition<T>("Max");
        static constexpr auto Step = AttributeDefinition<T>("Step");
        static constexpr auto Suffix = AttributeDefinition<AZStd::string_view>("Suffix");
        static constexpr auto SoftMin = AttributeDefinition<T>("SoftMin");
        static constexpr auto SoftMax = AttributeDefinition<T>("SoftMax");
        static constexpr auto Decimals = AttributeDefinition<int>("Decimals");
        static constexpr auto DisplayDecimals = AttributeDefinition<int>("DisplayDecimals");
    };

    struct Button : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "Button";
    };

    struct CheckBox : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "CheckBox";
    };

    struct Color : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "Color";
    };

    struct ComboBox : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "ComboBox";
    };

    struct RadioButton : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "RadioButton";
    };

    struct EntityId : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "EntityId";
    };

    struct LayoutPadding : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "LayoutPadding";
    };

    struct LineEdit : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "LineEdit";
    };

    struct MultiLineEdit : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "MultiLineEdit";
    };

    struct Quaternion : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "Quaternion";
    };

    template<typename T>
    struct Slider : NumericEditor<T>
    {
        static constexpr AZStd::string_view Name = "Slider";
    };

    template<typename T>
    struct SpinBox : NumericEditor<T>
    {
        static constexpr AZStd::string_view Name = "SpinBox";
    };

    struct Crc : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "Crc";
    };

    struct Vector2 : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "Vector2";
    };

    struct Vector3 : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "Vector3";
    };

    struct Vector4 : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "Vector4";
    };
} // namespace AZ::DocumentPropertyEditor::Nodes
