/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include "PropertyEditorAPI.h"
#include <UI/PropertyEditor/GenericComboBoxCtrl.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <QWidget>
#include <QToolButton>
#include <QComboBox>
#endif

namespace AzToolsFramework
{
    class AZTF_API PropertyStringComboBoxCtrl
        : public GenericComboBoxCtrl<AZStd::string>
    {
        Q_OBJECT
        using ComboBoxBase = GenericComboBoxCtrl<AZStd::string>;

        friend class StringEnumPropertyComboBoxHandler;
        template<typename T>
        friend class PropertyComboBoxHandlerCommon;

    public:
        AZ_RTTI(PropertyStringComboBoxCtrl, "{886E5B2C-46F5-4046-B0A3-89C28CB28B38}", ComboBoxBase);
        AZ_CLASS_ALLOCATOR(PropertyStringComboBoxCtrl, AZ::SystemAllocator);

        PropertyStringComboBoxCtrl(QWidget* pParent = NULL);
        ~PropertyStringComboBoxCtrl() override;

        void Add(const AZStd::string& value);
        void Add(const AZStd::vector<AZStd::string>& value);
        int GetCount();
        uint32_t GetCurrentIndex();

        void UpdateTabOrder() override;
    };

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4661) // no suitable definition provided for explicit template instantiation request
#endif

    template <class ValueType>
    class PropertyComboBoxHandlerCommon
        : public PropertyHandler<ValueType, PropertyStringComboBoxCtrl>
    {
        AZ::u32 GetHandlerName(void) const override  { return AZ::Edit::UIHandlers::ComboBox; }
        void UpdateWidgetInternalTabbing(PropertyStringComboBoxCtrl* widget) override { widget->UpdateTabOrder(); }
        void ConsumeAttribute(PropertyStringComboBoxCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override
        {
            if (attrib == AZ::Edit::Attributes::StringList)
            {
                AZStd::vector<AZStd::string> value;
                if (attrValue->Read<AZStd::vector<AZStd::string> >(value))
                {
                    GUI->Add(value);
                }
                else
                {
                    (void)debugName;
                    AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'StringList' attribute from property '%s' into string combo box. Expected string vector.", debugName);
                }
            }
            else if (attrib == AZ::Edit::Attributes::ComboBoxEditable)
            {
                bool value;
                if (attrValue->Read<bool>(value))
                {
                    GUI->GetComboBox()->setEditable(value);
                }
                else
                {
                    // emit a warning!
                    AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'EditableCombBox' attribute from property '%s' into string combo box", debugName);
                }
                return;
            }
            else if (attrib == AZ_CRC_CE("EditButtonVisible"))
            {
                bool visible;
                if (attrValue->Read<bool>(visible))
                {
                    GUI->GetEditButton()->setVisible(visible);
                }
            }
            else if (attrib == AZ_CRC_CE("EditButtonCallback"))
            {
                if (auto* editButtonInvokable = azrtti_cast<AZ::AttributeInvocable<GenericEditResultOutcome<AZStd::string>(AZStd::string)>*>(attrValue->GetAttribute()))
                {
                    GUI->SetEditButtonCallBack(editButtonInvokable->GetCallable());
                };
            }
            else if (attrib == AZ_CRC_CE("EditButtonToolTip"))
            {
                AZStd::string toolTip;
                if (attrValue->Read<AZStd::string>(toolTip))
                {
                    GUI->GetEditButton()->setToolTip(toolTip.c_str());
                }
            }
        }
    };

    class AZTF_API StringEnumPropertyComboBoxHandler
        : QObject
        , public PropertyComboBoxHandlerCommon < AZStd::string >
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(StringEnumPropertyComboBoxHandler, AZ::SystemAllocator);

        void WriteGUIValuesIntoProperty(size_t index, PropertyStringComboBoxCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyStringComboBoxCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
        QWidget* CreateGUI(QWidget* pParent) override;
    };

    AZTF_API void RegisterStringComboBoxHandler();

#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
