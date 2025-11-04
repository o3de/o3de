/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/Name.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AtomToolsFramework
{
    //! Configures the initial state, data type, attributes, and values that describe
    //! the dynamic property and how it is presented
    struct DynamicPropertyConfig
    {
        AZ_TYPE_INFO(DynamicPropertyConfig, "{9CA40E92-7F03-42BE-B6AA-51F30EE5796C}");
        AZ_CLASS_ALLOCATOR(DynamicPropertyConfig, AZ::SystemAllocator);

        AZ::Name m_id; //!< The full property ID, which will normally be "groupName.propertyName"
        AZStd::string m_name;
        AZStd::string m_displayName;
        AZStd::string m_groupName;
        AZStd::string m_groupDisplayName;
        AZStd::string m_description;
        AZStd::any m_defaultValue;
        AZStd::any m_parentValue;
        AZStd::any m_originalValue;
        AZStd::any m_min;
        AZStd::any m_max;
        AZStd::any m_softMin;
        AZStd::any m_softMax;
        AZStd::any m_step;
        AZStd::vector<AZStd::string> m_enumValues;
        AZStd::vector<AZStd::string> m_vectorLabels;
        bool m_visible = true;
        bool m_readOnly = false;
        bool m_showThumbnail = false;
        AZStd::function<AZ::u32(const AZStd::any&)> m_dataChangeCallback;
        AZStd::vector<AZ::Data::AssetType> m_supportedAssetTypes;
        AZ::u32 m_customHandler = 0;
    };

    //! Wraps an AZStd::any value and configuration so that it can be displayed and edited in a ReflectedPropertyEditor.
    //! Binds all of the data and attributes necessary to configure the controls used for editing in a ReflectedPropertyEditor.
    //! Does data validation for range-based properties like sliders and spin boxes.
    struct DynamicProperty
    {
        AZ_TYPE_INFO(DynamicProperty, "{B0E7DCC6-65D9-4F0C-86AE-AE768BC027F3}");
        AZ_CLASS_ALLOCATOR(DynamicProperty, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);
        static const AZ::Edit::ElementData* GetPropertyEditData(const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType);

        DynamicProperty() = default;
        DynamicProperty(const DynamicPropertyConfig& config);

        //! Set property value.
        void SetValue(const AZStd::any& value);

        //! Returns the current property value.
        const AZStd::any& GetValue() const;

        //! Set property config.
        void SetConfig(const DynamicPropertyConfig& config);

        //! Returns the current property value.
        const DynamicPropertyConfig& GetConfig() const;

        //! Rebuilds the dynamic edit data.
        void UpdateEditData();

        //! Returns true if the property has a valid value.
        bool IsValid() const;

        //! Returns the ID of the property.
        const AZ::Name GetId() const;

        //! Returns the current property visibility.
        AZ::Crc32 GetVisibility() const;

        //! Returns the current property read only state.
        bool IsReadOnly() const;

    private:
        // Functions used to configure edit data attributes.
        AZStd::string GetDisplayName() const;
        AZStd::string GetGroupDisplayName() const;
        AZStd::string GetAssetPickerTitle() const;
        AZStd::string GetDescription() const;
        AZStd::vector<AZ::Edit::EnumConstant<uint32_t>> GetEnumValues() const;

        // Handles changes from the ReflectedPropertyEditor and sends notification.
        AZ::u32 OnDataChanged() const;

        bool CheckRangeMetaDataValuesForType(const AZ::Uuid& expectedTypeId) const;
        bool CheckRangeMetaDataValues() const;
        bool IsValueInteger() const;

        // Registers attributes with the dynamic edit data that will be used to configure the ReflectedPropertyEditor.
        template<typename AttributeValueType>
        void AddEditDataAttribute(AZ::Crc32 crc, AttributeValueType attribute);
        template<typename AttributeMemberFunctionType>
        void AddEditDataAttributeMemberFunction(AZ::Crc32 crc, AttributeMemberFunctionType memberFunction);

        void ApplyVectorLabels();
        AZStd::string GetVectorLabel(const int index) const;
        AZStd::string GetVectorLabelX() const;
        AZStd::string GetVectorLabelY() const;
        AZStd::string GetVectorLabelZ() const;
        AZStd::string GetVectorLabelW() const;

        // Register is actually use for range-based control types.
        // If all the necessary data is present a slider control will be presented.
        bool ApplyRangeEditDataAttributesToNumericTypes();
        template<typename AttributeValueType>
        bool ApplyRangeEditDataAttributesToNumericType();
        template<typename AttributeValueType>
        void ApplyRangeEditDataAttributes();
        template<typename AttributeValueType>
        void ApplySliderEditDataAttributes();

        const AZ::Edit::ElementData* GetEditData() const;

        AZStd::any m_value;

        DynamicPropertyConfig m_config;

        // Edit data is used to configure control type and attributes that
        // determine how data is presented in a reflected property editor.
        // The entity will be configured based on the data type of the dynamic
        // property and other configuration settings.
        AZ::Edit::ElementData m_editData;

        // Using the last updated edit data pointer to monitor if the property
        // was copied or moved so the edit data can be rebuilt
        AZ::Edit::ElementData* m_editDataTracker = nullptr;
    };
} // namespace AtomToolsFramework
