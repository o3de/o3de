/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef PROPERTYEDITORAPI_INTERNALS_H
#define PROPERTYEDITORAPI_INTERNALS_H

#include "InstanceDataHierarchy.h"

// this header contains internal template magics for the way properties work
// You shouldn't need to work on this or modify it - instead
// A user is expected to derive from PropertyHandler<PropertyType, WidgetType>
// and implement that interface, then register it with the property manager.

#include <AzCore/Debug/Profiler.h>
#include <AzCore/DOM/DomUtils.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Prefab/PrefabEditorPreferences.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyEditorToolsSystemInterface.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystemInterface.h>

#include <QMessageBox>

class QWidget;
class QColor;
class QString;
class QPoint;

AZ_DECLARE_BUDGET(AzToolsFramework);

namespace AzToolsFramework
{
    namespace Components
    {
        class PropertyManagerComponent;
    }

    // Alias the AZ Core Attribute Reader
    using PropertyAttributeReader = AZ::AttributeReader;
    class PropertyRowWidget;
    class InstanceDataNode;

    // if you embed a property editor control in one of your widgets, you can (optionally) set hooks to get these events.
    // chances are, users don't have to worry about this.
    class IPropertyEditorNotify
    {
    public:
        virtual ~IPropertyEditorNotify() = default;

        // this function gets called each time you are about to actually modify a property (not when the editor opens)
        virtual void BeforePropertyModified(InstanceDataNode* pNode) = 0;

        // this function gets called each time a property is actually modified (not just when the editor appears),
        // for each and every change - so for example, as a slider moves.
        // its meant for undo state capture.
        virtual void AfterPropertyModified(InstanceDataNode* pNode) = 0;

        // this funciton is called when some stateful operation begins, such as dragging starts in the world editor or such
        // in which case you don't want to blow away the tree and rebuild it until editing is complete since doing so is
        // flickery and intensive.
        virtual void SetPropertyEditingActive(InstanceDataNode* pNode) = 0;
        virtual void SetPropertyEditingComplete(InstanceDataNode* pNode) = 0;

        // this will cause the current undo operation to complete, sealing it and beginning a new one if there are further edits.
        virtual void SealUndoStack() = 0;

        // allows a listener to implement a context menu for a given node's property.
        virtual void RequestPropertyContextMenu(InstanceDataNode*, const QPoint&) {}

        // allows a listener to respond to selection change.
        virtual void PropertySelectionChanged(InstanceDataNode *, bool) {}
    };

