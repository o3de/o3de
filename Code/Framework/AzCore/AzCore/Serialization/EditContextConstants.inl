/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>

namespace AZ
{
    namespace Edit
    {
        namespace ClassElements
        {
            //! This is a catch-all container for attributes that affect how the entirety of a type is handled in the editor.
            //!
            //! Accepted Child Attributes:
            //! Many, see AZ::Edit::Attributes
            const static AZ::Crc32 EditorData = AZ_CRC_CE("EditorData");
            //! Where specified, all subsequent children will be grouped together in the UI under the name of the group
            //! which is specified as the class element description.
            //!
            //! Accepted Child Attributes:
            //! - AZ::Edit::Attributes::AutoExpand
            const static AZ::Crc32 Group = AZ_CRC_CE("Group");
            //! Specifies that the type contains a UIElement, a child property editor without a mapping to a specific member.
            //! This can be used to add UI components to the property editor that aren't just value editors, e.g. buttons.
            //!
            //! There is a convenience method, AZ::EditContext::ClassBuilder::UIComponent that allows specifying a UI component
            //! and its handler at the same time. This should be preferred to adding a UIComponent and calling ClassComponent.
            //!
            //! Accepted Child Attributes:
            //! - AZ::Edit::Attributes::Handler
            //! - AZ::Edit::Attributes::AcceptsMultiEdit
            const static AZ::Crc32 UIElement = AZ_CRC_CE("UIElement");
        }

        namespace Attributes
        {
            const static AZ::Crc32 EnableForAssetEditor = AZ_CRC_CE("EnableInAssetEditor");

            //! AddableByUser : a bool which determines if the component can be added by the user.
            //! Setting this to false effectively hides the component from views where user can create components.
            const static AZ::Crc32 AddableByUser = AZ_CRC_CE("AddableByUser");
            //! RemoveableByUser : A bool which determines if the component can be removed by the user.
            //! Setting this to false prevents the user from removing this component. Default behavior is removeable by user.
            const static AZ::Crc32 RemoveableByUser = AZ_CRC_CE("RemoveableByUser");
            //! An int which, if specified, causes a component to be forced to a particular position in the sorted list of
            //! components on an entity, and prevents dragging or moving operations which would affect that position.
            const static AZ::Crc32 FixedComponentListIndex = AZ_CRC_CE("FixedComponentListIndex");

            const static AZ::Crc32 AppearsInAddComponentMenu = AZ_CRC_CE("AppearsInAddComponentMenu");
            const static AZ::Crc32 ForceAutoExpand = AZ_CRC_CE("ForceAutoExpand"); // Ignores expansion state set by user, enforces expansion.
            const static AZ::Crc32 AutoExpand = AZ_CRC_CE("AutoExpand"); // Expands automatically unless user changes expansion state.
            const static AZ::Crc32 ButtonText = AZ_CRC_CE("ButtonText");
            const static AZ::Crc32 Category = AZ_CRC_CE("Category");
            const static AZ::Crc32 Visibility = AZ_CRC_CE("Visibility");
            const static AZ::Crc32 ButtonTooltip = AZ_CRC_CE("ButtonTooltip");
            const static AZ::Crc32 CheckboxTooltip = AZ_CRC_CE("CheckboxTooltip");
            const static AZ::Crc32 CheckboxDefaultValue = AZ_CRC_CE("CheckboxDefaultValue");
            //! Affects the display order of a node relative to it's parent/children.  Higher values display further down (after) lower values.  Default is 0, negative values are allowed.  Must be applied as an attribute to the EditorData element
            const static AZ::Crc32 DisplayOrder = AZ_CRC_CE("DisplayOrder");
            //! Specifies whether the UI should support multi-edit for aggregate instances of this property
            //! This is enabled by default for everything except UIElements, which require an explicit opt-in
            //!
            //! Element type to use this with: Any
            //! Expected value type: bool
            //! Default value: true for DataElements, false for UIElements
            const static AZ::Crc32 AcceptsMultiEdit = AZ_CRC_CE("AcceptsMultiEdit");

