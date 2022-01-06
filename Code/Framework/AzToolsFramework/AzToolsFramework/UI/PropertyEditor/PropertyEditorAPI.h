/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Debug/Budget.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentBus.h>
#include "PropertyEditorAPI_Internals.h"

class QWidget;
class QCheckBox;
class QLabel;

namespace AzToolsFramework
{
    class InstanceDataNode;

    // when a property is modified, we attempt to retrieve the value that comes out in response to the Property Modification function that you may supply
    // if you return anything other than Refresh_None, the tree may be queued for update:
    enum PropertyModificationRefreshLevel : int
    {
        Refresh_None,
        Refresh_Values,
        Refresh_AttributesAndValues,
        Refresh_EntireTree,
        Refresh_EntireTree_NewContent,
    };

    // only ONE property handler is ever created for each kind of property.
    // so do not store state for a particular GUI, inside your property handler.  Your one handler may be responsible for translating
    // many values for many guis.  However, we don't force these functions to be const, because you ARE allowed to keep maps of your widgets
    // or pool them and such.  Just be aware that your one handler might be responsible for many guis in many panels in many windows, so
    // it should be made as "functional" as possible...
    template <typename PropertyType, class WidgetType>
    class PropertyHandler
        : public TypedPropertyHandler_Internal<PropertyType, WidgetType>
    {
    public:
        // WriteGUIValuesIntoProperty:  This will be called on each instance of your property type.  So for example if you have an object
        // selected, each of which has the same float property on them, and your property editor is for floats, you will get this
        // write and read function called 5x - once for each instance.
        // this is your opportunity to determine if the values are the same or different.
        // index is the index of the instance, from 0 to however many there are.  You can use this to determine if its
        // the first instance, or to check for multi-value edits if it matters to you.
        // GUI is a pointer to the GUI used to editor your property (the one you created in CreateGUI)
        // and instance is a the actual value (PropertyType).
        // you may not cache the pointer to anything.
        void WriteGUIValuesIntoProperty(size_t index, WidgetType* GUI, PropertyType& instance, InstanceDataNode* node) override = 0;

        // this will get called in order to initialize your gui.  It will be called once for each instance.
        // for example if you have multiple objects selected, index will go from 0 to however many there are.
        // you may not cache the pointer to anything.
        bool ReadValuesIntoGUI(size_t index, WidgetType* GUI, const PropertyType& instance, InstanceDataNode* node) override = 0;

        // this will be called in order to initialize or refresh your gui.  Your class will be fed one attribute at a time
        // you may override this to interpret the attributes as you wish - use attrValue->Read<int>() for example, to interpret it as an int.
        // all attributes can either be a flat value, or a function which returns that same type.  In the case of the function
        // it will be called on the first instance.
        // note that this can be called at any time later, again, after your GUI is initialized, if someone invalidates
        // the attributes and is requesting a refresh.
        // Example
        // if (attrib = AZ::Crc("Max")
        // {
        //     int maxValue = 0;
        //     if (attrValue->Read<int>(maxValue)) GUI->SetMax(maxValue);
        // }
        // you may not cache the pointer to anything.
        //virtual void ConsumeAttribute(WidgetType* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;

        // override GetFirstInTabOrder, GetLastInTabOrder in your base class to define which widget gets focus first when pressing tab,
        // and also what widget is last.
        // for example, if your widget is a compound widget and contains, say, 5 buttons
        // then for GetFirstInTabOrder, return the first button
        // and for GetLastInTabOrder, return the last button.
        // and in UpdateWidgetInternalTabbing, call setTabOrder(button0, button1); setTabOrder(button1, button2)...
        // this will cause the previous row to tab to your first button, when the user hits tab
        // and cause the next row to tab to your last button, when the user hits shift+tab on the next row.
        // if your widget is a single widget or has a single focus proxy there is no need to override.
        // you may not cache the pointer to anything.
        QWidget* GetFirstInTabOrder(WidgetType* widget) override { return widget; }
        QWidget* GetLastInTabOrder(WidgetType* widget) override { return widget; }

        // implement this function in order to set your internal tab order between child controls.
        // just call a series of QWidget::setTabOrder
        // you may not cache the pointer to anything.
        void UpdateWidgetInternalTabbing(WidgetType* /*widget*/) override {}

        // you must implement CreateGUI:
        // create an instance of the GUI that is used to edit this property type.
        // the QWidget pointer you return also serves as a handle for accessing data.  This means that in order to trigger
        // a write, you need to call RequestWrite(...) on that same widget handle you return.
        QWidget* CreateGUI(QWidget *pParent) override = 0;

        // you MAY override this if you wish to pool your widgets or reuse them.  The default implementation simply calls delete.
        //virtual QWidget* DestroyGUI(QWidget* object) override;
    };

