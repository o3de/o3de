/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/DOM/DomPrefixTree.h>
#include <AzCore/DOM/DomUtils.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/LegacyReflectionBridge.h>
#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    struct ReflectionAdapterReflectionImpl : public AZ::Reflection::IReadWrite
    {
        AZ::SerializeContext* m_serializeContext;
        ReflectionAdapter* m_adapter;
        AdapterBuilder m_builder;
        // Look-up table of onChanged callbacks for handling property changes
        AZ::Dom::DomPrefixTree<AZStd::function<Dom::Value(const Dom::Value&)>> m_onChangedCallbacks;

        struct BoundContainer
        {
            AZ::SerializeContext::IDataContainer* m_container;
            void* m_instance;
            void* m_elementInstance = nullptr;
            size_t m_elementIndex = 0;

            Dom::Value GetContainerNode(ReflectionAdapterReflectionImpl* impl, const AZ::Dom::Path& path)
            {
                // Find the row that contains the PropertyEditor for our actual container (if it exists)
                Dom::Value containerRow;
                impl->m_containers.VisitPath(
                    path,
                    [&](const AZ::Dom::Path& nodePath, const BoundContainer& container)
                    {
                        if (containerRow.IsNull() && container.m_container == m_container && container.m_elementInstance == nullptr)
                        {
                            containerRow = impl->m_adapter->GetContents()[nodePath];
                            return false; // we've found our container row, we can stop the visitor
                        }
                        return true;
                    },
                    Dom::PrefixTreeTraversalFlags::ExcludeChildPaths);

                if (containerRow.IsNode())
                {
                    // Look within the Row for a PropertyEditor that has a SerializedPath field.
                    // This will be the container's editor w/ attributes.
                    for (auto it = containerRow.ArrayBegin(); it != containerRow.ArrayEnd(); ++it)
                    {
                        if (it->IsNode() && it->GetNodeName() == GetNodeName<Nodes::PropertyEditor>())
                        {
                            auto serializedFieldIt = it->FindMember(Reflection::DescriptorAttributes::SerializedPath);
                            if (serializedFieldIt != it->MemberEnd())
                            {
                                return *it;
                            }
                        }
                    }
                }
                return {};
            }

            void OnClear(ReflectionAdapterReflectionImpl* impl, const AZ::Dom::Path& path)
            {
                m_container->ClearElements(m_instance, impl->m_serializeContext);

                auto containerNode = GetContainerNode(impl, path);
                for (AZ::s64 i = m_container->Size(m_instance) - 1; i >= 0; --i)
                {
                    Nodes::PropertyEditor::RemoveNotify.InvokeOnDomNode(containerNode, i);
                }
                Nodes::PropertyEditor::ClearNotify.InvokeOnDomNode(containerNode);
                Nodes::PropertyEditor::ChangeNotify.InvokeOnDomNode(containerNode);

                impl->m_adapter->NotifyResetDocument();
            }

            void OnAddElement(ReflectionAdapterReflectionImpl* impl, const AZ::Dom::Path& path)
            {
                if (m_container->IsFixedCapacity() && m_container->Size(m_instance) >= m_container->Capacity(m_instance))
                {
                    return;
                }

                const AZ::SerializeContext::ClassElement* containerClassElement =
                    m_container->GetElement(m_container->GetDefaultElementNameCrc());
                void* dataAddress = m_container->ReserveElement(m_instance, containerClassElement);
                m_container->StoreElement(m_instance, dataAddress);

                auto containerNode = GetContainerNode(impl, path);
                Nodes::PropertyEditor::AddNotify.InvokeOnDomNode(containerNode);
                Nodes::PropertyEditor::ChangeNotify.InvokeOnDomNode(containerNode);

                impl->m_adapter->NotifyResetDocument();
            }

            void OnRemoveElement(ReflectionAdapterReflectionImpl* impl, const AZ::Dom::Path& path)
            {
                AZ_Assert(m_elementInstance != nullptr, "Attempted to remove an element without a defined element instance");
                m_container->RemoveElement(m_instance, m_elementInstance, impl->m_serializeContext);

                auto containerNode = GetContainerNode(impl, path);
                Nodes::PropertyEditor::RemoveNotify.InvokeOnDomNode(containerNode, m_elementIndex);
                Nodes::PropertyEditor::ChangeNotify.InvokeOnDomNode(containerNode);

                impl->m_adapter->NotifyResetDocument();
            }
        };
        // Lookup table of containers and their elements for handling container operations
        AZ::Dom::DomPrefixTree<BoundContainer> m_containers;

        ReflectionAdapterReflectionImpl(ReflectionAdapter* adapter)
            : m_adapter(adapter)
        {
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
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

                    const AZStd::array ignoredAttributes = { Reflection::DescriptorAttributes::Label,
                                                             Reflection::DescriptorAttributes::Handler,
                                                             Reflection::DescriptorAttributes::Container,
                                                             Nodes::PropertyEditor::Visibility.GetName() };
                    if (AZStd::ranges::find(ignoredAttributes, name) != ignoredAttributes.end())
                    {
                        return;
                    }

                    for (const auto& rowAttribute : Nodes::Row::RowAttributes)
                    {
                        if (name == rowAttribute->GetName())
                        {
                            return;
                        }
                    }

                    m_builder.Attribute(name, value);
                });
        }

        void VisitValue(
            Dom::Value value,
            void* instance,
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
            m_onChangedCallbacks.SetValue(m_builder.GetCurrentPath(), AZStd::move(onChanged));
            m_builder.AddMessageHandler(m_adapter, Nodes::PropertyEditor::OnChanged);
            m_builder.EndPropertyEditor();

            CheckContainerElement(instance, attributes);

            if (createRow)
            {
                m_builder.EndRow();
            }
        }

        template<class T>
        void VisitPrimitive(T& value, const Reflection::IAttributes& attributes)
        {
            VisitValue(
                Dom::Utils::ValueFromType(value), &value, attributes,
                [&value](const Dom::Value& newValue)
                {
                    AZStd::optional<T> extractedValue = Dom::Utils::ValueToType<T>(newValue);
                    AZ_Warning("DPE", extractedValue.has_value(), "OnChanged failed, unable to extract value from DOM");
                    if (extractedValue.has_value())
                    {
                        value = AZStd::move(extractedValue.value());
                    }
                    return Dom::Utils::ValueFromType(value);
                },
                true);
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

        void CheckContainerElement(void* instance, const Reflection::IAttributes& attributes)
        {
            auto parentContainerAttribute = attributes.Find(AZ::Reflection::DescriptorAttributes::ParentContainer);
            auto parentContainerInstanceAttribute = attributes.Find(AZ::Reflection::DescriptorAttributes::ParentContainerInstance);
            if (!parentContainerAttribute.IsNull() && !parentContainerInstanceAttribute.IsNull())
            {
                auto parentContainer = AZ::Dom::Utils::ValueToTypeUnsafe<AZ::SerializeContext::IDataContainer*>(parentContainerAttribute);
                auto parentContainerInstance = AZ::Dom::Utils::ValueToTypeUnsafe<void*>(parentContainerInstanceAttribute);
                m_containers.SetValue(m_builder.GetCurrentPath(), BoundContainer{ parentContainer, parentContainerInstance, instance });

                if (!parentContainer->IsFixedSize())
                {
                    m_builder.BeginPropertyEditor<Nodes::ContainerActionButton>();
                    m_builder.Attribute(Nodes::ContainerActionButton::Action, Nodes::ContainerAction::RemoveElement);
                    m_builder.AddMessageHandler(m_adapter, Nodes::ContainerActionButton::OnActivate.GetName());
                    m_builder.EndPropertyEditor();
                }
            }
        }

        void VisitObjectBegin(Reflection::IObjectAccess& access, const Reflection::IAttributes& attributes) override
        {
            auto visibilityAttribute = attributes.Find(Nodes::PropertyEditor::Visibility.GetName());
            Nodes::PropertyVisibility visibility =
                Nodes::PropertyEditor::Visibility.DomToValue(visibilityAttribute).value_or(Nodes::PropertyVisibility::Show);
            if (visibility == Nodes::PropertyVisibility::Hide || visibility == Nodes::PropertyVisibility::ShowChildrenOnly)
            {
                return;
            }

            m_builder.BeginRow();

            for (const auto& attribute : Nodes::Row::RowAttributes)
            {
                auto attributeValue = attributes.Find(attribute->GetName());
                if (!attributeValue.IsNull())
                {
                    m_builder.Attribute(attribute->GetName(), attributeValue);
                }
            }

            ExtractLabel(attributes);
            if (access.GetType() == azrtti_typeid<AZStd::string>())
            {
                AZStd::string& value = *reinterpret_cast<AZStd::string*>(access.Get());
                VisitValue(
                    Dom::Utils::ValueFromType(value), &value, attributes,
                    [&value](const Dom::Value& newValue)
                    {
                        value = newValue.GetString();
                        return newValue;
                    },
                    false);
                return;
            }

            auto containerAttribute = attributes.Find(AZ::Reflection::DescriptorAttributes::Container);
            if (!containerAttribute.IsNull())
            {
                auto container = AZ::Dom::Utils::ValueToTypeUnsafe<AZ::SerializeContext::IDataContainer*>(containerAttribute);
                m_containers.SetValue(m_builder.GetCurrentPath(), BoundContainer{ container, access.Get() });
                size_t containerSize = container->Size(access.Get());
                if (containerSize == 1)
                {
                    m_builder.Label("1 element");
                }
                else
                {
                    m_builder.Label(AZStd::string::format("%zu elements", container->Size(access.Get())));
                }

                if (!container->IsFixedSize())
                {
                    m_builder.BeginPropertyEditor<Nodes::ContainerActionButton>();
                    m_builder.Attribute(Nodes::ContainerActionButton::Action, Nodes::ContainerAction::AddElement);
                    m_builder.AddMessageHandler(m_adapter, Nodes::ContainerActionButton::OnActivate.GetName());
                    m_builder.EndPropertyEditor();

                    m_builder.BeginPropertyEditor<Nodes::ContainerActionButton>();
                    m_builder.Attribute(Nodes::ContainerActionButton::Action, Nodes::ContainerAction::Clear);
                    m_builder.AddMessageHandler(m_adapter, Nodes::ContainerActionButton::OnActivate.GetName());
                    m_builder.EndPropertyEditor();
                }
            }

            AZ::Dom::Value instancePointerValue = AZ::Dom::Utils::MarshalTypedPointerToValue(access.Get(), access.GetType());
            VisitValue(
                instancePointerValue, access.Get(), attributes,
                [](const Dom::Value& newValue)
                {
                    return newValue;
                },
                false);
        }

        void VisitObjectEnd([[maybe_unused]] Reflection::IObjectAccess& access, const Reflection::IAttributes& attributes) override
        {
            auto visibilityAttribute = attributes.Find(Nodes::PropertyEditor::Visibility.GetName());
            Nodes::PropertyVisibility visibility =
                Nodes::PropertyEditor::Visibility.DomToValue(visibilityAttribute).value_or(Nodes::PropertyVisibility::Show);
            if (visibility == Nodes::PropertyVisibility::Hide || visibility == Nodes::PropertyVisibility::ShowChildrenOnly)
            {
                return;
            }
            m_builder.EndRow();
        }

        void Visit(
            [[maybe_unused]] const AZStd::string_view value,
            [[maybe_unused]] Reflection::IStringAccess& access,
            [[maybe_unused]] const Reflection::IAttributes& attributes) override
        {
        }

        void Visit([[maybe_unused]] Reflection::IArrayAccess& access, [[maybe_unused]] const Reflection::IAttributes& attributes) override
        {
        }

        void Visit([[maybe_unused]] Reflection::IMapAccess& access, [[maybe_unused]] const Reflection::IAttributes& attributes) override
        {
        }

        void Visit(
            [[maybe_unused]] Reflection::IDictionaryAccess& access, [[maybe_unused]] const Reflection::IAttributes& attributes) override
        {
        }

        void Visit(
            [[maybe_unused]] AZ::s64 value,
            [[maybe_unused]] const Reflection::IEnumAccess& access,
            [[maybe_unused]] const Reflection::IAttributes& attributes) override
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

    void ReflectionAdapter::InvokeChangeNotify(const AZ::Dom::Value& domNode)
    {
        using Nodes::PropertyEditor;
        using Nodes::PropertyRefreshLevel;

        // Trigger ChangeNotify
        auto changeNotify = PropertyEditor::ChangeNotify.InvokeOnDomNode(domNode);
        if (changeNotify.IsSuccess())
        {
            // If we were told to issue a property refresh, notify our adapter via RequestTreeUpdate
            PropertyRefreshLevel value = changeNotify.GetValue();
            if (value != PropertyRefreshLevel::Undefined && value != PropertyRefreshLevel::None)
            {
                PropertyEditor::RequestTreeUpdate.InvokeOnDomNode(domNode, value);
            }
        }
    }

    Dom::Value ReflectionAdapter::GenerateContents()
    {
        m_impl->m_builder.BeginAdapter();
        m_impl->m_onChangedCallbacks.Clear();
        m_impl->m_containers.Clear();
        if (m_instance != nullptr)
        {
            Reflection::VisitLegacyInMemoryInstance(m_impl.get(), m_instance, m_typeId);
        }
        m_impl->m_builder.EndAdapter();
        return m_impl->m_builder.FinishAndTakeResult();
    }

    Dom::Value ReflectionAdapter::HandleMessage(const AdapterMessage& message)
    {
        using Nodes::ContainerActionButton;
        using Nodes::PropertyEditor;

        auto handlePropertyEditorChanged =
            [&](const Dom::Value& valueFromEditor, [[maybe_unused]] PropertyEditor::ValueChangeType changeType)
        {
            auto changeHandler = m_impl->m_onChangedCallbacks.ValueAtPath(message.m_messageOrigin, AZ::Dom::PrefixTreeMatch::ExactPath);
            if (changeHandler != nullptr)
            {
                Dom::Value newValue = (*changeHandler)(valueFromEditor);
                NotifyContentsChanged({ Dom::PatchOperation::ReplaceOperation(message.m_messageOrigin / "Value", newValue) });
            }
        };

        auto handleContainerOperation = [&]()
        {
            if (message.m_messageOrigin.Size() == 0)
            {
                return;
            }
            auto containerEntry = m_impl->m_containers.ValueAtPath(message.m_messageOrigin, AZ::Dom::PrefixTreeMatch::ParentsOnly);
            if (containerEntry != nullptr)
            {
                using Nodes::ContainerAction;
                AZ::Dom::Value node = GetContents()[message.m_messageOrigin];
                auto action = ContainerActionButton::Action.ExtractFromDomNode(node);
                if (!action.has_value())
                {
                    return;
                }
                switch (action.value())
                {
                case ContainerAction::AddElement:
                    containerEntry->OnAddElement(m_impl.get(), message.m_messageOrigin);
                    break;
                case ContainerAction::RemoveElement:
                    containerEntry->OnRemoveElement(m_impl.get(), message.m_messageOrigin);
                    break;
                case ContainerAction::Clear:
                    containerEntry->OnClear(m_impl.get(), message.m_messageOrigin);
                    break;
                }
            }
        };

        auto handleTreeUpdate = [&](Nodes::PropertyRefreshLevel)
        {
            // For now just trigger a soft reset.
            // This will still only send the view patches for what's actually changed.
            NotifyResetDocument();
        };

        return message.Match(
            PropertyEditor::OnChanged, handlePropertyEditorChanged, ContainerActionButton::OnActivate, handleContainerOperation,
            PropertyEditor::RequestTreeUpdate, handleTreeUpdate);
    }
} // namespace AZ::DocumentPropertyEditor
