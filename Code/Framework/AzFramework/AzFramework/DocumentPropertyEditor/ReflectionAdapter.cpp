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

namespace AZ::DocumentPropertyEditor
{
    struct ReflectionAdapterReflectionImpl : public AZ::Reflection::IReadWrite
    {
        ReflectionAdapter* m_adapter;
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

        ReflectionAdapterReflectionImpl(ReflectionAdapter* adapter)
            : m_adapter(adapter)
        {
            PopulateDefaultEditors();
        }

        AZStd::string_view GetPropertyEditor(const AZ::TypeId& id, const Reflection::IAttributes& attributes)
        {
            (void)attributes;
            const Dom::Value handler = attributes.Find(Reflection::LegacyAttributes::Handler);
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
            Dom::Value label = attributes.Find(Reflection::LegacyAttributes::Label);
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
                    if (!group.IsEmpty())
                    {
                        return;
                    }

                    if (name == Reflection::LegacyAttributes::Label)
                    {
                        return;
                    }

                    m_builder.Attribute(name, value);
                });
        }

        void VisitValue(Dom::Value value, AZStd::string_view editorType, const Reflection::IAttributes& attributes)
        {
            m_builder.BeginRow();
            ExtractLabel(attributes);

            m_builder.BeginPropertyEditor(editorType, AZStd::move(value));
            ForwardAttributes(attributes);
            m_builder.EndPropertyEditor();

            m_builder.EndRow();
        }

        template<class T>
        void VisitPrimitive(T& value, const Reflection::IAttributes& attributes)
        {
            VisitValue(Reflection::CreateValue(value), GetPropertyEditor(azrtti_typeid<T>(), attributes), attributes);
        }

        virtual void Visit(bool& value, const Reflection::IAttributes& attributes)
        {
            VisitPrimitive(value, attributes);
        }

        virtual void Visit(int8_t& value, const Reflection::IAttributes& attributes)
        {
            VisitPrimitive(value, attributes);
        }

        virtual void Visit(int16_t& value, const Reflection::IAttributes& attributes)
        {
            VisitPrimitive(value, attributes);
        }

        virtual void Visit(int32_t& value, const Reflection::IAttributes& attributes)
        {
            VisitPrimitive(value, attributes);
        }

        virtual void Visit(int64_t& value, const Reflection::IAttributes& attributes)
        {
            VisitPrimitive(value, attributes);
        }

        virtual void Visit(uint8_t& value, const Reflection::IAttributes& attributes)
        {
            VisitPrimitive(value, attributes);
        }

        virtual void Visit(uint16_t& value, const Reflection::IAttributes& attributes)
        {
            VisitPrimitive(value, attributes);
        }

        virtual void Visit(uint32_t& value, const Reflection::IAttributes& attributes)
        {
            VisitPrimitive(value, attributes);
        }

        virtual void Visit(uint64_t& value, const Reflection::IAttributes& attributes)
        {
            VisitPrimitive(value, attributes);
        }

        virtual void Visit(float& value, const Reflection::IAttributes& attributes)
        {
            VisitPrimitive(value, attributes);
        }

        virtual void Visit(double& value, const Reflection::IAttributes& attributes)
        {
            VisitPrimitive(value, attributes);
        }

        virtual void VisitObjectBegin([[maybe_unused]] Reflection::IObjectAccess& access, const Reflection::IAttributes& attributes)
        {
            m_builder.BeginRow();
            ExtractLabel(attributes);
            ForwardAttributes(attributes);
        }

        virtual void VisitObjectEnd()
        {
            m_builder.EndRow();
        }

        virtual void Visit(
            const AZStd::string_view value, [[maybe_unused]] Reflection::IStringAccess& access, const Reflection::IAttributes& attributes)
        {
            VisitPrimitive(value, attributes);
        }

        virtual void Visit(Reflection::IArrayAccess& access, const Reflection::IAttributes& attributes);
        virtual void Visit(Reflection::IMapAccess& access, const Reflection::IAttributes& attributes);
        virtual void Visit(Reflection::IDictionaryAccess& access, const Reflection::IAttributes& attributes);
        virtual void Visit(int64_t value, const Reflection::IEnumAccess& access, const Reflection::IAttributes& attributes);
        virtual void Visit(Reflection::IPointerAccess& access, const Reflection::IAttributes& attributes);
        virtual void Visit(Reflection::IBufferAccess& access, const Reflection::IAttributes& attributes);
        virtual void Visit(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset, Reflection::IAssetAccess& access, const Reflection::IAttributes& attributes)
        {
            (void)asset;
            (void)access;
            (void)attributes;
        }
    };
} // namespace AZ::DocumentPropertyEditor