    // A GenericPropertyHandler may be used to register a widget for a property handler ID that is always used, regardless of the underlying type
    // This is useful for UI elements that don't have any specific underlying storage, like buttons
    template <class WidgetType>
    class GenericPropertyHandler
        : public PropertyHandler_Internal<WidgetType>
    {
    public:
        virtual void WriteGUIValuesIntoProperty(size_t index, WidgetType* GUI, void* value, const AZ::Uuid& propertyType)
        {
            (void)index;
            (void)GUI;
            (void)value;
            (void)propertyType;
        }

        virtual bool ReadValueIntoGUI(size_t index, WidgetType* GUI, void* value, const AZ::Uuid& propertyType)
        {
            (void)index;
            (void)GUI;
            (void)value;
            (void)propertyType;
            return false;
        }

        QWidget* GetFirstInTabOrder(WidgetType* widget) override { return widget; }
        QWidget* GetLastInTabOrder(WidgetType* widget) override { return widget; }
        void UpdateWidgetInternalTabbing(WidgetType* /*widget*/) override {}

        QWidget* CreateGUI(QWidget *pParent) override = 0;
    protected:
        bool HandlesType(const AZ::Uuid& id) const override
        {
            (void)id;
            return true;
        }

        const AZ::Uuid& GetHandledType() const override
        {
            return nullUuid;
        }

        void WriteGUIValuesIntoProperty_Internal(QWidget* widget, InstanceDataNode* node) override
        {
            for (size_t i = 0; i < node->GetNumInstances(); ++i)
            {
                WriteGUIValuesIntoProperty(i, reinterpret_cast<WidgetType*>(widget), node->GetInstance(i), node->GetClassMetadata()->m_typeId);
            }
        }

        void WriteGUIValuesIntoTempProperty_Internal(QWidget* widget, void* tempValue, const AZ::Uuid& propertyType, AZ::SerializeContext* serializeContext) override
        {
            (void)serializeContext;
            WriteGUIValuesIntoProperty(0, reinterpret_cast<WidgetType*>(widget), tempValue, propertyType);
        }

        void ReadValuesIntoGUI_Internal(QWidget* widget, InstanceDataNode* node) override
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            for (size_t i = 0; i < node->GetNumInstances(); ++i)
            {
                if (!ReadValueIntoGUI(i, reinterpret_cast<WidgetType*>(widget), node->GetInstance(i), node->GetClassMetadata()->m_typeId))
                {
                    break;
                }
            }
        }