            //! Specifies a small, human readable string representation of an entire class for editor display.
            //! This is used to create a string representation of the class when displaying associative containers.
            //!
            //! Element type to use this with: Any type that can sensibly be represented to the user with a string.
            //! Expected value type: AZStd::string
            //! Default value: None
            const static AZ::Crc32 ConciseEditorStringRepresentation = AZ_CRC_CE("ConciseEditorStringRepresentation");

            //! Container attributes
            const static AZ::Crc32 ContainerCanBeModified = AZ_CRC_CE("ContainerCanBeModified");
            const static AZ::Crc32 ShowAsKeyValuePairs = AZ_CRC_CE("ShowAsKeyValuePairs");
            const static AZ::Crc32 StringList = AZ_CRC_CE("StringList");

            //! GenericComboBox Attributes
            const static AZ::Crc32 GenericValue = AZ_CRC_CE("GenericValue");
            const static AZ::Crc32 GenericValueList = AZ_CRC_CE("GenericValueList");
            const static AZ::Crc32 PostChangeNotify = AZ_CRC_CE("PostChangeNotify");

            const static AZ::Crc32 ValueText = AZ_CRC_CE("ValueText");
            const static AZ::Crc32 PlaceholderText = AZ_CRC_CE("PlaceholderText");

            const static AZ::Crc32 TrueText = AZ_CRC_CE("TrueText");
            const static AZ::Crc32 FalseText = AZ_CRC_CE("FalseText");

            const static AZ::Crc32 EnumValues = AZ_CRC_CE("EnumValues");

            //! Used to bind either a callback or a refresh mode to the changing of a particular property.
            const static AZ::Crc32 AddNotify = AZ_CRC_CE("AddNotify");
            const static AZ::Crc32 RemoveNotify = AZ_CRC_CE("RemoveNotify");
            const static AZ::Crc32 ChangeNotify = AZ_CRC_CE("ChangeNotify");
            const static AZ::Crc32 ClearNotify = AZ_CRC_CE("ClearNotify");

            //! Specifies a function to accept or reject a value changed in the Open 3D Engine Editor.
            //! For example, a component could reject AZ::EntityId values that reference its own entity.
            //!
            //! Element type to use this with:   Any type that you reflect using AZ::EditContext::ClassInfo::DataElement().
            //! Expected value type:             A function with signature `AZ::Outcome<void, AZStd::string> fn(void* newValue, const AZ::TypeId& valueType)`.
            //!                                      If the function returns failure, then the new value will not be applied.
            //!                                      `newValue` is a void* pointing at the new value being validated.
            //!                                      `valueType` is the type ID of the value pointed to by `newValue`.
            //! Default value:                   None
            const AZ::Crc32 ChangeValidate = AZ_CRC_CE("ChangeValidate");

            //! Used to bind a callback to the editing complete event of a string line edit control.
            const static AZ::Crc32 StringLineEditingCompleteNotify = AZ_CRC_CE("StringLineEditingCompleteNotify");

            const static AZ::Crc32 NameLabelOverride = AZ_CRC_CE("NameLabelOverride");
            const static AZ::Crc32 AssetPickerTitle = AZ_CRC_CE("AssetPickerTitle");
            const static AZ::Crc32 HideProductFilesInAssetPicker = AZ_CRC_CE("HideProductFilesInAssetPicker");
            const static AZ::Crc32 ChildNameLabelOverride = AZ_CRC_CE("ChildNameLabelOverride");
            //! Container attribute that is used to override labels for its elements given the index of the element
            const static AZ::Crc32 IndexedChildNameLabelOverride = AZ_CRC_CE("IndexedChildNameLabelOverride");
            const static AZ::Crc32 DescriptionTextOverride = AZ_CRC_CE("DescriptionTextOverride");
            const static AZ::Crc32 ContainerReorderAllow = AZ_CRC_CE("ContainerReorderAllow");

