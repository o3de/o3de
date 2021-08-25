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
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>

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


    // this serves as the base class for all property managers.
    // you do not use this class directly.
    // derive from PropertyHandler instead.
    class PropertyHandlerBase
    {
        friend class ReflectedPropertyEditor;
        friend PropertyRowWidget;
        friend class Components::PropertyManagerComponent;

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

        virtual void WriteGUIValuesIntoProperty_Internal(QWidget* widget, InstanceDataNode* node) override
        {
            WidgetType* wid = static_cast<WidgetType*>(widget);

            const AZ::Uuid& actualUUID = node->GetClassMetadata()->m_typeId;
            const AZ::Uuid& desiredUUID = GetHandledType();

            for (size_t idx = 0; idx < node->GetNumInstances(); ++idx)
            {
                void* instanceData = node->GetInstance(idx);

                PropertyType* actualCast = static_cast<PropertyType*>(node->GetSerializeContext()->DownCast(instanceData, actualUUID, desiredUUID));
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

                PropertyType* actualCast = static_cast<PropertyType*>(node->GetSerializeContext()->DownCast(instanceData, actualUUID, desiredUUID));
                AZ_Assert(actualCast, "Could not cast from the existing type ID to the actual typeid required by the editor.");
                if (!ReadValuesIntoGUI(idx, wid, *actualCast, node))
                {
                    return;
                }
            }
        }
    };
}

#endif