    //! Notification bus for edit events from individual RPE widgets.
    //! Used exclusively by RpePropertyHandlerWrapper.
    class IndividualPropertyHandlerEditNotifications : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<IndividualPropertyHandlerEditNotifications>;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = QWidget*;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void OnValueChanged(AZ::DocumentPropertyEditor::Nodes::ValueChangeType changeType) = 0;
        virtual void OnRequestPropertyNotify() = 0;
    };

    class PropertyHandlerWidgetInterface;

    // this serves as the base class for all property managers.
    // you do not use this class directly.
    // derive from PropertyHandler instead.
    class PropertyHandlerBase
    {
        friend class ReflectedPropertyEditor;
        friend PropertyRowWidget;
        friend class Components::PropertyManagerComponent;
        template<typename WrappedType>
        friend class RpePropertyHandlerWrapper;

    public:
        PropertyHandlerBase();
        virtual ~PropertyHandlerBase();

        // This should be overriden by property handlers that need to register their own
        // adapter elements (nodes, property editors, attributes)
        virtual void RegisterWithPropertySystem(AZ::DocumentPropertyEditor::PropertyEditorSystemInterface* /*system*/) {}

        // you need to define this.
        virtual AZ::u32 GetHandlerName() const = 0;  // AZ_CRC_CE("IntSlider")

        // specify true if you want the user to be able to either specify no handler or specify "Default" as the handler.
        // and still get your handler.
        virtual bool IsDefaultHandler() const { return false; }

        // if this is set to true, then shutting down the Property Manager will delete this property handler for you
        // otherwise, if you have a system where the property handler needs to only exist when some plugin is loaded or component is active, then you need to
        // instead return false for AutoDelete, and call UnregisterPropertyType yourself.
        virtual bool AutoDelete() const { return true; }

        // this allows you to register multiple handlers and override our default handlers which always have priority 0
        virtual int Priority() const { return 0; }

        // create an instance of the GUI that is used to edit this property type.
        // the QWidget pointer you return also serves as a handle for accessing data.  This means that in order to trigger
        // a write, you need to call RequestWrite(...) on that same widget handle you return.
        virtual QWidget* CreateGUI(QWidget* pParent) = 0;

        virtual void DestroyGUI(QWidget* pTarget);

        // If the control wants to modify or replace the display name of an attribute,
        // it can be done in this function.
        // Return true if any modifications were made to the nameLabelString.
        virtual bool ModifyNameLabel(QWidget* /*widget*/, QString& /*nameLabelString*/) { return false; }

        // If the control wants to add anything to the tooltip of an attribute (like adding a range to a Spin Control),
        // it can be done in this function.
        // Return true if any modifications were made to the toolTipString.
        virtual bool ModifyTooltip(QWidget* /*widget*/, QString& /*toolTipString*/) { return false; }

        // If the control needs to do anything when refreshes have been disabled, such as cancelling async / threaded
        // updates, it can be done in this function.
        virtual void PreventRefresh(QWidget* /*widget*/, bool /*shouldPrevent*/) {}

        //! Registers this handler for usage in the DocumentPropertyHandler.
        //! GenericPropertyHandler handles this for most cases.
        virtual void RegisterDpeHandler()
        {
        }

        //! Unregisters this handler from the DocumentPropertyHandler.
        //! GenericPropertyHandler handles this for most cases.
        virtual void UnregisterDpeHandler()
        {
        }

    protected:
        // we automatically take care of the rest:
        // --------------------- Internal Implementation ------------------------------
        virtual bool ResetGUIToDefaults_Internal(QWidget* widget) = 0;
        virtual void ConsumeAttributes_Internal(QWidget* widget, InstanceDataNode* attrValue) = 0;
        virtual void WriteGUIValuesIntoProperty_Internal(QWidget* widget, InstanceDataNode* t) = 0;
        virtual void WriteGUIValuesIntoTempProperty_Internal(QWidget* widget, void* tempValue, const AZ::Uuid& propertyType, AZ::SerializeContext* serializeContext) = 0;
        virtual void ReadValuesIntoGUI_Internal(QWidget* widget, InstanceDataNode* t) = 0;
        // we define this automatically for you, you don't have to override it.
        virtual bool HandlesType(const AZ::Uuid& id) const = 0;
        virtual AZ::TypeId GetHandledType() const = 0;
        virtual QWidget* GetFirstInTabOrder_Internal(QWidget* widget) = 0;
        virtual QWidget* GetLastInTabOrder_Internal(QWidget* widget) = 0;
        virtual void UpdateWidgetInternalTabbing_Internal(QWidget* widget) = 0;
    };

    // Wrapper type that takes a PropertyHandleBase from the ReflectedPropertyEditor and
    // provides a PropertyHandlerWidgetInterface for the DocumentPropertyEditor.
    // Doesn't use the normal static ShouldHandleType and GetHandlerName implementations,
    // so must be custom registered to the PropertyEditorToolsSystemInterface.
    template<typename WrappedType>
    class RpePropertyHandlerWrapper
        : public PropertyHandlerWidgetInterface
        , public IndividualPropertyHandlerEditNotifications::Bus::Handler
    {
    public:
        explicit RpePropertyHandlerWrapper(PropertyHandlerBase& handlerToWrap)
            : m_rpeHandler(handlerToWrap)
        {
            m_proxyNode.m_classData = &m_proxyClassData;
            m_proxyNode.m_classElement = &m_proxyClassElement;
            m_proxyClassData.m_typeId = azrtti_typeid<WrappedType>();
            m_proxyNode.m_instances.push_back(&m_proxyValue);
        }

        ~RpePropertyHandlerWrapper()
        {
            if (m_widget)
            {
                // Detect whether this is being run in the Editor or during a Unit Test.
                AZ::ApplicationTypeQuery appType;
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
                if (appType.IsValid() && !appType.IsEditor())
                {
                    // In Unit Tests, immediately delete the widget to prevent triggering the leak detection mechanism.
                    delete m_widget;
                    m_widget = nullptr;
                }
                else
                {
                    // In the Editor, use deleteLater as it is more stable.
                    m_widget->deleteLater();
                }
            }
            IndividualPropertyHandlerEditNotifications::Bus::Handler::BusDisconnect();
        }

        QWidget* GetWidget() override
        {
            if (m_widget)
            {
                return m_widget;
            }
            m_widget = m_rpeHandler.CreateGUI(nullptr);
            IndividualPropertyHandlerEditNotifications::Bus::Handler::BusConnect(m_widget);
            return m_widget;
        }

        void SetValueFromDom(const AZ::Dom::Value& node)
        {
            using AZ::DocumentPropertyEditor::Nodes::PropertyEditor;

            auto propertyEditorSystem = AZ::Interface<AZ::DocumentPropertyEditor::PropertyEditorSystemInterface>::Get();
            AZ_Assert(
                propertyEditorSystem != nullptr,
                "RpePropertyHandlerWrapper::SetValueFromDom called with an uninitialized PropertyEditorSystemInterface");
            if (propertyEditorSystem == nullptr)
            {
                return;
            }

            m_proxyClassElement.m_attributes.clear();
            for (auto attributeIt = node.MemberBegin(); attributeIt != node.MemberEnd(); ++attributeIt)
            {
                const AZ::Name& name = attributeIt->first;

                if (name == PropertyEditor::Type.GetName() || name == PropertyEditor::Value.GetName() ||
                    name == PropertyEditor::ValueType.GetName())
                {
                    continue;
                }
                else if (name == PropertyEditor::ParentValue.GetName())
                {
                    auto parentValue = PropertyEditor::ParentValue.ExtractFromDomNode(node);
                    if (parentValue.has_value())
                    {
                        // Retrieve memory address of parent value
                        void* parentValuePtr{};
                        // First check if the parent value is a PointerObject
                        if (auto parentValuePointerObjectOpt = AZ::Dom::Utils::ValueToType<AZ::PointerObject>(parentValue.value());
                            parentValuePointerObjectOpt && parentValuePointerObjectOpt->IsValid())
                        {
                            parentValuePtr = parentValuePointerObjectOpt->m_address;
                        }
                        else if (auto parentValueVoidOpt = AZ::Dom::Utils::ValueToType<void*>(parentValue.value());
                            parentValueVoidOpt)
                        {
                            parentValuePtr = *parentValueVoidOpt;
                        }
                        AZ_Assert(parentValuePtr, "Parent instance was nullptr when attempting to add to instance list.");

                        // Only add parent instance if it has not been added previously
                        if (auto foundParentIt = AZStd::find(m_proxyParentNode.m_instances.begin(), m_proxyParentNode.m_instances.end(), parentValuePtr);
                            foundParentIt == m_proxyParentNode.m_instances.end())
                        {
                            m_proxyParentNode.m_instances.push_back(parentValuePtr);
                        }

                        // Set up the reference to parent node only if a parent value is available.
                        m_proxyNode.m_parent = &m_proxyParentNode;
                    }
                    continue;
                }
                // Use the SerializedPath as the proxy class element name, which prior to this was always an empty string
                // since it isn't used by the property handler. This helps when debugging the ConsumeAttribute method
                // in the property handlers because the class element name gets passed as the debugName, so its easier
                // to tell which property is currently processing the attributes.
                else if (name == AZ::Reflection::DescriptorAttributes::SerializedPath)
                {
                    continue;
                }

                AZ::Crc32 attributeId = AZ::Crc32(name.GetStringView());

                AZStd::shared_ptr<AZ::Attribute> marshalledAttribute;
                // Attribute definitions may be templated and will thus support multiple types.
                // Therefore we must try all registered definitions for a particular attribute
                // in an effort to find one that can successfully extract the attribute data.

                bool fallback = false;
                auto getAttributeFromValue = [&](const AZ::DocumentPropertyEditor::AttributeDefinitionInterface& attributeReader)
                {
                    if (marshalledAttribute == nullptr)
                    {
                        marshalledAttribute = attributeReader.DomValueToLegacyAttribute(attributeIt->second, fallback);
                    }
                };

                // try the conversion once without type fallback
                propertyEditorSystem->EnumerateRegisteredAttributes(name, getAttributeFromValue);

                if (marshalledAttribute == nullptr)
                {
                    // still null, try it again allowing type fallback
                    fallback = true;
                    propertyEditorSystem->EnumerateRegisteredAttributes(name, getAttributeFromValue);
                }

                if (!marshalledAttribute)
                {
                    marshalledAttribute = AZ::Reflection::WriteDomValueToGenericAttribute(attributeIt->second);

                    // If we didn't find the attribute in the registered definitions, then it's one we don't have explicitly
                    // registered, so instead of the `name` being the non-hashed string (e.g. "ChangeNotify"), it will
                    // be already be the hashed version (e.g. "123456789"), so we don't need to hash it again, but rather
                    // need to use the hash directly for the attribute id.
                    if (AZ::StringFunc::LooksLikeInt(name.GetCStr()))
                    {
                        AZStd::string attributeIdString(name.GetStringView());
                        AZ::u32 attributeIdHash = static_cast<AZ::u32>(AZStd::stoul(attributeIdString));
                        attributeId = AZ::Crc32(attributeIdHash);

                        // If we failed to marshal the attribute in the above AZ::Reflection::WriteDomValueToGenericAttribute,
                        // then its either an opaque value or an Attribute object (from being a callback), so we need
                        // to handle these cases separately to marshal them into an attribute to be passed onto the
                        // handlers that will actually consume them.
                        if (!marshalledAttribute)
                        {
                            if (attributeIt->second.IsOpaqueValue() && attributeIt->second.GetOpaqueValue().is<AZ::Attribute*>())
                            {
                                AZ::Attribute* attribute = AZStd::any_cast<AZ::Attribute*>(attributeIt->second.GetOpaqueValue());
                                marshalledAttribute = AZStd::shared_ptr<AZ::Attribute>(
                                    attribute,
                                    [](AZ::Attribute*)
                                    {
                                    });
                            }
                            else if (attributeIt->second.IsObject())
                            {
                                auto typeField = attributeIt->second.FindMember(AZ::Attribute::GetTypeField());
                                if (typeField != attributeIt->second.MemberEnd() && typeField->second.IsString() &&
                                    typeField->second.GetString() == AZ::Attribute::GetTypeName())
                                {
                                    AZ::Attribute* attribute =
                                        AZ::Dom::Utils::ValueToTypeUnsafe<AZ::Attribute*>(attributeIt->second[AZ::Attribute::GetAttributeField()]);
                                    marshalledAttribute = AZStd::shared_ptr<AZ::Attribute>(
                                        attribute,
                                        [](AZ::Attribute*)
                                        {
                                        });
                                }
                            }
                        }
                    }
                }

                if (marshalledAttribute)
                {
                    m_proxyClassElement.m_attributes.emplace_back(attributeId, AZStd::move(marshalledAttribute));
                }
            }

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(serializeContext, "Serialization context not available");

            AZ::TypeId typeId;
            auto typeIdAttribute = node.FindMember(PropertyEditor::ValueType.GetName());
            if (typeIdAttribute != node.MemberEnd())
            {
                typeId = AZ::Dom::Utils::DomValueToTypeId(typeIdAttribute->second);
            }

            auto value = AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Value.ExtractFromDomNode(node);
            if (value.has_value() && (!value->IsObject() || !value->ObjectEmpty()))
            {
                if (auto valueToType = AZ::Dom::Utils::ValueToType<WrappedType>(value.value()); valueToType)
                {
                    m_proxyValue = *valueToType;

                    if (typeId.IsNull())
                    {
                        typeId = m_proxyClassData.m_typeId;
                    }
                }
                else if (auto valueToPointerObject = AZ::Dom::Utils::ValueToType<AZ::PointerObject>(value.value()); valueToPointerObject && valueToPointerObject->IsValid())
                {
                    if (valueToPointerObject->m_typeId == m_proxyClassData.m_typeId)
                    {
                        m_proxyValue = *reinterpret_cast<WrappedType*>(valueToPointerObject->m_address);
                        typeId = valueToPointerObject->m_typeId;
                    }
                    else
                    {
                        const AZ::SerializeContext::ClassData* pointerClassData =
                            serializeContext != nullptr ? serializeContext->FindClassData(valueToPointerObject->m_typeId) : nullptr;

                        auto proxyClassData = serializeContext->FindClassData(m_proxyClassData.m_typeId);

                        if (pointerClassData != nullptr && pointerClassData->m_azRtti != nullptr &&
                            pointerClassData->m_azRtti->IsTypeOf<WrappedType>())
                        {
                            if (auto castInstance = pointerClassData->m_azRtti->Cast<WrappedType>(valueToPointerObject->m_address); castInstance != nullptr)
                            {
                                m_proxyValue = *castInstance;
                                // Set the type ID to the derived class ID
                                typeId = valueToPointerObject->m_typeId;
                            }
                        }
                        else if (proxyClassData != nullptr && proxyClassData->CanConvertFromType(valueToPointerObject->m_typeId, *serializeContext))
                        {
                            // Otherwise attempt to check if the proxy value is convertible to the TypeId type
                            // such as for the case of an AZ::Data::Asset<T> from an AZ::Data::Asset<AZ::Data::AssetData>
                            typeId = valueToPointerObject->m_typeId;
                            m_proxyValue = *reinterpret_cast<WrappedType*>(valueToPointerObject->m_address);
                        }
                    }
                }
            }

            if (typeId.IsNull() && value.has_value())
            {
                typeId = AZ::Dom::Utils::GetValueTypeId(value.value());
            }

            // Set the m_genericClassInfo (if any) for our type in case the property handler needs to access it
            // when downcasting to the generic type (e.g. for asset property downcasting to the generic AZ::Data::Asset<AZ::Data::AssetData>)
            if (!typeId.IsNull())
            {
                m_proxyClassElement.m_genericClassInfo = serializeContext->FindGenericClassInfo(typeId);
            }

            m_rpeHandler.ConsumeAttributes_Internal(GetWidget(), &m_proxyNode);
            m_rpeHandler.ReadValuesIntoGUI_Internal(GetWidget(), &m_proxyNode);

            m_domNode = node;
        }

        bool ResetToDefaults() override
        {
            if (m_widget)
            {
                // Reset widget's attributes before reading in new values
                return m_rpeHandler.ResetGUIToDefaults_Internal(m_widget);
            }
            return false;
        }

        QWidget* GetFirstInTabOrder() override
        {
            return m_rpeHandler.GetFirstInTabOrder_Internal(GetWidget());
        }

        QWidget* GetLastInTabOrder() override
        {
            return m_rpeHandler.GetLastInTabOrder_Internal(GetWidget());
        }

        static bool ShouldHandleType(PropertyHandlerBase& rpeHandler, const AZ::TypeId& typeId)
        {
            return rpeHandler.HandlesType(typeId);
        }

        static const AZStd::string_view GetHandlerName(PropertyHandlerBase& rpeHandler)
        {
            auto propertyEditorSystem = AZ::Interface<AZ::DocumentPropertyEditor::PropertyEditorSystemInterface>::Get();
            AZ_Assert(
                propertyEditorSystem != nullptr,
                "RpePropertyHandlerWrapper::GetHandlerName called with an uninitialized PropertyEditorSystemInterface");
            if (propertyEditorSystem == nullptr)
            {
                return {};
            }
            return propertyEditorSystem->LookupNameFromId(rpeHandler.GetHandlerName()).GetStringView();
        }

        void OnValueChanged(AZ::DocumentPropertyEditor::Nodes::ValueChangeType changeType) override
        {
            using AZ::DocumentPropertyEditor::Nodes::PropertyEditor;

            auto typeIdAttribute = m_domNode.FindMember(PropertyEditor::ValueType.GetName());
            AZ::TypeId typeId = AZ::TypeId::CreateNull();
            if (typeIdAttribute != m_domNode.MemberEnd())
            {
                typeId = AZ::Dom::Utils::DomValueToTypeId(typeIdAttribute->second);
            }

            // If a ChangeValidate method was specified, verify the new value with it before continuing.
            if (m_domNode.FindMember(PropertyEditor::ChangeValidate.GetName()) != m_domNode.MemberEnd())
            {
                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                AZ_Assert(serializeContext, "Serialization context not available");

                AZStd::any tempValue = serializeContext->CreateAny(typeId);
                void* tempValueRef = AZStd::any_cast<void>(&tempValue);

                m_rpeHandler.WriteGUIValuesIntoTempProperty_Internal(GetWidget(), tempValueRef, typeId, serializeContext);

                auto validateOutcome = PropertyEditor::ChangeValidate.InvokeOnDomNode(m_domNode, tempValueRef, typeId);
                if (validateOutcome.IsSuccess())
                {
                    // If the outcome is a failure, the change was rejected, so show an error message popup
                    // with the outcome error and don't continue processing the OnValueChanged.
                    AZ::Outcome<void, AZStd::string> outcome = validateOutcome.GetValue();
                    if (!outcome.IsSuccess())
                    {
                        if (changeType == AZ::DocumentPropertyEditor::Nodes::ValueChangeType::InProgressEdit)
                        {
                            QMessageBox::warning(AzToolsFramework::GetActiveWindow(), "Invalid Assignment", outcome.GetError().c_str(), QMessageBox::Ok);

                            // Force the values to update so that they are correct since something just declined changes and
                            // we want the UI to display the current values and not the invalid ones
                            ToolsApplicationNotificationBus::Broadcast(
                                &ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
                        }

                        return;
                    }
                }
            }

            m_rpeHandler.WriteGUIValuesIntoProperty_Internal(GetWidget(), &m_proxyNode);

            // If the expected TypeId differs from m_proxyClassData.m_typeId (WrappedType),
            // then it means this handler was written for a base-class/m_proxyClassElement.m_genericClassInfo
            //      e.g. AZ::Data::Asset<AZ::Data::AssetData> when the actual type is for a specific asset data such as
            //           AZ::Data::Asset<RPI::ModelAsset>
            // So we need to marshal it with a typed pointer, which is how the data gets read in as well
            // NOTE: The exception to this is for enum types, which might have a specialized type vs. an underlying type (e.g. short, int,
            // etc...) in which case, the proxy value still comes as a primitive instead of a pointer, so we still need to use ValueFromType
            // for that case. For everything else, we can just use ValueFromType which can imply the type from the value itself.

            // Use the serialize context to lookup the AZStd any action handler
            AZ::SerializeContext* serializeContext{};
            if (auto componentApplicationRequests = AZ::Interface<AZ::ComponentApplicationRequests>::Get();
                componentApplicationRequests != nullptr)
            {
                serializeContext = componentApplicationRequests->GetSerializeContext();
            }

            AZ::Dom::Value newValue;
            bool newValueSet{};
            if (const AZ::SerializeContext::ClassData* classData = serializeContext != nullptr ? serializeContext->FindClassData(typeId) : nullptr;
                classData != nullptr && !typeId.IsNull() && (typeId != m_proxyClassData.m_typeId) && !m_domNode.HasMember(PropertyEditor::EnumType.GetName()))
            {
                // Cast the proxy value to typeId type if is a pointer
                void* proxyAddress{};
                if constexpr(AZStd::is_pointer_v<WrappedType>)
                {
                    if (auto proxyRtti = m_proxyClassData.m_azRtti; proxyRtti != nullptr)
                    {
                        proxyAddress = proxyRtti->Cast(m_proxyValue, typeId);
                    }
                }
                else if (classData->CanConvertFromType(m_proxyClassData.m_typeId, *serializeContext))
                {
                    // Otherwise attempt to check if the proxy value is convertible to the TypeId type
                    // such as for the case of an AZ::Data::Asset<T> from an AZ::Data::Asset<AZ::Data::AssetData>
                    void* sourceAddress{};
                    if constexpr (AZStd::is_pointer_v<WrappedType>)
                    {
                        sourceAddress = m_proxyValue;
                    }
                    else
                    {
                        sourceAddress = &m_proxyValue;
                    }
                    classData->ConvertFromType(proxyAddress, m_proxyClassData.m_typeId, sourceAddress, *serializeContext);
                }

                if (proxyAddress != nullptr)
                {
                    AZ::Dom::Utils::MarshalTypeTraits marshalTypeTraits;
                    marshalTypeTraits.m_typeId = typeId;
                    marshalTypeTraits.m_isPointer = AZStd::is_pointer_v<WrappedType>;
                    marshalTypeTraits.m_isReference = AZStd::is_reference_v<WrappedType>;
                    marshalTypeTraits.m_isCopyConstructible = AZStd::is_copy_constructible_v<WrappedType>;
                    marshalTypeTraits.m_typeSize = classData->m_azRtti!= nullptr ? classData->m_azRtti->GetTypeSize() : sizeof(WrappedType);

                    if (classData->m_createAzStdAnyActionHandler)
                    {
                        auto anyActionHandler = classData->m_createAzStdAnyActionHandler(serializeContext);
                        newValue = AZ::Dom::Utils::ValueFromType(proxyAddress, marshalTypeTraits, AZStd::move(anyActionHandler));
                        newValueSet = true;
                    }
                }
            }
            else
            {
                newValue = AZ::Dom::Utils::ValueFromType(m_proxyValue);
                newValueSet = true;
            }

            if (newValueSet)
            {
                PropertyEditor::OnChanged.InvokeOnDomNode(m_domNode, newValue, changeType);
            }

            // Only trigger the ChangeNotify in response to PropertyEditorGUIMessages::RequestWrite.
            // Also, we trigger this regardless of newValueSet, because of cases like a UIElement
            // with no backing data element where there will be no value getting set but we still
            // need to trigger the change notify in case the user has some logic that needs to
            // be executed.
            if (changeType == AZ::DocumentPropertyEditor::Nodes::ValueChangeType::InProgressEdit)
            {
                OnRequestPropertyNotify();
            }
        }

        void OnRequestPropertyNotify() override
        {
            AZ::DocumentPropertyEditor::ReflectionAdapter::InvokeChangeNotify(m_domNode);
        }

    private:
        PropertyHandlerBase& m_rpeHandler;
        AZ::Dom::Value m_domNode;
        QPointer<QWidget> m_widget;
        InstanceDataNode m_proxyNode;
        InstanceDataNode m_proxyParentNode;
        AZ::SerializeContext::ClassData m_proxyClassData;
        AZ::SerializeContext::ClassElement m_proxyClassElement;
        WrappedType m_proxyValue{};
    };

    template <class WidgetType>
    class PropertyHandler_Internal
        : public PropertyHandlerBase
    {
    public:
        typedef WidgetType widget_t;

        // Resets widget attributes for reuse; returns true if widget was reset
        // if you return false, the widget will be destroyed and recreated on each reuse, so implementing this
        // can improve response speed and reduce flicker.  On the other hand, actually resetting a really complicated
        // widget can involve cleaning out unexpected amounts of hidden state in a tree of child widgets, which themselves
        // may have complicated hidden state, so implement it with care and test it extensively.
        virtual bool ResetGUIToDefaults([[maybe_unused]] WidgetType* widget)
        {
            return false;
        }

        // see documentation in PropertyEditorAPI.h in @ref class PropertyHandler
        virtual void BeforeConsumeAttributes(WidgetType* widget, InstanceDataNode* attrValue) = 0;

        // this will be called in order to initialize your gui.  Your class will be fed one attribute at a time
        // you can interpret the attributes as you wish - use attrValue->Read<int>() for example, to interpret it as an int.
        // all attributes can either be a flat value, or a function which returns that same type.  In the case of the function
        // it will be called on the first instance.
        virtual void ConsumeAttribute(WidgetType* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) = 0;

        // provides an option to specify reading parent element attributes.
        // This allows parent elements to override attributes of their children if needed.
        virtual void ConsumeParentAttribute(WidgetType* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
        {
            (void)widget;
            (void)attrib;
            (void)attrValue;
            (void)debugName;
        }

        // override GetFirstInTabOrder, GetLastInTabOrder in your base class to define which widget gets focus first when pressing tab,
        // and also what widget is last.
        // for example, if your widget is a compound widget and contains, say, 5 buttons
        // then for GetFirstInTabOrder, return the first button
        // and for GetLastInTabOrder, return the last button.
        // and in UpdateWidgetInternalTabbing, call setTabOrder(button0, button1); setTabOrder(button1, button2)...
        // this will cause the previous row to tab to your first button, when the user hits tab
        // and cause the next row to tab to your last button, when the user hits shift+tab on the next row.
        // if your widget is a single widget or has a single focus proxy there is no need to override.

        virtual QWidget* GetFirstInTabOrder(WidgetType* widget) { return widget; }
        virtual QWidget* GetLastInTabOrder(WidgetType* widget) { return widget; }

        // implement this function in order to set your internal tab order between child controls.
        // just call a series of QWidget::setTabOrder
        virtual void UpdateWidgetInternalTabbing(WidgetType* widget)
        {
            (void)widget;
        }

    protected:
        // ---------------- INTERNAL -----------------------------
        virtual bool ResetGUIToDefaults_Internal(QWidget* widget) override
        {
            WidgetType* wid = static_cast<WidgetType*>(widget);
            return ResetGUIToDefaults(wid);
        }

        virtual void ConsumeAttributes_Internal(QWidget* widget, InstanceDataNode* dataNode) override;

        virtual QWidget* GetFirstInTabOrder_Internal(QWidget* widget) override
        {
            WidgetType* wid = static_cast<WidgetType*>(widget);
            return GetFirstInTabOrder(wid);
        }

        virtual void UpdateWidgetInternalTabbing_Internal(QWidget* widget) override
        {
            WidgetType* wid = static_cast<WidgetType*>(widget);
            return UpdateWidgetInternalTabbing(wid);
        }

        virtual QWidget* GetLastInTabOrder_Internal(QWidget* widget) override
        {
            WidgetType* wid = static_cast<WidgetType*>(widget);
            return GetLastInTabOrder(wid);
        }
    };

    template <typename PropertyType, class WidgetType>
    class TypedPropertyHandler_Internal
        : public PropertyHandler_Internal<WidgetType>
    {
    public:
        typedef PropertyType property_t;

        // WriteGUIValuesIntoProperty:  This will be called on each instance of your property type.  So for example if you have an object
        // selected, each of which has the same float property on them, and your property editor is for floats, you will get this
        // write and read function called 5x - once for each instance.
        // this is your opportunity to determine if the values are the same or different.
        // index is the index of the instance, from 0 to however many there are.  You can use this to determine if its
        // the first instance, or to check for multi-value edits if it matters to you.
        // GUI is a pointer to the GUI used to editor your property (the one you created in CreateGUI)
        // and instance is a the actual value (PropertyType).
        virtual void WriteGUIValuesIntoProperty(size_t index, WidgetType* GUI, PropertyType& instance, InstanceDataNode* node) = 0;

        // this will get called in order to initialize your gui.  It will be called once for each instance.
        // for example if you have multiple objects selected, index will go from 0 to however many there are.
        virtual bool ReadValuesIntoGUI(size_t index, WidgetType* GUI, const PropertyType& instance, InstanceDataNode* node) = 0;

        // registers this handler to the DocumentPropertyEditor
        // can be overridden to provide an alternate handler to the DPE
        // or to disable registration outright if a replacement has been provided
        void RegisterDpeHandler() override
        {
            // For non-constructible types, we can't automatically create a wrapper
            if constexpr (AZStd::is_constructible_v<PropertyType>)
            {
                AZ_Assert(m_registeredDpeHandlerId == nullptr, "RegisterDpeHandler called multiple times for the same handler");

                auto dpeSystemInterface = AZ::Interface<PropertyEditorToolsSystemInterface>::Get();
                if (dpeSystemInterface == nullptr)
                {
                    AZ_WarningOnce("dpe", false, "RegisterDpeHandler failed, PropertyEditorToolsSystemInterface was not found");
                    return;
                }

                using HandlerType = RpePropertyHandlerWrapper<PropertyType>;
                PropertyEditorToolsSystemInterface::HandlerData registrationInfo;
                registrationInfo.m_name = HandlerType::GetHandlerName(*this);
                registrationInfo.m_shouldHandleType = [this](const AZ::TypeId& typeId)
                {
                    return HandlerType::ShouldHandleType(*this, typeId);
                };
                registrationInfo.m_factory = [this]()
                {
                    return AZStd::make_unique<HandlerType>(*this);
                };
                registrationInfo.m_isDefaultHandler = this->IsDefaultHandler();
                m_registeredDpeHandlerId = dpeSystemInterface->RegisterHandler(AZStd::move(registrationInfo));
            }
        }

        // unregisters this handler from the DocumentPropertyEditor
        void UnregisterDpeHandler() override
        {
            if constexpr (AZStd::is_constructible_v<PropertyType>)
            {
                AZ_Assert(m_registeredDpeHandlerId != nullptr, "UnregisterDpeHandler called for a handler that's not registered");

                auto dpeSystemInterface = AZ::Interface<PropertyEditorToolsSystemInterface>::Get();
                if (dpeSystemInterface == nullptr)
                {
                    AZ_WarningOnce("dpe", false, "UnregisterDpeHandler failed, PropertyEditorToolsSystemInterface was not found");
                    return;
                }

                dpeSystemInterface->UnregisterHandler(m_registeredDpeHandlerId);
                m_registeredDpeHandlerId = nullptr;
            }
        }

    protected:
        // ---------------- INTERNAL -----------------------------
        virtual bool HandlesType(const AZ::Uuid& id) const override
        {
            return GetHandledType() == id;
        }

        virtual AZ::TypeId GetHandledType() const override
        {
            return AZ::SerializeTypeInfo<PropertyType>::GetUuid();
        }

        virtual PropertyType* CastTo(void* instance, const InstanceDataNode* node, const AZ::Uuid& fromId, const AZ::Uuid& toId) const
        {
            return static_cast<PropertyType*>(node->GetSerializeContext()->DownCast(instance, fromId, toId));
        }

        virtual void WriteGUIValuesIntoProperty_Internal(QWidget* widget, InstanceDataNode* node) override
        {
            // Can bail out here if there's no class metadata (e.g. if the node is a UIElement instead of a DataElement)
            if (!node->GetClassMetadata())
            {
                return;
            }

            WidgetType* wid = static_cast<WidgetType*>(widget);

            const AZ::Uuid& actualUUID = node->GetClassMetadata()->m_typeId;
            const AZ::Uuid& desiredUUID = GetHandledType();

            for (size_t idx = 0; idx < node->GetNumInstances(); ++idx)
            {
                void* instanceData = node->GetInstance(idx);

                PropertyType* actualCast = CastTo(instanceData, node, actualUUID, desiredUUID);
                AZ_Assert(actualCast, "Could not cast from the existing type ID to the actual typeid required by the editor.");
                WriteGUIValuesIntoProperty(idx, wid, *actualCast, node);
            }
        }

        virtual void WriteGUIValuesIntoTempProperty_Internal(QWidget* widget, void* tempValue, const AZ::Uuid& propertyType, AZ::SerializeContext* serializeContext) override
        {
            WidgetType* wid = static_cast<WidgetType*>(widget);

            const AZ::Uuid& desiredUUID = GetHandledType();

            PropertyType* actualCast = [&]() -> PropertyType*
            {
                if (serializeContext->CanDowncast(propertyType, desiredUUID))
                {
                    return static_cast<PropertyType*>(serializeContext->DownCast(tempValue, propertyType, desiredUUID));
                }
                // Cover the case of Asset<T> property handling
                else if (const auto* genericTypeInfo = serializeContext->FindGenericClassInfo(propertyType);
                            genericTypeInfo && genericTypeInfo->GetGenericTypeId() == desiredUUID)
                {
                    return reinterpret_cast<PropertyType*>(tempValue);
                }

                return nullptr;
            }();

            AZ_Assert(actualCast, "Could not cast from the existing type ID to the actual typeid required by the editor.");
            WriteGUIValuesIntoProperty(0, wid, *actualCast, nullptr);
        }

        virtual void ReadValuesIntoGUI_Internal(QWidget* widget, InstanceDataNode* node) override
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // Can bail out here if there's no class metadata (e.g. if the node is a UIElement instead of a DataElement)
            if (!node->GetClassMetadata())
            {
                return;
            }

            WidgetType* wid = static_cast<WidgetType*>(widget);

            const AZ::Uuid& actualUUID = node->GetClassMetadata()->m_typeId;
            const AZ::Uuid& desiredUUID = GetHandledType();

            AZ::SerializeContext* serializeContext{};
            if (auto componentApplicationRequests = AZ::Interface<AZ::ComponentApplicationRequests>::Get();
                componentApplicationRequests != nullptr)
            {
                serializeContext = componentApplicationRequests->GetSerializeContext();
            }

            if (serializeContext == nullptr)
            {
                return;
            }

            auto desiredClassData = serializeContext->FindClassData(desiredUUID);

            for (size_t idx = 0; idx < node->GetNumInstances(); ++idx)
            {
                void* instanceData = node->GetInstance(idx);

                PropertyType* actualCast = CastTo(instanceData, node, actualUUID, desiredUUID);

                // Check to see if the type supports an IDataConverter for Asset<T> types
                // in case CastTo fails
                if (actualCast == nullptr && desiredClassData != nullptr && desiredClassData->CanConvertFromType(actualUUID, *serializeContext))
                {
                    void* convertibleInstance{};
                    desiredClassData->ConvertFromType(convertibleInstance, actualUUID, instanceData, *serializeContext);
                    actualCast = reinterpret_cast<PropertyType*>(convertibleInstance);
                }
                AZ_Assert(actualCast, "Could not cast from the existing type ID to the actual typeid required by the editor.");
                if (actualCast == nullptr || !ReadValuesIntoGUI(idx, wid, *actualCast, node))
                {
                    return;
                }
            }
        }

        PropertyEditorToolsSystemInterface::PropertyHandlerId m_registeredDpeHandlerId = PropertyEditorToolsSystemInterface::InvalidHandlerId;
    };
}

#endif
