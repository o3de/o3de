/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomUtils.h>
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

        ReflectionAdapterReflectionImpl(ReflectionAdapter* adapter)
            : m_adapter(adapter)
        {
        }

        AZStd::string_view GetPropertyEditor(const Reflection::IAttributes& attributes)
        {
            const Dom::Value handler = attributes.Find(Reflection::DescriptorAttributes::Handler);
            if (handler.IsString())
            {
                return handler.GetString();
            }
            // Special case defaulting to ComboBox for enum types, as ComboBox isn't a default handler.
            if (!attributes.Find(Nodes::PropertyEditor::EnumType.GetName()).IsNull())
            {
                return Nodes::ComboBox::Name;
            }
            return {};
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

                    for (const auto& rowAttribute : Nodes::Row::RowAttributes)
                    {
                        if (name == rowAttribute.GetName())
                        {
                            return;
                        }
                    }

                    m_builder.Attribute(name, value);
                });
        }

        void VisitValue(
            Dom::Value value,
            const Reflection::IAttributes& attributes,
            AZStd::function<Dom::Value(const Dom::Value&)> onChanged,
            bool createRow)
        {
            if (createRow)
            {
                m_builder.BeginRow();
                ExtractLabel(attributes);
            }

            m_builder.BeginPropertyEditor(GetPropertyEditor(attributes), AZStd::move(value));
            ForwardAttributes(attributes);
            m_builder.OnEditorChanged(
                [this, onChanged](const Dom::Path& path, const Dom::Value& value, Nodes::PropertyEditor::ValueChangeType changeType)
                {
                    Dom::Value newValue = onChanged(value);
                    m_adapter->OnContentsChanged(path, newValue, changeType);
                });
            m_builder.EndPropertyEditor();

            if (createRow)
            {
                m_builder.EndRow();
            }
        }

        template<class T>
        void VisitPrimitive(T& value, const Reflection::IAttributes& attributes)
        {
            VisitValue(
                Dom::Utils::ValueFromType(value), attributes,
                [&value](const Dom::Value& newValue)
                {
                    AZStd::optional<T> extractedValue = Dom::Utils::ValueToType<T>(newValue);
                    AZ_Warning("DPE", extractedValue.has_value(), "OnChanged failed, unable to extract value from DOM");
                    if (extractedValue.has_value())
                    {
                        value = AZStd::move(extractedValue.value());
                    }
                    return Dom::Utils::ValueFromType(value);
                }, true);
        }

        void Visit(bool& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(AZ::s8& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(AZ::s16& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(AZ::s32& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(AZ::s64& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(AZ::u8& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(AZ::u16& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(AZ::u32& value, const Reflection::IAttributes& attributes) override
        {
            VisitPrimitive(value, attributes);
        }

        void Visit(AZ::u64& value, const Reflection::IAttributes& attributes) override
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

            for (const auto& attribute : Nodes::Row::RowAttributes)
            {
                auto attributeValue = attributes.Find(attribute.GetName());
                if (!attributeValue.IsNull())
                {
                    m_builder.Attribute(attribute, attributeValue);
                }
            }

            ExtractLabel(attributes);
            if (access.GetType() == azrtti_typeid<AZStd::string>())
            {
                AZStd::string& value = *reinterpret_cast<AZStd::string*>(access.Get());
                VisitValue(
                    Dom::Utils::ValueFromType(value), attributes,
                    [&value](const Dom::Value& newValue)
                    {
                        value = newValue.GetString();
                        return newValue;
                    }, false);
                return;
            }

            AZ::Dom::Value instancePointerValue = AZ::Dom::Utils::ValueFromType<void*>(access.Get());
            VisitValue(AZ::Dom::Utils::ValueFromType<void*>(access.Get()), attributes, [](const Dom::Value& newValue)
                {
                    return newValue;
                }, false);
        }

        void VisitObjectEnd() override
        {
            m_builder.EndRow();
        }

        void Visit(const AZStd::string_view value, Reflection::IStringAccess& access, const Reflection::IAttributes& attributes) override
        {
            VisitValue(
                Dom::Utils::ValueFromType(value), attributes,
                [&access](const Dom::Value& newValue)
                {
                    access.Set(newValue.GetString());
                    return newValue;
                }, true);
        }

        void Visit([[maybe_unused]] Reflection::IArrayAccess& access, [[maybe_unused]] const Reflection::IAttributes& attributes) override
        {
        }

        void Visit([[maybe_unused]] Reflection::IMapAccess& access, [[maybe_unused]] const Reflection::IAttributes& attributes) override
        {
        }

        void Visit([[maybe_unused]] Reflection::IDictionaryAccess& access, [[maybe_unused]] const Reflection::IAttributes& attributes) override
        {
        }

        void Visit([[maybe_unused]] AZ::s64 value, [[maybe_unused]] const Reflection::IEnumAccess& access, [[maybe_unused]] const Reflection::IAttributes& attributes) override
        {
        }

        void Visit([[maybe_unused]] Reflection::IPointerAccess& access, [[maybe_unused]] const Reflection::IAttributes& attributes) override
        {
        }

        void Visit([[maybe_unused]] Reflection::IBufferAccess& access, [[maybe_unused]] const Reflection::IAttributes& attributes) override
        {
        }

        void Visit(
            [[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            [[maybe_unused]] Reflection::IAssetAccess& access,
            [[maybe_unused]] const Reflection::IAttributes& attributes) override
        {
        }
    };

    ReflectionAdapter::ReflectionAdapter()
        : RoutingAdapter()
        , m_impl(AZStd::make_unique<ReflectionAdapterReflectionImpl>(this))
    {
    }

    ReflectionAdapter::ReflectionAdapter(void* instance, AZ::TypeId typeId)
        : RoutingAdapter()
        , m_impl(AZStd::make_unique<ReflectionAdapterReflectionImpl>(this))
    {
        SetValue(instance, AZStd::move(typeId));
    }

    // Declare dtor in implementation to make the unique_ptr deleter for m_impl accessible
    ReflectionAdapter::~ReflectionAdapter() = default;

    void ReflectionAdapter::SetValue(void* instance, AZ::TypeId typeId)
    {
        m_instance = instance;
        m_typeId = AZStd::move(typeId);
        NotifyResetDocument();
    }

    Dom::Value ReflectionAdapter::GenerateContents()
    {
        m_impl->m_builder.BeginAdapter();
        if (m_instance != nullptr)
        {
            Reflection::VisitLegacyInMemoryInstance(m_impl.get(), m_instance, m_typeId);
        }
        m_impl->m_builder.EndAdapter();
        return m_impl->m_builder.FinishAndTakeResult();
    }

    void ReflectionAdapter::OnContentsChanged(const Dom::Path& path, const Dom::Value& value, [[maybe_unused]] Nodes::PropertyEditor::ValueChangeType changeType)
    {
        NotifyContentsChanged({ Dom::PatchOperation::ReplaceOperation(path, value) });
    }
} // namespace AZ::DocumentPropertyEditor
