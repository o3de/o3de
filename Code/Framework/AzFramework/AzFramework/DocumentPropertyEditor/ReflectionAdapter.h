/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/RoutingAdapter.h>
#include <AzCore/std/string/string_view.h>

namespace AZ::DocumentPropertyEditor
{
    struct ReflectionAdapterReflectionImpl;

    namespace Nodes
    {
        enum class ValueChangeType;
    }

    //! ReflectionAdapter turns an in-memory instance of an object backed by
    //! the AZ Reflection system (via SerializeContext & EditContext) and creates
    //! a property grid that supports editing its members in a manner outlined by
    //! its reflection data.
    class ReflectionAdapter : public RoutingAdapter
    {
    public:
        //! Holds the parameters that define a specific property change event
        struct PropertyChangeInfo
        {
            const AZ::Dom::Path& path;
            const AZ::Dom::Value& newValue;
            Nodes::ValueChangeType changeType;
        };

        using PropertyChangeEvent = Event<const PropertyChangeInfo&>;

        //! Creates an uninitialized (empty) ReflectionAdapter.
        ReflectionAdapter();
        //! Creates a ReflectionAdapter with a contents comrpised of the reflected data of
        //! the specified instance.
        //! \see SetValue
        ReflectionAdapter(void* instance, AZ::TypeId typeId);
        ~ReflectionAdapter();

        //! Sets the instance to reflect. If typeId is a type registered to SerializeContext,
        //! this adapter will produce a property grid based on its contents.
        void SetValue(void* instance, AZ::TypeId typeId);

        //! Invokes the ChangeNotify attribute for an Adapter, if present, and follows up
        //! with a tree refresh if needed.
        static void InvokeChangeNotify(const AZ::Dom::Value& domNode);

        //! Connects a listener to the event fired when a property has been changed.
        //! This can be used to notify the view of changes to the DOM, e.g. notifying an editor of document changes.
        void ConnectPropertyChangeHandler(PropertyChangeEvent::Handler& handler);
        //! Call this to trigger a PropertyChangeEvent to notify the view that one of this adapter's
        //! property editor instances has altered its value.
        void NotifyPropertyChanged(const PropertyChangeInfo& changeInfo);

        //! Creates a node for displaying label information.
        //! Subclass adapter can override the default behavior.
        //! @param adapterBuilder The adapter builder currently being used by this adapter.
        //! @param labelText The text string to be displayed in label.
        //! @param serializedPath The serialized path fetched from AZ::Reflection::DescriptorAttributes.
        virtual void CreateLabel(AdapterBuilder* adapterBuilder, AZStd::string_view labelText, AZStd::string_view serializedPath);

        //! Updates the contents of adapter Dom contents using the property change information provided. Child adapters can override
        //! this function to make more changes to the Dom in addition to the property changes.
        //! @param propertyChangeInfo Object containing information about the property change.
        virtual void UpdateDomContents(const PropertyChangeInfo& propertyChangeInfo);

        void* GetInstance() { return m_instance; }
        const void* GetInstance() const { return m_instance; }
        AZ::TypeId GetTypeId() const { return m_typeId; }

        ExpanderSettings* CreateExpanderSettings(
            DocumentAdapter* referenceAdapter,
            const AZStd::string& settingsRegistryKey = AZStd::string(),
            const AZStd::string& propertyEditorName = AZStd::string()) override;

    protected:
        Dom::Value GenerateContents() override;
        Dom::Value HandleMessage(const AdapterMessage& message) override;

    private:
        void* m_instance = nullptr;
        AZ::TypeId m_typeId = AZ::TypeId::CreateNull();

        AZStd::unique_ptr<ReflectionAdapterReflectionImpl> m_impl;

        PropertyChangeEvent m_propertyChangeEvent;

        friend struct ReflectionAdapterReflectionImpl;
    };
}
