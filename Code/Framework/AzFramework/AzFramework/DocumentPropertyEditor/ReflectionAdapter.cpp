/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/LegacyReflectionBridge.h>
#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>
#include <AzCore/DOM/DomUtils.h>

namespace AZ::DocumentPropertyEditor
{
    struct ReflectionAdapterReflectionImpl : public AZ::Reflection::IReadWrite
    {
        const ReflectionAdapter* m_adapter;
        AdapterBuilder m_builder;
        AZStd::unordered_map<AZ::TypeId, AZStd::string_view> m_defaultEditorMap;

        void PopulateDefaultEditors()
        {
            m_defaultEditorMap[azrtti_typeid<bool>()] = Nodes::CheckBox::Name;
            m_defaultEditorMap[azrtti_typeid<uint8_t>()] = Nodes::UintSpinBox::Name;
            m_defaultEditorMap[azrtti_typeid<uint16_t>()] = Nodes::UintSpinBox::Name;
            m_defaultEditorMap[azrtti_typeid<uint32_t>()] = Nodes::UintSpinBox::Name;
            m_defaultEditorMap[azrtti_typeid<uint64_t>()] = Nodes::UintSpinBox::Name;
            m_defaultEditorMap[azrtti_typeid<int8_t>()] = Nodes::IntSpinBox::Name;
            m_defaultEditorMap[azrtti_typeid<int16_t>()] = Nodes::IntSpinBox::Name;
            m_defaultEditorMap[azrtti_typeid<int32_t>()] = Nodes::IntSpinBox::Name;
            m_defaultEditorMap[azrtti_typeid<int64_t>()] = Nodes::IntSpinBox::Name;
            m_defaultEditorMap[azrtti_typeid<float>()] = Nodes::DoubleSpinBox::Name;
            m_defaultEditorMap[azrtti_typeid<double>()] = Nodes::DoubleSpinBox::Name;
            m_defaultEditorMap[azrtti_typeid<AZStd::string>()] = Nodes::LineEdit::Name;
        }

        ReflectionAdapterReflectionImpl(const ReflectionAdapter* adapter)
            : m_adapter(adapter)
        {
            PopulateDefaultEditors();
        }

        AZStd::string_view GetPropertyEditor(const AZ::TypeId& id, const Reflection::IAttributes& attributes)
        {
            (void)attributes;
            const Dom::Value handler = attributes.Find(Reflection::DescriptorAttributes::Handler);
            if (handler.IsString())
            {
                return handler.GetString();
            }
            if (auto it = m_defaultEditorMap.find(id); it != m_defaultEditorMap.end())
            {
                return it->second;
            }
            AZ_Assert(false, "Attempted to create primitive property editor for unknown type");
            return "Unknown";
        }

        void ExtractLabel(const Reflection::IAttributes& attributes)
        {
            Dom::Value label = attributes.Find(Reflection::DescriptorAttributes::Label);
            if (!label.IsNull())
            {
                if (!label.IsString())
                {
                    AZ_Warning("DPE", false, "Unable to read Label from property, Label was not a string");
                }
                else
                {
                    m_builder.Label(label.GetString());
                }
            }
        }

        void ForwardAttributes(const Reflection::IAttributes& attributes)
        {
            attributes.ListAttributes(
                [this](AZ::Name group, AZ::Name name, const Dom::Value& value)
                {
                    AZ_Warning("dpe", !name.IsEmpty(), "DPE: Received empty name in ListAttributes");
                    // Skip non-default groups, we don't presently source any attributes from outside of the default group.
                    if (!group.IsEmpty())
                    {
                        return;
                    }

                    if (name == Reflection::DescriptorAttributes::Label || name == Reflection::DescriptorAttributes::Handler)
                    {
                        return;
                    }

                    m_builder.Attribute(name, value);
                });
        }

