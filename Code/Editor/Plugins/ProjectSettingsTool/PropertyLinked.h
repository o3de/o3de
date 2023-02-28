/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "PropertyFuncValLineEdit.h"

#include <AzCore/std/containers/unordered_map.h>
#endif

// Forward declares
class QPushButton;

namespace ProjectSettingsTool
{
    // Forward declare
    class ValidationHandler;

    class PropertyLinkedCtrl
        : public PropertyFuncValLineEditCtrl
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(PropertyLinkedCtrl, AZ::SystemAllocator)
        typedef QString(* FileSelectFuncType)(const QString&);

        PropertyLinkedCtrl(QWidget* pParent = nullptr);

        // Set property this will mirror its values to
        void SetLinkedProperty(PropertyLinkedCtrl* property);
        // Set the tooltip on the link button so the user can see what property this is linked to
        void SetLinkTooltip(const QString& tip);
        // Returns true if all linked properties are equal
        bool AllLinkedPropertiesEqual();
        // Returns true if links are optional on this
        bool LinkIsOptional();
        // Set the option link state to given bool
        void SetOptionalLink(bool linked);
        // Tries to mirror the value to all linked properties
        void MirrorToLinkedProperty();
        // Enabled/disables link regardless of type
        void SetLinkEnabled(bool enabled);

        void ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

    protected:
        void MakeLinkButton();
        void MirrorToLinkedPropertyRecursive(PropertyLinkedCtrl* caller, const QString& value);
        // Tries to mirror the link button state to all linked properties
        void MirrorLinkButtonState(bool checked);
        void MirrorLinkButtonStateRecursive(PropertyLinkedCtrl* caller, bool state);
        // Returns true if all linked properties are equal
        bool AllLinkedPropertiesEqual(PropertyLinkedCtrl* caller, const QString& value);

        QPushButton* m_linkButton;
        PropertyLinkedCtrl* m_linkedProperty;
        bool m_linkEnabled;
    };

    class PropertyLinkedHandler
        : public AzToolsFramework::PropertyHandler<AZStd::string, PropertyLinkedCtrl>
    {
        AZ_CLASS_ALLOCATOR(PropertyLinkedHandler, AZ::SystemAllocator);

    public:
        PropertyLinkedHandler(ValidationHandler* valHdlr);

        AZ::u32 GetHandlerName(void) const override;
        // Need to unregister ourselves
        bool AutoDelete() const override { return false; }

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(PropertyLinkedCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyLinkedCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyLinkedCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;
        void LinkAllProperties();
        void EnableOptionalLinksIfAllPropertiesEqual();
        void MirrorAllLinkedProperties();
        void DisableAllPropertyLinks();
        void EnableAllPropertyLinks();
        static PropertyLinkedHandler* Register(ValidationHandler* valHdlr);

    private:
        struct IdentAndLink
        {
            AZStd::string identifier;
            AZStd::string linkedIdentifier;
        };
        // Map of identifiers to controls
        AZStd::unordered_map<AZStd::string, PropertyLinkedCtrl*> m_identToCtrl;

        // Map of controls to identifiers and their linked control's identifiers
        AZStd::unordered_map<PropertyLinkedCtrl*, IdentAndLink> m_ctrlToIdentAndLink;
        // Keeps track of the order ctrls were initialized
        AZStd::vector<PropertyLinkedCtrl*> m_ctrlInitOrder;
        // Tracks all validating properties
        ValidationHandler* m_validationHandler;
    };
} // namespace ProjectSettingsTool
