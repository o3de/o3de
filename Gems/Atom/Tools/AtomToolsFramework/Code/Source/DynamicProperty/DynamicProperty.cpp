/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Edit/Common/ColorUtils.h>
#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>

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
        AZ_RTTI((AtomToolsFramework::AttributeFixedMemberFunction<R(C::*)(Args...) const>, "{78511F1E-58AD-4670-8440-1FE4C9BD1C21}", R, C, Args...), AZ::AttributeFunction<R(Args...)>);
        AZ_CLASS_ALLOCATOR(AttributeFixedMemberFunction<R(C::*)(Args...) const>, AZ::SystemAllocator, 0);
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
        return owner->GetEditData();
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
            m_editData.m_elementId = AZ::Edit::UIHandlers::Default;

            AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::NameLabelOverride, &DynamicProperty::GetDisplayName);
            AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::AssetPickerTitle, &DynamicProperty::GetAssetPickerTitle);
            AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::DescriptionTextOverride, &DynamicProperty::GetDescription);
            AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::ReadOnly, &DynamicProperty::IsReadOnly);
            AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::EnumValues, &DynamicProperty::GetEnumValues);
            AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::ChangeNotify, &DynamicProperty::OnDataChanged);
            AddEditDataAttribute(AZ::Edit::Attributes::ShowProductAssetFileName, false);
            AddEditDataAttribute(AZ_CRC_CE("Thumbnail"), m_config.m_showThumbnail);

            switch (m_config.m_dataType)
            {
            case DynamicPropertyType::Int:
                ApplyRangeEditDataAttributes<int32_t>();
                ApplySliderEditDataAttributes<int32_t>();
                break;
            case DynamicPropertyType::UInt:
                ApplyRangeEditDataAttributes<uint32_t>();
                ApplySliderEditDataAttributes<uint32_t>();
                break;
            case DynamicPropertyType::Float:
                ApplyRangeEditDataAttributes<float>();
                ApplySliderEditDataAttributes<float>();
                break;
            case DynamicPropertyType::Vector2:
            case DynamicPropertyType::Vector3:
            case DynamicPropertyType::Vector4:
                ApplyVectorLabels();
                ApplyRangeEditDataAttributes<float>();
                break;
            case DynamicPropertyType::Color:
                AddEditDataAttribute(AZ_CRC("ColorEditorConfiguration", 0xc8b9510e), AZ::RPI::ColorUtils::GetLinearRgbEditorConfig());
                break;
            case DynamicPropertyType::Enum:
                m_editData.m_elementId = AZ::Edit::UIHandlers::ComboBox;
                break;
            case DynamicPropertyType::String:
                m_editData.m_elementId = AZ::Edit::UIHandlers::LineEdit;
                break;
            case DynamicPropertyType::Invalid:
                break;
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

    AZStd::string DynamicProperty::GetGroupName() const
    {
        return m_config.m_groupName;
    }

    AZStd::string DynamicProperty::GetAssetPickerTitle() const
    {
        return GetGroupName().empty() ? GetDisplayName() : GetGroupName() + " " + GetDisplayName();
    }

    AZStd::string DynamicProperty::GetDescription() const
    {
        return m_config.m_description;
    }

    AZ::Crc32 DynamicProperty::GetVisibility() const
    {
        return (IsValid() && m_config.m_visible) ?
            AZ::Edit::PropertyVisibility::ShowChildrenOnly :
            AZ::Edit::PropertyVisibility::Hide;
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
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    template<typename T>
    bool DynamicProperty::CheckRangeMetaDataValuesForType() const
    {
        auto checkAnyType = [&](const AZ::TypeId& expectedTypeId, const AZStd::any& any, [[maybe_unused]] const char* valueName)
        {
            if (!any.empty() && expectedTypeId != any.type())
            {
                AZ_Error("AtomToolsFramework", false, "Property '%s': '%s' value data type does not match property data type.", m_config.m_id.GetCStr(), valueName);
                return false;
            }
            return true;
        };

        AZ::TypeId expectedRangeTypeId = azrtti_typeid<T>();

        if (!checkAnyType(expectedRangeTypeId, m_config.m_min, "Min") ||
            !checkAnyType(expectedRangeTypeId, m_config.m_max, "Max") ||
            !checkAnyType(expectedRangeTypeId, m_config.m_softMin, "Soft Min") ||
            !checkAnyType(expectedRangeTypeId, m_config.m_softMax, "Soft Max") ||
            !checkAnyType(expectedRangeTypeId, m_config.m_step, "Step"))
        {
            return false;
        }

        if (!m_config.m_min.empty() &&
            !m_config.m_max.empty() &&
            AZStd::any_cast<T>(m_config.m_min) == AZStd::any_cast<T>(m_config.m_max))
        {
            AZ_Warning("AtomToolsFramework", false, "Property '%s': Min == Max, value may be frozen in the editor.", m_config.m_id.GetCStr());
        }

        if (!m_config.m_step.empty() && 0 == AZStd::any_cast<T>(m_config.m_step))
        {
            AZ_Warning("AtomToolsFramework", false, "Property '%s': Step is 0, value may be frozen in the editor.", m_config.m_id.GetCStr());
        }

        return true;
    }

    bool DynamicProperty::CheckRangeMetaDataValues() const
    {
        using namespace AZ::RPI;

        auto warnIfNotEmpty = [&](const AZStd::any& any, [[maybe_unused]] const char* valueName)
        {
            if (!any.empty())
            {
                AZ_Warning("AtomToolsFramework", false, "Property '%s': '%s' is not supported by this property data type.", m_config.m_id.GetCStr(), valueName);
            }
        };

        switch (m_config.m_dataType)
        {
        case DynamicPropertyType::Int:
            return CheckRangeMetaDataValuesForType<int32_t>();
        case DynamicPropertyType::UInt:
            return CheckRangeMetaDataValuesForType<uint32_t>();
        case DynamicPropertyType::Float:
        case DynamicPropertyType::Vector2:
        case DynamicPropertyType::Vector3:
        case DynamicPropertyType::Vector4:
            return CheckRangeMetaDataValuesForType<float>();
        default:
            warnIfNotEmpty(m_config.m_min, "Min");
            warnIfNotEmpty(m_config.m_max, "Max");
            warnIfNotEmpty(m_config.m_step, "Step");
            return true;
        }
    }

    template<typename AttributeValueType>
    void DynamicProperty::AddEditDataAttribute(AZ::Crc32 crc, AttributeValueType attribute)
    {
        m_editData.m_attributes.push_back(AZ::Edit::AttributePair(
            crc, aznew AZ::AttributeContainerType<AttributeValueType>(attribute)));
    }

    template<typename AttributeMemberFunctionType>
    void DynamicProperty::AddEditDataAttributeMemberFunction(AZ::Crc32 crc, AttributeMemberFunctionType memberFunction)
    {
        m_editData.m_attributes.push_back(AZ::Edit::AttributePair(
            crc, aznew AttributeFixedMemberFunction<AttributeMemberFunctionType>(this, memberFunction)));
    }

    template<typename AttributeValueType>
    void DynamicProperty::ApplyRangeEditDataAttributes()
    {
        AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::Min, &DynamicProperty::GetMin<AttributeValueType>);
        AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::Max, &DynamicProperty::GetMax<AttributeValueType>);
        AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::SoftMin, &DynamicProperty::GetSoftMin<AttributeValueType>);
        AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::SoftMax, &DynamicProperty::GetSoftMax<AttributeValueType>);
        AddEditDataAttributeMemberFunction(AZ::Edit::Attributes::Step, &DynamicProperty::GetStep<AttributeValueType>);
    }

    template<typename AttributeValueType>
    void DynamicProperty::ApplySliderEditDataAttributes()
    {
        if ((m_config.m_min.is<AttributeValueType>() || m_config.m_softMin.is<AttributeValueType>())
            && (m_config.m_max.is<AttributeValueType>() || m_config.m_softMax.is<AttributeValueType>()))
        {
            m_editData.m_elementId = AZ::Edit::UIHandlers::Slider;
        }
    }

    template<typename AttributeValueType>
    AttributeValueType DynamicProperty::GetMin() const
    {
        if (m_config.m_min.is<AttributeValueType>())
        {
            return AZStd::any_cast<AttributeValueType>(m_config.m_min);
        }
        return std::numeric_limits<AttributeValueType>::lowest();
    }

    template<typename AttributeValueType>
    AttributeValueType DynamicProperty::GetMax() const
    {
        if (m_config.m_max.is<AttributeValueType>())
        {
            return AZStd::any_cast<AttributeValueType>(m_config.m_max);
        }
        return std::numeric_limits<AttributeValueType>::max();
    }

    template<typename AttributeValueType>
    AttributeValueType DynamicProperty::GetSoftMin() const
    {
        if (m_config.m_softMin.is<AttributeValueType>())
        {
            return AZStd::any_cast<AttributeValueType>(m_config.m_softMin);
        }
        return GetMin<AttributeValueType>();
    }

    template<typename AttributeValueType>
    AttributeValueType DynamicProperty::GetSoftMax() const
    {
        if (m_config.m_softMax.is<AttributeValueType>())
        {
            return AZStd::any_cast<AttributeValueType>(m_config.m_softMax);
        }
        return GetMax<AttributeValueType>();
    }

    template<typename AttributeValueType>
    AttributeValueType DynamicProperty::GetStep() const
    {
        if (m_config.m_step.is<AttributeValueType>())
        {
            return AZStd::any_cast<AttributeValueType>(m_config.m_step);
        }
        if (m_config.m_step.is<float>())
        {
            return aznumeric_cast<AttributeValueType>(0.001f);
        }
        return aznumeric_cast<AttributeValueType>(1.0f);
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

        return index < m_config.m_vectorLabels.size() ? m_config.m_vectorLabels[index] : defaultLabels[index];
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