        // Needed since GetHandledType returns a reference
        AZ::Uuid nullUuid = AZ::Uuid::CreateNull();
    };

    // your components talk to the property manager in this way:
    class PropertyTypeRegistrationMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // there's only one address to this bus, its always broadcst
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        using Bus = AZ::EBus<PropertyTypeRegistrationMessages>;

        virtual ~PropertyTypeRegistrationMessages() {}

        // call this to register and unregister your handlers.
        // note that the property manager exposes the "PropertyManager" dependent service, so any component which registers a property type should specify
        // "PropertyManager" as one of the services it depends on
        // like this, which will cause the property manager to be ready first:
        // virtual void GetRequiredServices(DependencyArrayType& required) const
        //{
        //      required.push_back(AZ_CRC("PropertyManagerService"));
        //}

        virtual void RegisterPropertyType(PropertyHandlerBase* pHandler) = 0;
        virtual void UnregisterPropertyType(PropertyHandlerBase* pHandler) = 0;

        // you probably don't need to use this, but you can.  Given a name and type it will return the property handler responsible were that type and name
        virtual PropertyHandlerBase* ResolvePropertyHandler(AZ::u32 handlerName, const AZ::Uuid& handlerType) = 0;
    };

    // You can also talk to the property editor GUI itself, using your widget as the key to control which one you're talking to.
    // the reason you need to feed your widget in as the key is that there is likely many property editor guis
    // and multiple copies of your widget, in any of them.
    class PropertyEditorGUIMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // there's only one address to this bus, its always broadcast
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // each Property Editor GUI connects to it.
        //////////////////////////////////////////////////////////////////////////

        using Bus = AZ::EBus<PropertyEditorGUIMessages>;

        virtual ~PropertyEditorGUIMessages() {}

        // the GUI can call this to request that WriteGUIValuesIntoProperty is iterated on its handler.
        // Alternatively, the handler (if its a QT object), can call this too, connecting the value changed
        // messages in the GUI.
        // calling this will result in WriteGUIValuesIntoProperty() being called on your handler
        // and undo stack capture to occur.
        virtual void RequestWrite(QWidget* editorGUI) = 0;
        virtual void RequestRefresh(PropertyModificationRefreshLevel) = 0;

        virtual void AddElementsToParentContainer(QWidget* editorGUI, size_t numElements, const InstanceDataNode::FillDataClassCallback& fillDataCallback) = 0;

        // Invokes a Property Notification without writing modifying the property
        virtual void RequestPropertyNotify(QWidget* editorGUI) = 0;

        // Invoked by widgets to notify the property editor that the editing session has finished
        // This can be used to end an undo batch operation
        virtual void OnEditingFinished(QWidget* editorGUI) = 0;

    };

    /**
     * Events/bus for listening externally for property changes on a specific entity.
     */
    class PropertyEditorEntityChangeNotifications
        : public AZ::ComponentBus
    {
    public:
        /// Fired when property data was changed for the entity.
        /// \param componentId - Id of the component on which property data was changed.
        virtual void OnEntityComponentPropertyChanged(AZ::ComponentId /*componentId*/) {}
    };

    using PropertyEditorEntityChangeNotificationBus = AZ::EBus<PropertyEditorEntityChangeNotifications>;

    /**
     * Event bus for notifying property changes on a specific component.
     */
    class PropertyEditorChangeNotifications
        : public AZ::EBusTraits
    {
    public:
        /// Fired when property data is changed on a component.
        /// The event is only fired once per component change, even with a multiple selection.
        /// \param componentType - The type of component which was modified.
        virtual void OnComponentPropertyChanged(AZ::Uuid /*componentType*/) {}
    };

    /// Type to inherit to implement PropertyEditorChangeNotifications
    using PropertyEditorChangeNotificationBus = AZ::EBus<PropertyEditorChangeNotifications>;

    /**
     * Describes a field/node's visibility with editor UIs, for consistency across tools
     * and controls.
     */
    enum class NodeDisplayVisibility
    {
        NotVisible,
        Visible,
        ShowChildrenOnly,
        HideChildren
    };

    /**
     * Resolve the \ref AZ::Edit::PropertyVisibility attribute for a given data node.
     * \param node - instance data hierarchy node
     * \return ref AZ::Edit::PropertyVisibility value
     */
    AZ::Crc32 ResolveVisibilityAttribute(const InstanceDataNode& node);

    /**
     * Used by in-editor tools to determine if a given field has any visible children.
     * Calls CalculateNodeDisplayVisibility() on all child nodes of the input node.
     * \param node instance data hierarchy node for which visibility should be calculated.
     * \param isSlicePushUI (optional - false by default) if enabled, additional push-only visibility options are applied.
     * \return bool
     */
    bool HasAnyVisibleChildren(const InstanceDataNode& node, bool isSlicePushUI = false);

    /**
     * Used by in-editor tools to determine if a given field should be visible.
     * This aggregates everything required to make the determination, including
     * editor reflection, bound "Visibility" attributes, etc.
     * \param node instance data hierarchy node for which visibility should be calculated.
     * \param isSlicePushUI (optional - false by default) if enabled, additional push-only visibility options are applied.
     * \return ref NodeDisplayVisibility
     */
    NodeDisplayVisibility CalculateNodeDisplayVisibility(const InstanceDataNode& node, bool isSlicePushUI = false);

    /**
     * Used by in-editor tools to determine if a node matches the passed in filter
    */
    bool NodeMatchesFilter(const InstanceDataNode& node, const char* filter);

    /**
     * Used by in-editor tools to determine if the parent of a node matches the passed in filter
    */
    bool NodeGroupMatchesFilter(const InstanceDataNode& node, const char* filter);

    /**
     * Used by in-editor tools to read the visibility attribute on a given instance
    */
    bool ReadVisibilityAttribute(void* instance, AZ::Edit::Attribute* attr, AZ::Crc32& visibility);

    /**
     * Determines the display name tools should use for a given property node.
     * \param node - instance data hierarchy node for which display name should be determined.
     */
    AZStd::string GetNodeDisplayName(const InstanceDataNode& node);

    /**
     * Wrapper for OnEntityComponentPropertyChanged EBus call.
     */
    void OnEntityComponentPropertyChanged(const AZ::EntityComponentIdPair& entityComponentIdPair);

    /**
     * Wrapper for OnEntityComponentPropertyChanged EBus call (overload).
     */
    void OnEntityComponentPropertyChanged(AZ::EntityId entityId, AZ::ComponentId componentId);

    /**
     * A function that evaluates whether a property node is read-only.
     * This can be used to make a property read-only when that can't be
     * accomplished through attributes on the node.
     */
    using ReadOnlyQueryFunction = AZStd::function<bool(const InstanceDataNode*)>;

    /**
     * A function that evaluates whether a property node is hidden.
     * This can be used to make a property hidden when that can't be
     * accomplished through attributes on the node.
     */
    using HiddenQueryFunction = AZStd::function<bool(const InstanceDataNode*)>;

    /**
     * A function that evaluates whether a property node should display an indicator
     * and if so, which indicator.  Return nullptr if you don't want an indicator to show
     */
    using IndicatorQueryFunction = AZStd::function<const char*(const InstanceDataNode*)>;

} // namespace AzToolsFramework

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzToolsFramework::PropertyModificationRefreshLevel, "{06F58AEC-352A-4761-9040-FD5FCEC4D314}")
} // namespace AZ

#include "PropertyEditorAPI_Internals_Impl.h"