            const static AZ::Crc32 PrimaryAssetType = AZ_CRC_CE("PrimaryAssetType");
            const static AZ::Crc32 DynamicElementType = AZ_CRC_CE("DynamicElementType");
            //! Asset to show when the selection is empty
            const static AZ::Crc32 DefaultAsset = AZ_CRC_CE("DefaultAsset");
            //! Allow or disallow clearing the asset
            const static AZ::Crc32 AllowClearAsset = AZ_CRC_CE("AllowClearAsset");
            // Show the name of the asset that was produced from the source asset
            const static AZ::Crc32 ShowProductAssetFileName = AZ_CRC_CE("ShowProductAssetFileName");
            //! Regular expression pattern filter for source files
            const static AZ::Crc32 SourceAssetFilterPattern = AZ_CRC_CE("SourceAssetFilterPattern");

            //! Component icon attributes
            const static AZ::Crc32 Icon = AZ_CRC_CE("Icon");
            const static AZ::Crc32 CategoryStyle = AZ_CRC_CE("CategoryStyle");
            const static AZ::Crc32 ViewportIcon = AZ_CRC_CE("ViewportIcon");
            const static AZ::Crc32 HideIcon = AZ_CRC_CE("HideIcon");
            const static AZ::Crc32 PreferNoViewportIcon = AZ_CRC_CE("PreferNoViewportIcon");
            const static AZ::Crc32 DynamicIconOverride = AZ_CRC_CE("DynamicIconOverride");

            //! Data attributes
            const static AZ::Crc32 Min = AZ_CRC_CE("Min");
            const static AZ::Crc32 Max = AZ_CRC_CE("Max");
            const static AZ::Crc32 ReadOnly = AZ_CRC_CE("ReadOnly");
            const static AZ::Crc32 Step = AZ_CRC_CE("Step");
            const static AZ::Crc32 Suffix = AZ_CRC_CE("Suffix");
            const static AZ::Crc32 SoftMin = AZ_CRC_CE("SoftMin");
            const static AZ::Crc32 SoftMax = AZ_CRC_CE("SoftMax");
            const static AZ::Crc32 Decimals = AZ_CRC_CE("Decimals");
            const static AZ::Crc32 DisplayDecimals = AZ_CRC_CE("DisplayDecimals");

            const static AZ::Crc32 LabelForX = AZ_CRC_CE("LabelForX");
            const static AZ::Crc32 LabelForY = AZ_CRC_CE("LabelForY");
            const static AZ::Crc32 LabelForZ = AZ_CRC_CE("LabelForZ");
            const static AZ::Crc32 LabelForW = AZ_CRC_CE("LabelForW");

            const static AZ::Crc32 StyleForX = AZ_CRC_CE("StyleForX");
            const static AZ::Crc32 StyleForY = AZ_CRC_CE("StyleForY");
            const static AZ::Crc32 StyleForZ = AZ_CRC_CE("StyleForZ");
            const static AZ::Crc32 StyleForW = AZ_CRC_CE("StyleForW");
            
            const static AZ::Crc32 RequiredService = AZ_CRC_CE("RequiredService");
            const static AZ::Crc32 IncompatibleService = AZ_CRC_CE("IncompatibleService");

            const static AZ::Crc32 MaxLength = AZ_CRC_CE("MaxLength");

            //! Specifies the URL to load for a component
            //!
            //! **Element type to use this with:**   AZ::Edit::ClassElements::EditorData, which you reflect using
            //!                                      AZ::EditContext::ClassInfo::ClassElement().
            //!
            //! **Expected value type:**             `AZStd::string`
            //!
            //! **Default value:**                   None
            //!
            //! **Example:**                         The following example shows how to specify a help URL for a given component
            //!
            //! @code{.cpp}
            //! editContext->Class<ScriptEditorComponent>("Lua Script", "The Lua Script component allows you to add arbitrary Lua logic to an entity in the form of a Lua script")
            //!   ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            //!   ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/lua-script/")
            //! @endcode
            const static AZ::Crc32 HelpPageURL = AZ_CRC_CE("HelpPageURL");

            //! Combobox parameters.
            const static AZ::Crc32 ComboBoxEditable = AZ_CRC_CE("ComboBoxEditable");

            //! For use with slice creation tools. See SliceCreationFlags below for details.
            const static AZ::Crc32 SliceFlags = AZ_CRC_CE("SliceFlags");

            //! On controls with a clear button, this attribute specifies whether the clear button should be shown.
            const static AZ::Crc32 ShowClearButtonHandler = AZ_CRC_CE("ShowClearButtonHandler");

