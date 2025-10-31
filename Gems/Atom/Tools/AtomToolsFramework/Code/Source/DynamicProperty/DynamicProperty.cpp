/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/ColorUtils.h>
#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>

namespace AtomToolsFramework
{
    // DynamicProperty uses AZStd::any and some other template container types like assets for editable values.
    // DynamicProperty uses a single dynamic edit data object to apply to all contained instances in its data hierarchy.
    // The dynamic edit data is not read directly from DynamicProperty but copied whenever the RPE rebuilds its tree.
    // Whenever attributes are refreshed, new values are read from the dynamic edit data copy. Updating the source values has no effect
    // unless the tree is rebuilt. We want to avoid rebuilding the RPE tree because it is a distracting and terrible UI experience.

    // The edit context and RPE allow binding functions and methods to attribute to support dynamic edit data changes.
    // If attributes are bound to functions the edit data can be copied and functions will be called each time attributes are refreshed.

    // The pre existing AttributeMemberFunction expects the instance data pointer to be the object pointer for the member function.
    // The pre existing AttributeMemberFunction will not work for DynamicProperty because it shares one dynamic edit data
    // object throughout its hierarchy. The instance data pointer will only be the same as DynamicProperty at the root.

    // AttributeFixedMemberFunction (based on AttributeMemberFunction) addresses these issues by binding member functions with
    // a fixed object pointer.
    template<class T>
    class AttributeFixedMemberFunction;

    template<class R, class C, class... Args>
    class AttributeFixedMemberFunction<R(C::*)(Args...) const>
        : public AZ::AttributeFunction<R(Args...)>
    {
    public:
        AZ_RTTI((AttributeFixedMemberFunction, "{78511F1E-58AD-4670-8440-1FE4C9BD1C21}", R(C::*)(Args...) const), AZ::AttributeFunction<R(Args...)>);
        AZ_CLASS_ALLOCATOR(AttributeFixedMemberFunction, AZ::SystemAllocator);
        typedef R(C::* FunctionPtr)(Args...) const;

        explicit AttributeFixedMemberFunction(C* o, FunctionPtr f)
            : AZ::AttributeFunction<R(Args...)>(nullptr)
            , m_object(o)
            , m_memFunction(f)
        {}

        R Invoke(void* /*instance*/, const Args&... args) override
        {
            return (m_object->*m_memFunction)(args...);
        }

        AZ::Uuid GetInstanceType() const override
        {
            return AZ::Uuid::CreateNull();
        }

    private:
        C* m_object = nullptr;
        FunctionPtr m_memFunction;
    };

