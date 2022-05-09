/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentSchema.h>

namespace AZ::DocumentPropertyEditor
{
    class PropertyEditorSystemInterface;
}

namespace AZ::DocumentPropertyEditor::Nodes
{
    //! Reflection method, registers all nodes in this header to the runtime context.
    //! Be sure to update this if you change this file.
    void Reflect(PropertyEditorSystemInterface* system);

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
        static constexpr auto Value = AttributeDefinition<AZStd::string_view>("Value");
    };

    //! PropertyEditor: A property editor, of a type dictated by its "type" field,
    //! that can edit an associated value.
    struct PropertyEditor : NodeDefinition
    {
        static constexpr AZStd::string_view Name = "PropertyEditor";
        static constexpr auto Type = AttributeDefinition<AZStd::string_view>("Type");
        static constexpr auto OnChanged = CallbackAttributeDefinition<void(const Dom::Value&)>("OnChanged");
        static constexpr auto Value = AttributeDefinition<AZ::Dom::Value>("Value");
    };

    template<typename T = Dom::Value>
    struct NumericEditor : PropertyEditorDefinition
    {
        static_assert(
            AZStd::is_same_v<T, Dom::Value> || AZStd::is_floating_point_v<T> || AZStd::is_integral_v<T>,
            "Numeric editors must have a numeric type");
        using StorageType = AZStd::conditional_t<
            AZStd::is_same_v<T, Dom::Value>,
            Dom::Value,
            AZStd::conditional_t<AZStd::is_floating_point_v<T>, double, AZStd::conditional_t<AZStd::is_signed_v<T>, int64_t, uint64_t>>>;

        static constexpr AZStd::string_view Name = "NumericEditor";
        static constexpr auto Min = AttributeDefinition<StorageType>("Min");
        static constexpr auto Max = AttributeDefinition<StorageType>("Max");
        static constexpr auto Step = AttributeDefinition<StorageType>("Step");
        static constexpr auto Suffix = AttributeDefinition<AZStd::string_view>("Suffix");
        static constexpr auto SoftMin = AttributeDefinition<StorageType>("SoftMin");
        static constexpr auto SoftMax = AttributeDefinition<StorageType>("SoftMax");
        static constexpr auto Decimals = AttributeDefinition<int>("Decimals");
        static constexpr auto DisplayDecimals = AttributeDefinition<int>("DisplayDecimals");
    };
    using IntNumericEditor = NumericEditor<int64_t>;
    using UintNumericEditor = NumericEditor<uint64_t>;
    using DoubleNumericEditor = NumericEditor<double>;

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

    template<typename T = Dom::Value>
    struct Slider : NumericEditor<T>
    {
        static constexpr AZStd::string_view Name = "Slider";
    };
    using IntSlider = Slider<int64_t>;
    using UintSlider = Slider<uint64_t>;
    using DoubleSlider = Slider<double>;

    template<typename T = Dom::Value>
    struct SpinBox : NumericEditor<T>
    {
        static constexpr AZStd::string_view Name = "SpinBox";
    };
    using IntSpinBox = SpinBox<int64_t>;
    using UintSpinBox = SpinBox<uint64_t>;
    using DoubleSpinBox = SpinBox<double>;

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