            //! Specifies whether the picker button should be shown.
            //! Authors - if you add this attribute to other handlers, list them here:
            //!      EntityId handler
            const static AZ::Crc32 ShowPickButton = AZ_CRC_CE("ShowPickButton");

            //! Specifies whether drop onto this control should be allowed.
            //! Used to turn off drag and drop for Transform Parents - you have to modify that via the outliner.
            //! Authors - if you add this attribute to other handlers, list them here:
            //!      EntityId handler
            const static AZ::Crc32 AllowDrop = AZ_CRC_CE("AllowDrop");

            //! For optional use on Getter Events used for Virtual Properties
            const static AZ::Crc32 PropertyPosition = AZ_CRC_CE("Position");
            const static AZ::Crc32 PropertyRotation = AZ_CRC_CE("Rotation");
            const static AZ::Crc32 PropertyScale = AZ_CRC_CE("Scale");
            const static AZ::Crc32 PropertyHidden = AZ_CRC_CE("Hidden");

            //! Specifies a vector<Crc32> of platform tags that must *all* be set on the current platform for the component to be exported.
            const static AZ::Crc32 ExportIfAllPlatformTags = AZ_CRC_CE("ExportIfAllPlatformTags");

            //! Specifies a vector<Crc32> of platform tags, of which at least one must be set on the current platform for the component to be exported.
            const static AZ::Crc32 ExportIfAnyPlatformTags = AZ_CRC_CE("ExportIfAnyPlatformTags");

            //! Binds to a function (static or member) to allow for dynamic (runtime) slice exporting of custom editor components.
            const static AZ::Crc32 RuntimeExportCallback = AZ_CRC_CE("RuntimeExportCallback");

            //! Attribute for storing a Id Generator function used by GenerateNewIdsAndFixRefs to remapping old id's to new id's
            const static AZ::Crc32 IdGeneratorFunction = AZ_CRC_CE("IdGeneratorFunction");

            //! Attribute for tagging a System Component for use in certain contexts
            const static AZ::Crc32 SystemComponentTags = AZ_CRC_CE("SystemComponentTags");
            
            //! Attribute for providing a custom UI Handler - can be used with Attribute() (or with ElementAttribute() for containers such as vectors, to specify the handler for container elements (i.e. vectors))
            const static AZ::Crc32 Handler = AZ_CRC_CE("Handler");

            //! Attribute for skipping a set amount of descendant elements which are not leaves when calculating property visibility
            const static AZ::Crc32 VisibilitySkipNonLeafDepth = AZ_CRC_CE("VisibilitySkipNonLeafDepth");

            //! Attribute for making a slider have non-linear scale. The default is 0.5, which results in linear scale. Value can be shifted lower or higher to control more precision in the power curve at those ends (minimum = 0, maximum = 1)
            const static AZ::Crc32 SliderCurveMidpoint = AZ_CRC_CE("SliderCurveMidpoint");

            //! Attribute for binding a function that can convert the type being viewed to a string
            inline constexpr AZ::Crc32 ToString{ "ToString" };
        }


        namespace UIHandlers
        {
            //! Helper for explicitly designating the default UI handler.
            //! i.e. DataElement(DefaultHandler, field, ...)
            const static AZ::Crc32 Default = 0;

            const static AZ::Crc32 Button = AZ_CRC_CE("Button");
            const static AZ::Crc32 CheckBox = AZ_CRC_CE("CheckBox");
            const static AZ::Crc32 Color = AZ_CRC_CE("Color");
            const static AZ::Crc32 ComboBox = AZ_CRC_CE("ComboBox");
            const static AZ::Crc32 RadioButton = AZ_CRC_CE("RadioButton");
            const static AZ::Crc32 EntityId = AZ_CRC_CE("EntityId");
            const static AZ::Crc32 LayoutPadding = AZ_CRC_CE("LayoutPadding");
            const static AZ::Crc32 LineEdit = AZ_CRC_CE("LineEdit");
            const static AZ::Crc32 MultiLineEdit = AZ_CRC_CE("MultiLineEdit");
            const static AZ::Crc32 Quaternion = AZ_CRC_CE("Quaternion");
            const static AZ::Crc32 Slider = AZ_CRC_CE("Slider");
            const static AZ::Crc32 SpinBox = AZ_CRC_CE("SpinBox");
            const static AZ::Crc32 Crc = AZ_CRC_CE("Crc"); // for u32 representation of a CRC, only works on u32 values.
            const static AZ::Crc32 Vector2 = AZ_CRC_CE("Vector2");
            const static AZ::Crc32 Vector3 = AZ_CRC_CE("Vector3");
            const static AZ::Crc32 Vector4 = AZ_CRC_CE("Vector4");
            const static AZ::Crc32 ExeSelectBrowseEdit = AZ_CRC_CE("ExeSelectBrowseEdit");
            const static AZ::Crc32 Label = AZ_CRC_CE("Label");

