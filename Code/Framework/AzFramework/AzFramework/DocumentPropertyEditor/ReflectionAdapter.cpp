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

                // dataAddress is allocated instance of key type. Get the key type and send it with the dataAddress in the
                // message, then skip the store element below until we get an AddContainerKey message back from the DPE UI
                void* dataAddress = m_container->ReserveElement(m_instance, containerClassElement);

                auto associativeContainer = m_container->GetAssociativeContainerInterface();
                if (associativeContainer)
                {
                    auto keyTypeAttribute = containerClassElement->FindAttribute(AZ_CRC_CE("KeyType"));
                    if (keyTypeAttribute)
                    {
                        auto* keyTypeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Uuid>*>(keyTypeAttribute);
                        if (keyTypeData)
                        {
                            auto keyType = keyTypeData->Get(nullptr);
                            DocumentAdapterPtr reflectionAdapter = AZStd::make_shared<ReflectionAdapter>(dataAddress, keyType);
                            Nodes::Adapter::QueryKey.InvokeOnDomNode(impl->m_adapter->GetContents(), &reflectionAdapter, path);
                            return; // key queried; don't store the actual entry until the DPE handles the QueryKey message
                        }
                    }
                }

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
            bool createRow,
            bool hashValue)
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
            m_builder.AddMessageHandler(m_adapter, Nodes::PropertyEditor::RequestTreeUpdate);

            if (hashValue)
            {
                AZStd::any anyVal(&instance);
                m_builder.Attribute(
                    Nodes::PropertyEditor::ValueHashed,
                    AZ::Uuid::CreateData(reinterpret_cast<AZStd::byte*>(AZStd::any_cast<void>(&anyVal)), anyVal.get_type_info().m_valueSize));
            }
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
                Dom::Utils::ValueFromType(value),
                &value,
                attributes,
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
                true, false);
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

                // check if this element is actually standing in for a direct child of a container. This is used in scenarios like
                // maps, where the direct children are actually pairs of key/value, but we need to only show the value as an editable item
                // who pretends that they can be removed directly from the container
                auto containerElementOverrideAttribute = attributes.Find(AZ::Reflection::DescriptorAttributes::ContainerElementOverride);
                if (!containerElementOverrideAttribute.IsNull())
                {
                    instance = AZ::Dom::Utils::ValueToTypeUnsafe<void*>(containerElementOverrideAttribute);
                }

                m_containers.SetValue(m_builder.GetCurrentPath(), BoundContainer{ parentContainer, parentContainerInstance, instance });

                if (!parentContainer->IsFixedSize())
                {
                    m_builder.BeginPropertyEditor<Nodes::ContainerActionButton>();
                    m_builder.Attribute(Nodes::PropertyEditor::SharePriorColumn, true);
                    m_builder.Attribute(Nodes::PropertyEditor::UseMinimumWidth, true);
                    m_builder.Attribute(Nodes::PropertyEditor::Alignment, Nodes::PropertyEditor::Align::AlignRight);
                    m_builder.Attribute(Nodes::ContainerActionButton::Action, Nodes::ContainerAction::RemoveElement);
                    auto ancestorDisabledValue = attributes.Find(Nodes::NodeWithVisiblityControl::AncestorDisabled.GetName());
                    bool isAncestorDisabledValue = ancestorDisabledValue.IsBool() && ancestorDisabledValue.GetBool();
                    if (isAncestorDisabledValue)
                    {
                        m_builder.Attribute(Nodes::PropertyEditor::AncestorDisabled, true);
                    }
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

            if (access.GetType() == azrtti_typeid<AZStd::string>())
            {
                ExtractLabel(attributes);
                AZStd::string& value = *reinterpret_cast<AZStd::string*>(access.Get());
                VisitValue(
                    Dom::Utils::ValueFromType(value),
                    &value,
                    attributes,
                    [&value](const Dom::Value& newValue)
                    {
                        value = newValue.GetString();
                        return newValue;
                    },
                    false, false);
                return;
            }
            else
            {
                AZStd::string_view labelAttribute = "MISSING_LABEL";
                Dom::Value label = attributes.Find(Reflection::DescriptorAttributes::Label);
                if (!label.IsNull())
                {
                    if (!label.IsString())
                    {
                        AZ_Warning("DPE", false, "Unable to read Label from property, Label was not a string");
                    }
                    else
                    {
                        labelAttribute = label.GetString();
                    }
                }

                auto containerAttribute = attributes.Find(AZ::Reflection::DescriptorAttributes::Container);
                if (!containerAttribute.IsNull())
                {
                    auto container = AZ::Dom::Utils::ValueToTypeUnsafe<AZ::SerializeContext::IDataContainer*>(containerAttribute);
                    m_containers.SetValue(m_builder.GetCurrentPath(), BoundContainer{ container, access.Get() });
                    size_t containerSize = container->Size(access.Get());
                    if (containerSize == 1)
                    {
                        m_builder.Label(AZStd::string::format("%s (1 element)", labelAttribute.data()));
                    }
                    else
                    {
                        m_builder.Label(AZStd::string::format("%s (%zu elements)", labelAttribute.data(), container->Size(access.Get())));
                    }

                    if (!container->IsFixedSize())
                    {
                        auto disabledValue = attributes.Find(Nodes::NodeWithVisiblityControl::Disabled.GetName());
                        bool isDisabled = disabledValue.IsBool() && disabledValue.GetBool();

                        m_builder.BeginPropertyEditor<Nodes::ContainerActionButton>();
                        m_builder.Attribute(Nodes::ContainerActionButton::Action, Nodes::ContainerAction::AddElement);
                        m_builder.Attribute(Nodes::PropertyEditor::UseMinimumWidth, true);
                        if (isDisabled)
                        {
                            m_builder.Attribute(Nodes::PropertyEditor::Disabled, true);
                        }
                        m_builder.AddMessageHandler(m_adapter, Nodes::ContainerActionButton::OnActivate.GetName());
                        m_builder.EndPropertyEditor();

                        m_builder.BeginPropertyEditor<Nodes::ContainerActionButton>();
                        m_builder.Attribute(Nodes::PropertyEditor::SharePriorColumn, true);
                        m_builder.Attribute(Nodes::PropertyEditor::UseMinimumWidth, true);
                        m_builder.Attribute(Nodes::PropertyEditor::Alignment, Nodes::PropertyEditor::Align::AlignRight);
                        m_builder.Attribute(Nodes::ContainerActionButton::Action, Nodes::ContainerAction::Clear);
                        if (isDisabled)
                        {
                            m_builder.Attribute(Nodes::PropertyEditor::Disabled, true);
                        }
                        m_builder.AddMessageHandler(m_adapter, Nodes::ContainerActionButton::OnActivate.GetName());
                        m_builder.EndPropertyEditor();
                    }
                }
                else
                {
                    m_builder.Label(labelAttribute.data());
                }

                AZ::Dom::Value instancePointerValue = AZ::Dom::Utils::MarshalTypedPointerToValue(access.Get(), access.GetType());
                bool hashValue = false;
                const AZ::Name PointerTypeFieldName = AZ::Dom::Utils::PointerTypeFieldName;
                if (instancePointerValue.IsOpaqueValue() || instancePointerValue.FindMember(PointerTypeFieldName))
                {
                    hashValue = true;
                }
                VisitValue(
                    instancePointerValue,
                    access.Get(),
                    attributes,
                    // this needs to write the value back into the reflected object via Json serialization
                    [valuePointer = access.Get(), valueType = access.GetType(), this](const Dom::Value& newValue)
                    {
                        // marshal this new value into a pointer for use by the Json serializer
                        auto marshalledPointer = AZ::Dom::Utils::TryMarshalValueToPointer(newValue, valueType);

                        rapidjson::Document buffer;
                        JsonSerializerSettings serializeSettings;
                        JsonDeserializerSettings deserializeSettings;
                        serializeSettings.m_serializeContext = m_serializeContext;
                        deserializeSettings.m_serializeContext = m_serializeContext;

                        // serialize the new value to Json, using the original valuePointer as a reference object to generate a minimal diff
                        JsonSerialization::Store(buffer, buffer.GetAllocator(), marshalledPointer, valuePointer, valueType, serializeSettings);

                        // now deserialize that value into the original location
                        JsonSerialization::Load(valuePointer, valueType, buffer, deserializeSettings);

                        // NB: the returned value for serialized pointer values is instancePointerValue, but since this is passed by pointer,
                        // it will not actually detect a changed dom value. Since we are already writing directly to the DOM before this step,
                        // it won't affect the calling DPE, however, other DPEs pointed at the same adapter would be unaware of the change,
                        // and wouldn't update their UI.
                        // In future, to properly support multiple DPEs on one adapter, we will need to solve this. One way would be to store
                        // the json serialized value (which is mostly human-readable text) as an attribute, so any change to the Json would
                        // trigger an update. This would have the advantage of allowing opaque and pointer types to be searchable by the
                        // string-based Filter adapter. Without this, things like Vector3 will not have searchable values by text. These
                        // advantages would have to be measured against the size changes in the DOM and the time taken to populate and parse them.
                        return newValue;
                    },
                    false, hashValue);
            }
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

        // new top-value, do a full reset
        NotifyResetDocument(DocumentResetType::HardReset);
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
            PropertyRefreshLevel level = changeNotify.GetValue();
            if (level != PropertyRefreshLevel::Undefined && level != PropertyRefreshLevel::None)
            {
                PropertyEditor::RequestTreeUpdate.InvokeOnDomNode(domNode, level);
            }
        }
    }

    void ReflectionAdapter::ConnectPropertyChangeHandler(PropertyChangeEvent::Handler& handler)
    {
        handler.Connect(m_propertyChangeEvent);
    }

    void ReflectionAdapter::NotifyPropertyChanged(const PropertyChangeInfo& changeInfo)
    {
        m_propertyChangeEvent.Signal(changeInfo);
    }

    Dom::Value ReflectionAdapter::GenerateContents()
    {
        m_impl->m_builder.BeginAdapter();
        m_impl->m_builder.AddMessageHandler(this, Nodes::Adapter::QueryKey);
        m_impl->m_builder.AddMessageHandler(this, Nodes::Adapter::AddContainerKey);
        m_impl->m_builder.AddMessageHandler(this, Nodes::Adapter::RejectContainerKey);
        m_impl->m_builder.AddMessageHandler(this, Nodes::Adapter::SetNodeDisabled);
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
        auto handlePropertyEditorChanged = [&](const Dom::Value& valueFromEditor, Nodes::ValueChangeType changeType)
        {
            auto changeHandler = m_impl->m_onChangedCallbacks.ValueAtPath(message.m_messageOrigin, AZ::Dom::PrefixTreeMatch::ExactPath);
            if (changeHandler != nullptr)
            {
                Dom::Value newValue = (*changeHandler)(valueFromEditor);
                NotifyContentsChanged({ Dom::PatchOperation::ReplaceOperation(message.m_messageOrigin / "Value", newValue) });
                NotifyPropertyChanged({ message.m_messageOrigin, newValue, changeType });
            }
        };

        auto handleSetNodeDisabled = [&](bool shouldDisable, Dom::Path targetNodePath)
        {
            Dom::Value targetNode = GetContents()[targetNodePath];

            if (!targetNode.IsNode() || targetNode.IsNull())
            {
                AZ_Warning("ReflectionAdapter",
                    false,
                    "Failed to update disabled state for Value at path `%s`; this is not a valid node",
                    targetNodePath.ToString().c_str());
                return;
            }

            const Name& disabledAttributeName = Nodes::NodeWithVisiblityControl::Disabled.GetName();
            const Name& ancestorDisabledAttrName = Nodes::NodeWithVisiblityControl::AncestorDisabled.GetName();

            Dom::Patch patch;
            AZStd::stack<AZStd::pair<Dom::Path, const Dom::Value*>> unvisitedDescendants;

            const auto queueDescendantsForSearch = [&unvisitedDescendants](const Dom::Value& parentNode, const Dom::Path& parentPath)
            {
                int index = 0;
                for (auto child = parentNode.ArrayBegin(); child != parentNode.ArrayEnd(); ++child)
                {
                    if (child->IsNode())
                    {
                        unvisitedDescendants.push({ parentPath / index, child });
                    }
                    ++index;
                }
            };

            const auto propagateAttributeChangeToRow = [&](
                const Dom::Value& parentNode,
                const Dom::Path& parentPath,
                AZStd::function<void(const Dom::Value&, const Dom::Path&)> procedure)
            {
                int index = 0;
                for (auto child = parentNode.ArrayBegin(); child != parentNode.ArrayEnd(); ++child)
                {
                    if (child->IsNode())
                    {
                        auto childPath = parentPath / index;
                        if (child->GetNodeName() != GetNodeName<Nodes::Row>())
                        {
                            procedure(*child, childPath);
                        }
                        queueDescendantsForSearch(*child, childPath);
                    }
                    ++index;
                }
            };

            // This lambda applies the attribute change to any descendants in unvisitedChildren until its done
            const auto propagateAttributeChangeToDescendants = [&](AZStd::function<void(const Dom::Value&, Dom::Path&)> procedure)
            {
                while (!unvisitedDescendants.empty())
                {
                    Dom::Path nodePath = unvisitedDescendants.top().first;
                    auto node = unvisitedDescendants.top().second;
                    unvisitedDescendants.pop();

                    if (node->GetNodeName() != GetNodeName<Nodes::Row>())
                    {
                        procedure(*node, nodePath);
                    }

                    // We can stop traversing this path if the node has a truthy disabled attribute since its descendants
                    // should retain their inherited disabled state
                    if (auto iter = node->FindMember(disabledAttributeName); iter == node->MemberEnd() || !iter->second.GetBool())
                    {
                        queueDescendantsForSearch(*node, nodePath);
                    }
                }
            };

            if (shouldDisable)
            {
                if (targetNode.GetNodeName() == GetNodeName<Nodes::Row>())
                {
                    propagateAttributeChangeToRow(targetNode,
                        targetNodePath,
                        [&patch, &disabledAttributeName](const Dom::Value& node, const Dom::Path& nodePath)
                        {
                            if (auto iter = node.FindMember(disabledAttributeName); iter == node.MemberEnd() || !iter->second.GetBool())
                            {
                                patch.PushBack({ Dom::PatchOperation::AddOperation(nodePath / disabledAttributeName, Dom::Value(true)) });
                            }
                        });
                }
                else
                {
                    patch.PushBack({ Dom::PatchOperation::AddOperation(targetNodePath / disabledAttributeName, Dom::Value(true)) });
                    queueDescendantsForSearch(targetNode, targetNodePath);
                }

                propagateAttributeChangeToDescendants(
                    [&patch, &ancestorDisabledAttrName](const Dom::Value& node, const Dom::Path& nodePath)
                    {
                        if (auto iter = node.FindMember(ancestorDisabledAttrName); iter == node.MemberEnd() || !iter->second.GetBool())
                        {
                            patch.PushBack({ Dom::PatchOperation::AddOperation(nodePath / ancestorDisabledAttrName, Dom::Value(true)) });
                        }
                    });
            }
            else
            {
                if (targetNode.GetNodeName() == GetNodeName<Nodes::Row>())
                {
                    propagateAttributeChangeToRow(targetNode,
                        targetNodePath,
                        [&patch, &disabledAttributeName](const Dom::Value& node, const Dom::Path& nodePath)
                        {
                            if (auto iter = node.FindMember(disabledAttributeName); iter != node.MemberEnd() && iter->second.GetBool())
                            {
                                patch.PushBack({ Dom::PatchOperation::RemoveOperation(nodePath / disabledAttributeName) });
                            }
                        });
                }
                else
                {
                    patch.PushBack({ Dom::PatchOperation::RemoveOperation(targetNodePath / disabledAttributeName) });
                    queueDescendantsForSearch(targetNode, targetNodePath);
                }

                propagateAttributeChangeToDescendants(
                    [&patch, &ancestorDisabledAttrName](const Dom::Value& node, const Dom::Path& nodePath)
                    {
                        if (auto iter = node.FindMember(ancestorDisabledAttrName); iter != node.MemberEnd() && iter->second.GetBool())
                        {
                            patch.PushBack({ Dom::PatchOperation::RemoveOperation(nodePath / ancestorDisabledAttrName) });
                        }
                    });
            }

            if (patch.Size() > 0)
            {
                NotifyContentsChanged(patch);
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
                auto action = Nodes::ContainerActionButton::Action.ExtractFromDomNode(node);
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
        auto addKeyToContainer = [&](AZ::DocumentPropertyEditor::DocumentAdapterPtr* adapter, AZ::Dom::Path containerPath)
        {
            ReflectionAdapter* actualAdapter = static_cast<ReflectionAdapter*>(adapter->get());
            auto containerEntry = m_impl->m_containers.ValueAtPath(containerPath, AZ::Dom::PrefixTreeMatch::ParentsOnly);
            void* keyInstance = actualAdapter->GetInstance();

            containerEntry->m_container->StoreElement(containerEntry->m_instance, keyInstance);
            auto containerNode = containerEntry->GetContainerNode(m_impl.get(), containerPath);
            Nodes::PropertyEditor::AddNotify.InvokeOnDomNode(containerNode);
            Nodes::PropertyEditor::ChangeNotify.InvokeOnDomNode(containerNode);
            NotifyResetDocument();
        };

        auto rejectKeyToContainer = [&](AZ::DocumentPropertyEditor::DocumentAdapterPtr * adapter, AZ::Dom::Path containerPath)
        {
            ReflectionAdapter* actualAdapter = static_cast<ReflectionAdapter*>(adapter->get());
            auto containerEntry = m_impl->m_containers.ValueAtPath(containerPath, AZ::Dom::PrefixTreeMatch::ParentsOnly);
            void* keyInstance = actualAdapter->GetInstance();
            containerEntry->m_container->FreeReservedElement(containerEntry->m_instance, keyInstance, m_impl->m_serializeContext);
        };

        auto handleTreeUpdate = [&](Nodes::PropertyRefreshLevel)
        {
            // For now just trigger a soft reset but the end goal is to handle granular updates.
            // This will still only send the view patches for what's actually changed.
            NotifyResetDocument();
        };

        return message.Match(
            Nodes::PropertyEditor::OnChanged, handlePropertyEditorChanged,
            Nodes::ContainerActionButton::OnActivate, handleContainerOperation,
            Nodes::PropertyEditor::RequestTreeUpdate, handleTreeUpdate,
            Nodes::Adapter::SetNodeDisabled, handleSetNodeDisabled,
            Nodes::Adapter::AddContainerKey, addKeyToContainer,
            Nodes::Adapter::RejectContainerKey, rejectKeyToContainer
            );
    }
} // namespace AZ::DocumentPropertyEditor