    void DynamicProperty::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DynamicProperty>()
                ->Field("value", &DynamicProperty::m_value)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DynamicProperty>(
                    "DynamicProperty", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &DynamicProperty::GetVisibility)
                    ->SetDynamicEditDataProvider(&DynamicProperty::GetPropertyEditData)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicProperty::m_value, "Value", "")
                    // AZStd::any is treated like a container type so we hide it and pass attributes to the child element
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    const AZ::Edit::ElementData* DynamicProperty::GetPropertyEditData(const void* handlerPtr, [[maybe_unused]] const void* elementPtr, [[maybe_unused]] const AZ::Uuid& elementType)
    {
        const DynamicProperty* owner = reinterpret_cast<const DynamicProperty*>(handlerPtr);
        if (elementType == owner->m_value.type() && elementPtr == AZStd::any_cast<void>(&owner->m_value))
        {
            return owner->GetEditData();
        }
        return nullptr;
    }

    DynamicProperty::DynamicProperty(const DynamicPropertyConfig& config)
        : m_value(config.m_originalValue)
        , m_config(config)
    {
    }

    void DynamicProperty::SetValue(const AZStd::any& value)
    {
        AZ_Assert(!value.empty(), "DynamicProperty attempting to assign a bad value to: %s", m_config.m_id.GetCStr());
        m_value = value;
    }

    const AZStd::any& DynamicProperty::GetValue() const
    {
        return m_value;
    }

    void DynamicProperty::SetConfig(const DynamicPropertyConfig& config)
    {
        m_config = config;
        m_editDataTracker = nullptr;
    }

    const DynamicPropertyConfig& DynamicProperty::GetConfig() const
    {
        return m_config;
    }

    void DynamicProperty::UpdateEditData()
    {
        if (m_editDataTracker != &m_editData)
        {
            m_editDataTracker = &m_editData;

            CheckRangeMetaDataValues();

            m_editData = {};
            m_editData.m_name = nullptr;
            m_editData.m_elementId = AZ::Edit::UIHandlers::Default;

            AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::NameLabelOverride, &DynamicProperty::GetDisplayName);
            AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::DescriptionTextOverride, &DynamicProperty::GetDescription);
            AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::ReadOnly, &DynamicProperty::IsReadOnly);
            AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::ChangeNotify, &DynamicProperty::OnDataChanged);

            // Moving the attributes outside of the switch statement for specific types because the dynamic property is moving away from
            // using the enum. Even though these attributes only apply to specific property types, they can safely be applied to all
            // property types because each property control will only process attributes they recognize.
            AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::AssetPickerTitle, &DynamicProperty::GetAssetPickerTitle);
            AddEditDataAttribute(AZ::Edit::Attributes::ShowProductAssetFileName, false);
            AddEditDataAttribute(AZ_CRC_CE("Thumbnail"), m_config.m_showThumbnail);
            AddEditDataAttribute(AZ_CRC_CE("SupportedAssetTypes"), m_config.m_supportedAssetTypes);

            if (m_config.m_customHandler)
            {
                AddEditDataAttribute(AZ::Edit::Attributes::Handler, m_config.m_customHandler);
            }

            ApplyRangeEditDataAttributesToNumericTypes();

            if (m_value.is<AZ::Vector2>() || m_value.is<AZ::Vector3>() || m_value.is<AZ::Vector4>())
            {
                ApplyVectorLabels();
                ApplyRangeEditDataAttributes<float>();
            }

            if (m_value.is<AZ::Color>())
            {
                AddEditDataAttribute(AZ_CRC_CE("ColorEditorConfiguration"), AZ::RPI::ColorUtils::GetLinearRgbEditorConfig());
            }

            if (!m_config.m_enumValues.empty() && IsValueInteger())
            {
                m_editData.m_elementId = AZ::Edit::UIHandlers::ComboBox;
                AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::EnumValues, &DynamicProperty::GetEnumValues);
            }

            if (!m_config.m_enumValues.empty() && m_value.is<AZStd::string>())
            {
                m_editData.m_elementId = AZ::Edit::UIHandlers::ComboBox;
                AddEditDataAttribute(AZ::Edit::Attributes::StringList, m_config.m_enumValues);
            }
        }
    }

    const AZ::Edit::ElementData* DynamicProperty::GetEditData() const
    {
        const_cast<DynamicProperty*>(this)->UpdateEditData();
        return m_editDataTracker;
    }

    bool DynamicProperty::IsValid() const
    {
        return !m_value.empty();
    }

    const AZ::Name DynamicProperty::GetId() const
    {
        return m_config.m_id;
    }

    AZStd::string DynamicProperty::GetDisplayName() const
    {
        return !m_config.m_displayName.empty() ? m_config.m_displayName : m_config.m_name;
    }

    AZStd::string DynamicProperty::GetGroupDisplayName() const
    {
        return m_config.m_groupDisplayName;
    }

    AZStd::string DynamicProperty::GetAssetPickerTitle() const
    {
        return GetGroupDisplayName().empty() ? GetDisplayName() : GetGroupDisplayName() + " " + GetDisplayName();
    }

    AZStd::string DynamicProperty::GetDescription() const
    {
        return m_config.m_description;
    }

    AZ::Crc32 DynamicProperty::GetVisibility() const
    {
        return (IsValid() && m_config.m_visible) ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    bool DynamicProperty::IsReadOnly() const
    {
        return !IsValid() || m_config.m_readOnly;
    }

    AZStd::vector<AZ::Edit::EnumConstant<uint32_t>> DynamicProperty::GetEnumValues() const
    {
        AZStd::vector<AZ::Edit::EnumConstant<uint32_t>> enumValues;
        enumValues.reserve(m_config.m_enumValues.size());
        for (const AZStd::string& name : m_config.m_enumValues)
        {
            enumValues.emplace_back((uint32_t)enumValues.size(), name.c_str());
        }
        return enumValues;
    }

    AZ::u32 DynamicProperty::OnDataChanged() const
    {
        if (m_config.m_dataChangeCallback)
        {
            return m_config.m_dataChangeCallback(GetValue());
        }
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    bool DynamicProperty::CheckRangeMetaDataValuesForType(const AZ::Uuid& expectedTypeId) const
    {
        auto isExpectedType = [&](const AZStd::any& any, [[maybe_unused]] const char* valueName)
        {
            if (!any.empty() && expectedTypeId != any.type())
            {
                AZ_Error(
                    "AtomToolsFramework",
                    false,
                    "Property '%s': '%s' value data type does not match property data type.",
                    m_config.m_id.GetCStr(),
                    valueName);
                return false;
            }
            return true;
        };

        return isExpectedType(m_config.m_min, "Min") &&
            isExpectedType(m_config.m_max, "Max") &&
            isExpectedType(m_config.m_softMin, "Soft Min") &&
            isExpectedType(m_config.m_softMax, "Soft Max") &&
            isExpectedType(m_config.m_step, "Step");
    }

    bool DynamicProperty::CheckRangeMetaDataValues() const
    {
        if (IsValueInteger() || m_value.is<float>() || m_value.is<double>())
        {
            return CheckRangeMetaDataValuesForType(m_value.type());
        }

        if (m_value.is<AZ::Vector2>() || m_value.is<AZ::Vector3>() || m_value.is<AZ::Vector4>())
        {
            return CheckRangeMetaDataValuesForType(azrtti_typeid<float>());
        }

        auto warnIfNotEmpty = [&]([[maybe_unused]] const AZStd::any& any, [[maybe_unused]] const char* valueName)
        {
            AZ_Warning(
                "AtomToolsFramework",
                any.empty(),
                "Property '%s': '%s' is not supported by this property data type.",
                m_config.m_id.GetCStr(),
                valueName);
        };

        warnIfNotEmpty(m_config.m_min, "Min");
        warnIfNotEmpty(m_config.m_max, "Max");
        warnIfNotEmpty(m_config.m_step, "Step");
        return true;
    }

    bool DynamicProperty::IsValueInteger() const
    {
        return
            m_value.is<int8_t>() || m_value.is<uint8_t>() ||
            m_value.is<int16_t>() || m_value.is<uint16_t>() ||
            m_value.is<int32_t>() || m_value.is<uint32_t>() ||
            m_value.is<int64_t>() || m_value.is<uint64_t>() ||
            m_value.is<AZ::s8>() || m_value.is<AZ::u8>() ||
            m_value.is<AZ::s16>() || m_value.is<AZ::u16>() ||
            m_value.is<AZ::s32>() || m_value.is<AZ::u32>() ||
            m_value.is<AZ::s64>() || m_value.is<AZ::u64>();
    }

    template<typename AttributeValueType>
    void DynamicProperty::AddEditDataAttribute(AZ::Crc32 crc, AttributeValueType attribute)
    {
        m_editData.m_attributes.push_back(AZ::Edit::AttributePair(crc, aznew AZ::AttributeContainerType<AttributeValueType>(attribute)));
    }

    template<typename AttributeMemberFunctionType>
    void DynamicProperty::AddEditDataAttributeMemberFunction(AZ::Crc32 crc, AttributeMemberFunctionType memberFunction)
    {
        m_editData.m_attributes.push_back(
            AZ::Edit::AttributePair(crc, aznew AttributeFixedMemberFunction<AttributeMemberFunctionType>(this, memberFunction)));
    }

    bool DynamicProperty::ApplyRangeEditDataAttributesToNumericTypes()
    {
        if (ApplyRangeEditDataAttributesToNumericType<int8_t>() || ApplyRangeEditDataAttributesToNumericType<uint8_t>() ||
            ApplyRangeEditDataAttributesToNumericType<int16_t>() || ApplyRangeEditDataAttributesToNumericType<uint16_t>() ||
            ApplyRangeEditDataAttributesToNumericType<int32_t>() || ApplyRangeEditDataAttributesToNumericType<uint32_t>() ||
            ApplyRangeEditDataAttributesToNumericType<int64_t>() || ApplyRangeEditDataAttributesToNumericType<uint64_t>() ||
            ApplyRangeEditDataAttributesToNumericType<AZ::s8>() || ApplyRangeEditDataAttributesToNumericType<AZ::u8>() ||
            ApplyRangeEditDataAttributesToNumericType<AZ::s16>() || ApplyRangeEditDataAttributesToNumericType<AZ::u16>() ||
            ApplyRangeEditDataAttributesToNumericType<AZ::s32>() || ApplyRangeEditDataAttributesToNumericType<AZ::u32>() ||
            ApplyRangeEditDataAttributesToNumericType<AZ::s64>() || ApplyRangeEditDataAttributesToNumericType<AZ::u64>() ||
            ApplyRangeEditDataAttributesToNumericType<float>() || ApplyRangeEditDataAttributesToNumericType<double>())
        {
            return true;
        }
        return false;
    }

    template<typename AttributeValueType>
    bool DynamicProperty::ApplyRangeEditDataAttributesToNumericType()
    {
        if (m_value.is<AttributeValueType>())
        {
            ApplyRangeEditDataAttributes<AttributeValueType>();
            ApplySliderEditDataAttributes<AttributeValueType>();
            return true;
        }
        return false;
    }

    template<typename AttributeValueType>
    void DynamicProperty::ApplyRangeEditDataAttributes()
    {
        // Slider and spin box controls require both minimum and maximum ranges to be entered in order to override the default values set
        // to 0 and 100. They must also be set in a certain order because of clamping that is done as the attributes are applied.
        AddEditDataAttribute(
            AZ::Edit::Attributes::Min,
            m_config.m_min.is<AttributeValueType>() ? AZStd::any_cast<AttributeValueType>(m_config.m_min)
                                                    : AZStd::numeric_limits<AttributeValueType>::lowest());
        AddEditDataAttribute(
            AZ::Edit::Attributes::Max,
            m_config.m_max.is<AttributeValueType>() ? AZStd::any_cast<AttributeValueType>(m_config.m_max)
                                                    : AZStd::numeric_limits<AttributeValueType>::max());

        if (m_config.m_softMin.is<AttributeValueType>())
        {
            AddEditDataAttribute(AZ::Edit::Attributes::SoftMin, AZStd::any_cast<AttributeValueType>(m_config.m_softMin));
        }

        if (m_config.m_softMax.is<AttributeValueType>())
        {
            AddEditDataAttribute(AZ::Edit::Attributes::SoftMax, AZStd::any_cast<AttributeValueType>(m_config.m_softMax));
        }

        if (m_config.m_step.is<AttributeValueType>())
        {
            AddEditDataAttribute(AZ::Edit::Attributes::Step, AZStd::any_cast<AttributeValueType>(m_config.m_step));
        }
    }

    template<typename AttributeValueType>
    void DynamicProperty::ApplySliderEditDataAttributes()
    {
        if ((m_config.m_min.is<AttributeValueType>() || m_config.m_softMin.is<AttributeValueType>()) &&
            (m_config.m_max.is<AttributeValueType>() || m_config.m_softMax.is<AttributeValueType>()))
        {
            m_editData.m_elementId = AZ::Edit::UIHandlers::Slider;
        }
    }

    void DynamicProperty::ApplyVectorLabels()
    {
        AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::LabelForX, &DynamicProperty::GetVectorLabelX);
        AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::LabelForY, &DynamicProperty::GetVectorLabelY);
        AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::LabelForZ, &DynamicProperty::GetVectorLabelZ);
        AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::LabelForW, &DynamicProperty::GetVectorLabelW);
    }

    AZStd::string DynamicProperty::GetVectorLabel(const int index) const
    {
        static const char* defaultLabels[] = { "X", "Y", "Z", "W" };
        return index < m_config.m_vectorLabels.size() ? m_config.m_vectorLabels[index] : defaultLabels[AZStd::clamp(index, 0, 3)];
    }

    AZStd::string DynamicProperty::GetVectorLabelX() const
    {
        return GetVectorLabel(0);
    }

    AZStd::string DynamicProperty::GetVectorLabelY() const
    {
        return GetVectorLabel(1);
    }

    AZStd::string DynamicProperty::GetVectorLabelZ() const
    {
        return GetVectorLabel(2);
    }

    AZStd::string DynamicProperty::GetVectorLabelW() const
    {
        return GetVectorLabel(3);
    }
} // namespace AtomToolsFramework