            // Maintained in the UIHandlers namespace for backwards compatibility; moved to the Attributes namespace now
            const static AZ::Crc32 Handler = Attributes::Handler;
        }

        // Attributes used in internal implementation only
        namespace InternalAttributes
        {
            const static AZ::Crc32 EnumValue = AZ_CRC_CE("EnumValue");
            const static AZ::Crc32 EnumType = AZ_CRC_CE("EnumType");
            const static AZ::Crc32 ElementInstances = AZ_CRC_CE("ElementInstances");
        }

        //! Notifies the property system to refresh the property grid, along with the level of refresh.
        namespace PropertyRefreshLevels
        {
            const static AZ::Crc32 None = AZ_CRC_CE("RefreshNone");

            //! This will only update the values in each row that has a property
            const static AZ::Crc32 ValuesOnly = AZ_CRC_CE("RefreshValues");

            //! This will re-consume all attributes and values, with the exception of the
            //! Visibility attribute. This is due to the Visibility attribute being consumed
            //! at a higher level in the system and would be a more expensive operation that
            //! would essentially be the same as the EntireTree refresh level.
            const static AZ::Crc32 AttributesAndValues = AZ_CRC_CE("RefreshAttributesAndValues");

            //! Re-create the entire tree of properties.
            const static AZ::Crc32 EntireTree = AZ_CRC_CE("RefreshEntireTree");
        }

        //! Specifies the visibility setting for a particular property.
        namespace PropertyVisibility
        {
            const static AZ::Crc32 Show = AZ_CRC_CE("PropertyVisibility_Show");
            const static AZ::Crc32 ShowChildrenOnly = AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly");
            const static AZ::Crc32 Hide = AZ_CRC_CE("PropertyVisibility_Hide");
            const static AZ::Crc32 HideChildren = AZ_CRC_CE("PropertyVisibility_HideChildren");
        }

        namespace SliceFlags
        {
            const static AZ::u32 DontGatherReference        = 1 << 0;   ///< Slice creation should not auto-gather entities referenced through this field.
            const static AZ::u32 NotPushable                = 1 << 1;   ///< This field should not be pushed to slices. Can be bound as a dynamic attribute.
            const static AZ::u32 NotPushableOnSliceRoot     = 1 << 2;   ///< This field is not pushable on slice root entities.
            const static AZ::u32 PushWhenHidden             = 1 << 4;   ///< When not displayed in the Push widget and changes are present, push changes to the slice (useful for hidden editor-only components)
            const static AZ::u32 HideOnAdd                  = 1 << 5;   ///< When property/field/component class is being added to an entity, hide from Push Widget display
            const static AZ::u32 HideOnChange               = 1 << 6;   ///< When property/field/component class is being changed on an entity, hide from Push Widget display
            const static AZ::u32 HideOnRemove               = 1 << 7;   ///< When property/field/component class is being removed on an entity, hide from Push Widget display
            const static AZ::u32 HideAllTheTime             = 1 << 8;   ///< Hide property/field/component class from Push Widget display all the time
        }

        namespace UISliceFlags
        {
            // IMPORANT: Start at the first bit NOT used by SliceFlags above.
            const static AZ::u32 PushableEvenIfInvisible = 1 << 3;  ///< Deprecated - Used currently only by UI slices, and will be phased out in the future. Forces display of hidden fields on Push widget.
        }

    } // namespace Edit

} // namespace AZ
