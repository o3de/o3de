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
#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyEditorToolsSystemInterface.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzCore/Asset/AssetSerializer.h>

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

        virtual void OnValueChanged(AZ::DocumentPropertyEditor::Nodes::PropertyEditor::ValueChangeType changeType) = 0;
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

        // you need to define this.
        virtual AZ::u32 GetHandlerName() const = 0;  // AZ_CRC("IntSlider")

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
        virtual void ConsumeAttributes_Internal(QWidget* widget, InstanceDataNode* attrValue) = 0;
        virtual void WriteGUIValuesIntoProperty_Internal(QWidget* widget, InstanceDataNode* t) = 0;
        virtual void WriteGUIValuesIntoTempProperty_Internal(QWidget* widget, void* tempValue, const AZ::Uuid& propertyType, AZ::SerializeContext* serializeContext) = 0;
        virtual void ReadValuesIntoGUI_Internal(QWidget* widget, InstanceDataNode* t) = 0;
        // we define this automatically for you, you don't have to override it.
        virtual bool HandlesType(const AZ::Uuid& id) const = 0;
        virtual const AZ::Uuid& GetHandledType() const = 0;
        virtual QWidget* GetFirstInTabOrder_Internal(QWidget* widget) = 0;
        virtual QWidget* GetLastInTabOrder_Internal(QWidget* widget) = 0;
        virtual void UpdateWidgetInternalTabbing_Internal(QWidget* widget) = 0;
    };

    // Wrapper type that takes a PropertyHandleBase from the ReflectedPropertyEditor and
    // provides a PropertyHandlerWidgetInterface for the DocumentPropertyEditor.
    // Doesn't use the normal static ShouldHandleNode and GetHandlerName implementations,
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
                delete m_widget;
            }
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
                AZ::Crc32 attributeId = AZ::Crc32(name.GetStringView());

                const AZ::DocumentPropertyEditor::AttributeDefinitionInterface* attribute =
                    propertyEditorSystem->FindNodeAttribute(name, propertyEditorSystem->FindNode(AZ::Name(GetHandlerName(m_rpeHandler))));
                AZStd::shared_ptr<AZ::Attribute> marshalledAttribute;
                if (attribute != nullptr)
                {
                    marshalledAttribute = attribute->DomValueToLegacyAttribute(attributeIt->second);
                }
                else
                {
                    marshalledAttribute = AZ::Reflection::WriteDomValueToGenericAttribute(attributeIt->second);
                }

                if (marshalledAttribute)
                {
                    m_proxyClassElement.m_attributes.emplace_back(attributeId, AZStd::move(marshalledAttribute));
                }
            }

            AZ::Dom::Value value = AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Value.ExtractFromDomNode(node).value();
            m_proxyValue = AZ::Dom::Utils::ValueToType<WrappedType>(value).value();
            m_rpeHandler.ReadValuesIntoGUI_Internal(GetWidget(), &m_proxyNode);
            m_rpeHandler.ConsumeAttributes_Internal(GetWidget(), &m_proxyNode);

            m_domNode = node;
        }

        QWidget* GetFirstInTabOrder() override
        {
            return m_rpeHandler.GetFirstInTabOrder_Internal(GetWidget());
        }

        QWidget* GetLastInTabOrder() override
        {
            return m_rpeHandler.GetLastInTabOrder_Internal(GetWidget());
        }

        static bool ShouldHandleNode(PropertyHandlerBase& rpeHandler, const AZ::Dom::Value& node)
        {
            using AZ::DocumentPropertyEditor::Nodes::PropertyEditor;
            auto typeId = PropertyEditor::ValueType.ExtractFromDomNode(node);
            if (!typeId.has_value())
            {
                AZ::Dom::Value value = PropertyEditor::Value.ExtractFromDomNode(node).value_or(AZ::Dom::Value());
                typeId = &AZ::Dom::Utils::GetValueTypeId(value);
            }
            return rpeHandler.HandlesType(*typeId.value());
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

        void OnValueChanged(AZ::DocumentPropertyEditor::Nodes::PropertyEditor::ValueChangeType changeType) override
        {
            using AZ::DocumentPropertyEditor::Nodes::PropertyEditor;

            m_rpeHandler.WriteGUIValuesIntoProperty_Internal(GetWidget(), &m_proxyNode);
            const AZ::Dom::Value newValue = AZ::Dom::Utils::ValueFromType(m_proxyValue);
            PropertyEditor::OnChanged.InvokeOnDomNode(m_domNode, newValue, changeType);
        }

    private:
        PropertyHandlerBase& m_rpeHandler;
        AZ::Dom::Value m_domNode;
        QPointer<QWidget> m_widget;
        InstanceDataNode m_proxyNode;
        AZ::SerializeContext::ClassData m_proxyClassData;
        AZ::SerializeContext::ClassElement m_proxyClassElement;
        WrappedType m_proxyValue;
    };

    template <class WidgetType>
    class PropertyHandler_Internal
        : public PropertyHandlerBase
    {
    public:
        typedef WidgetType widget_t;

        // this will be called in order to initialize your gui.  Your class will be fed one attribute at a time
        // you can interpret the attributes as you wish - use attrValue->Read<int>() for example, to interpret it as an int.
        // all attributes can either be a flat value, or a function which returns that same type.  In the case of the function
        // it will be called on the first instance.
        virtual void ConsumeAttribute(WidgetType* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
        {
            (void)widget;
            (void)attrib;
            (void)attrValue;
            (void)debugName;
        }

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
                registrationInfo.m_shouldHandleNode = [this](const AZ::Dom::Value& node)
                {
                    return HandlerType::ShouldHandleNode(*this, node);
                };
                registrationInfo.m_factory = [this]()
                {
                    return AZStd::make_unique<HandlerType>(*this);
                };
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

        virtual const AZ::Uuid& GetHandledType() const override
        {
            return AZ::SerializeTypeInfo<PropertyType>::GetUuid();
        }

        virtual PropertyType* CastTo(void* instance, const InstanceDataNode* node, const AZ::Uuid& fromId, const AZ::Uuid& toId) const
        {
            return static_cast<PropertyType*>(node->GetSerializeContext()->DownCast(instance, fromId, toId));
        }

        virtual void WriteGUIValuesIntoProperty_Internal(QWidget* widget, InstanceDataNode* node) override
        {
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

            PropertyType* actualCast = static_cast<PropertyType*>(serializeContext->DownCast(tempValue, propertyType, desiredUUID));
            AZ_Assert(actualCast, "Could not cast from the existing type ID to the actual typeid required by the editor.");
            WriteGUIValuesIntoProperty(0, wid, *actualCast, nullptr);
        }

        virtual void ReadValuesIntoGUI_Internal(QWidget* widget, InstanceDataNode* node) override
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            WidgetType* wid = static_cast<WidgetType*>(widget);

            const AZ::Uuid& actualUUID = node->GetClassMetadata()->m_typeId;
            const AZ::Uuid& desiredUUID = GetHandledType();

            for (size_t idx = 0; idx < node->GetNumInstances(); ++idx)
            {
                void* instanceData = node->GetInstance(idx);

                PropertyType* actualCast = CastTo(instanceData, node, actualUUID, desiredUUID);
                AZ_Assert(actualCast, "Could not cast from the existing type ID to the actual typeid required by the editor.");
                if (!ReadValuesIntoGUI(idx, wid, *actualCast, node))
                {
                    return;
                }
            }
        }

        PropertyEditorToolsSystemInterface::PropertyHandlerId m_registeredDpeHandlerId = PropertyEditorToolsSystemInterface::InvalidHandlerId;
    };
}

#endif
