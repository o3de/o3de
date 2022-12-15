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

    //! PropertyVisibility: Provided for compatability with the RPE, determines whether an entry
    //! and/or its children should be visible.
    enum class PropertyVisibility : AZ::u32
    {
        Show = static_cast<AZ::u32>(AZ_CRC_CE("PropertyVisibility_Show")),
        ShowChildrenOnly = static_cast<AZ::u32>(AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly")),
        Hide = static_cast<AZ::u32>(AZ_CRC_CE("PropertyVisibility_Hide")),
        HideChildren = static_cast<AZ::u32>(AZ_CRC_CE("PropertyVisibility_HideChildren")),
    };

    //! Base class for nodes that have a Visibility attribute.
    struct NodeWithVisiblityControl : NodeDefinition
    {
        static constexpr AZStd::string_view Name = "NodeWithVisiblityControl";
        static constexpr auto Visibility = AttributeDefinition<PropertyVisibility>("Visibility");

        static constexpr auto Disabled = AttributeDefinition<bool>("Disabled");
        //! In some cases, a node may need to know that it is descended from a disabled ancestor. For example, disabled
        //! elements of a disabled container might require different treatment than disabled elements of an enabled container.
        static constexpr auto AncestorDisabled = AttributeDefinition<bool>("AncestorDisabled");
    };

    //! Adapter: The top-level tag for a DocumentAdapter that may contain any number of Rows.
    struct Adapter : NodeWithVisiblityControl
    {
        static constexpr AZStd::string_view Name = "Adapter";
        static constexpr auto QueryKey = CallbackAttributeDefinition<void(DocumentAdapterPtr*, AZ::Dom::Path)>("QueryKey");
        static constexpr auto AddContainerKey = CallbackAttributeDefinition<void(DocumentAdapterPtr*, AZ::Dom::Path)>("AddContainerKey");
        static constexpr auto RejectContainerKey = CallbackAttributeDefinition<void(DocumentAdapterPtr*, AZ::Dom::Path)>("RejectContainerKey");

        //! Use this callback attribute if there is need to enable/disable an adapter's nodes at runtime.
        static constexpr auto SetNodeDisabled =
            CallbackAttributeDefinition<void(bool shouldDisable, Dom::Path targetNode)>("SetDisabled");

        static bool CanAddToParentNode(const Dom::Value& parentNode);
        static bool CanBeParentToValue(const Dom::Value& value);
    };

    //! Row: An adapter entry that may contain any number of child nodes or other Rows
    struct Row : NodeWithVisiblityControl
    {
        static constexpr AZStd::string_view Name = "Row";
        static bool CanAddToParentNode(const Dom::Value& parentNode);
        static bool CanBeParentToValue(const Dom::Value& value);

        static constexpr auto AutoExpand = AttributeDefinition<bool>("AutoExpand");
        static constexpr auto ForceAutoExpand = AttributeDefinition<bool>("ForceAutoExpand");

        static constexpr AZStd::initializer_list<const AttributeDefinitionInterface*> RowAttributes = { &AutoExpand, &ForceAutoExpand };
    };

    //! PropertyRefreshLevel: Determines the amount of a property tree that needs to be rebuilt
    //! based on a change to a reflected property.
    enum class PropertyRefreshLevel : AZ::u32
    {
        Undefined = 0,
        None = static_cast<AZ::u32>(AZ_CRC_CE("RefreshNone")),
        ValuesOnly = static_cast<AZ::u32>(AZ_CRC_CE("RefreshValues")),
        AttributesAndValues = static_cast<AZ::u32>(AZ_CRC_CE("RefreshAttributesAndValues")),
        EntireTree = static_cast<AZ::u32>(AZ_CRC_CE("RefreshEntireTree")),
    };

    //! Label: A textual label that shall render its contents as part of a Row.
    struct Label : NodeWithVisiblityControl
    {
        static constexpr AZStd::string_view Name = "Label";
        static constexpr auto Value = AttributeDefinition<AZStd::string_view>("Value");
    };

    //! Specifies types describing a value change's state.
    //! Used to determine whether a value update is suitable for expensive operations like updating the undo stack or to
    //! otherwise notify interested parties that a property's value has been changed, or is being changed, by a property editor.
    enum class ValueChangeType
    {
        //! This is a "live", in-progress edit, and additional updates may follow at an arbitrarily fast rate.
        InProgressEdit,
        //! This is a "final" edit provided by the user doing something to signal a decision
        //! e.g. releasing the mouse or pressing enter.
        FinishedEdit,
    };

    //! PropertyEditor: A property editor, of a type dictated by its "type" field,
    //! that can edit an associated value.
    struct PropertyEditor : NodeWithVisiblityControl
    {
        static constexpr AZStd::string_view Name = "PropertyEditor";
        static constexpr auto Description = AttributeDefinition<AZStd::string_view>("Description");
        static constexpr auto Type = AttributeDefinition<AZStd::string_view>("Type");
        static constexpr auto OnChanged = CallbackAttributeDefinition<void(const Dom::Value&, ValueChangeType)>("OnChanged");
        static constexpr auto Value = AttributeDefinition<AZ::Dom::Value>("Value");
        static constexpr auto ValueType = TypeIdAttributeDefinition("ValueType");
        static constexpr auto ValueHashed = AttributeDefinition<AZ::Uuid>("ValueHashed");

        //! If set to true, specifies that this PropertyEditor shouldn't be allocated its own column, but instead appended
        //! to the previous column in the layout, creating a SharedColumn that can hold many PropertyEditors.
        //! Useful for things like the "add container entry" button.
        static constexpr auto SharePriorColumn = AttributeDefinition<bool>("SharePriorColumn");

        //! DEPENDENT attribute - must be used inside a SharedColumn.
        //! If set to true, specifies that this PropertyEditor should only take up as much space as its minimum width.
        //! Useful for placing things like "add container entry" and "remove all elements" next to each other
        static constexpr auto UseMinimumWidth = AttributeDefinition<bool>("UseMinimumWidth");

        //! Specifies the alignment options for a PropertyEditor that has the Alignment attribute.
        enum class Align : AZ::u8
        {
            AlignLeft,
            AlignRight,
            AlignCenter,
            UseDefaultAlignment
        };
        //! Specifies that this PropertyEditor should have a specific alignment within its own column. The alignment of ALL
        //! PropertyEditors inside of a SharedColumn will be the alignment of the last PropertyEditor with a valid alignment attribute.
        static constexpr auto Alignment = AttributeDefinition<Align>("Alignment");

        static constexpr auto EnumType = TypeIdAttributeDefinition("EnumType");
        static constexpr auto EnumUnderlyingType = TypeIdAttributeDefinition("EnumUnderlyingType");
        static constexpr auto EnumValue = AttributeDefinition<Dom::Value>("EnumValue");
        static constexpr auto EnumValues = EnumValuesAttributeDefinition("EnumValues");
        static constexpr auto ChangeNotify = CallbackAttributeDefinition<PropertyRefreshLevel()>("ChangeNotify");
        static constexpr auto RequestTreeUpdate = CallbackAttributeDefinition<void(PropertyRefreshLevel)>("RequestTreeUpdate");

        // Container attributes
        static constexpr auto AddNotify = CallbackAttributeDefinition<void()>("AddNotify");
        static constexpr auto RemoveNotify = CallbackAttributeDefinition<void(size_t)>("RemoveNotify");
        static constexpr auto ClearNotify = CallbackAttributeDefinition<void()>("ClearNotify");
    };

    struct UIElement : PropertyEditor
    {
        static constexpr AZStd::string_view Name = "UIElement";
        static constexpr auto Handler = NamedCrcAttributeDefinition("Handler");
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
        static constexpr auto ButtonText = AttributeDefinition<AZStd::string_view>("ButtonText");
    };

    enum class ContainerAction
    {
        AddElement,
        RemoveElement,
        Clear
    };

    struct ContainerActionButton : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "ContainerActionButton";
        static constexpr auto Action = AttributeDefinition<ContainerAction>("Action");
        static constexpr auto OnActivate = CallbackAttributeDefinition<void()>("OnActivate");
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
        static constexpr auto StringList = AttributeDefinition<AZStd::vector<AZStd::string>>("StringList");
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

    struct FilePath : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "FilePath";
    };

    struct Asset : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "Asset";
    };

    struct AudioControl : PropertyEditorDefinition
    {
        static constexpr AZStd::string_view Name = "AudioControl";
    };
} // namespace AZ::DocumentPropertyEditor::Nodes