        void VisitValue(Dom::Value value, AZStd::string_view editorType, const Reflection::IAttributes& attributes, AZStd::function<void(const Dom::Value&)> onChanged)
        {
            m_builder.BeginRow();
            ExtractLabel(attributes);

            m_builder.BeginPropertyEditor(editorType, AZStd::move(value));
            ForwardAttributes(attributes);
            m_builder.Attribute(Nodes::PropertyEditor::OnChanged, onChanged);
            m_builder.EndPropertyEditor();

            m_builder.EndRow();
        }

        template<class T>
        void VisitPrimitive(T& value, const Reflection::IAttributes& attributes)
        {
            VisitValue(
                Dom::Utils::ValueFromType(value), GetPropertyEditor(azrtti_typeid<T>(), attributes), attributes,
                [&value](const Dom::Value& newValue)
                {
                    AZStd::optional<T> extractedValue = Dom::Utils::ValueToType<T>(newValue);
                    AZ_Warning("DPE", extractedValue.has_value(), "OnChanged failed, unable to extract value from DOM");
                    if (extractedValue.has_value())
                    {
                        value = AZStd::move(extractedValue.value());
                    }
                });
        }

        void Visit(bool& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(int8_t& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(int16_t& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(int32_t& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(int64_t& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(uint8_t& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(uint16_t& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(uint32_t& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(uint64_t& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(float& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(double& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void VisitObjectBegin([[maybe_unused]] Reflection::IObjectAccess& access, const Reflection::IAttributes& attributes) override
        {
            m_builder.BeginRow();
            ExtractLabel(attributes);
            ForwardAttributes(attributes);
        }

        void VisitObjectEnd() override
        {
            m_builder.EndRow();
        }

        void Visit(
            const AZStd::string_view value, Reflection::IStringAccess& access, const Reflection::IAttributes& attributes) override
        {
            VisitValue(
                Dom::Utils::ValueFromType(value), GetPropertyEditor(azrtti_typeid<AZStd::string_view>(), attributes), attributes,
                [&access](const Dom::Value& newValue)
                {
                    access.Set(newValue.GetString());
                });
        }

        void Visit(Reflection::IArrayAccess& access, const Reflection::IAttributes& attributes) override
        {
            (void)access;
            (void)attributes;
        }
        void Visit(Reflection::IMapAccess& access, const Reflection::IAttributes& attributes) override
        {
            (void)access;
            (void)attributes;
        }
        void Visit(Reflection::IDictionaryAccess& access, const Reflection::IAttributes& attributes) override
        {
            (void)access;
            (void)attributes;
        }
        void Visit(int64_t value, const Reflection::IEnumAccess& access, const Reflection::IAttributes& attributes) override
        {
            (void)value;
            (void)access;
            (void)attributes;
        }
        void Visit(Reflection::IPointerAccess& access, const Reflection::IAttributes& attributes) override
        {
            (void)access;
            (void)attributes;
        }
        void Visit(Reflection::IBufferAccess& access, const Reflection::IAttributes& attributes) override
        {
            (void)access;
            (void)attributes;
        }
        void Visit(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset, Reflection::IAssetAccess& access, const Reflection::IAttributes& attributes) override
        {
            (void)asset;
            (void)access;
            (void)attributes;
        }
    };

    ReflectionAdapter::ReflectionAdapter(void* instance, AZ::TypeId typeId)
    {
        SetValue(instance, AZStd::move(typeId));
    }

    void ReflectionAdapter::SetValue(void* instance, AZ::TypeId typeId)
    {
        m_instance = instance;
        m_typeId = AZStd::move(typeId);
        NotifyResetDocument();
    }

    Dom::Value ReflectionAdapter::GetContents() const
    {
        ReflectionAdapterReflectionImpl impl(this);
        impl.m_builder.BeginAdapter();
        Reflection::VisitLegacyInMemoryInstance(&impl, m_instance, m_typeId);
        impl.m_builder.EndAdapter();
        return impl.m_builder.FinishAndTakeResult();
    }
} // namespace AZ::DocumentPropertyEditor
