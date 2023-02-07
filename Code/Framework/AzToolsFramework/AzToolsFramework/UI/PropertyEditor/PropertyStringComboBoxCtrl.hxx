/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef UI_PROPERTYEDITOR_PROPERTYSTRINGCOMBOBOX_CTRL
#define UI_PROPERTYEDITOR_PROPERTYSTRINGCOMBOBOX_CTRL

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include "PropertyEditorAPI.h"
#include <UI/PropertyEditor/GenericComboBoxCtrl.h>

#include <QWidget>
#include <QToolButton>
#include <QComboBox>
#endif

namespace AzToolsFramework
{
    //! Type Alias for the Error String type that is used when an Edit is rejected
    //! Uses AZStd::fixed_string to avoid dynamic memory allocations
    using EditRejectString = AZStd::fixed_string<256>;

    //! Outcome that returns the value that the ComboBox control should set on Success
    //! On Failure a rejection string with the reason for failure is returned
    using EditResultOutcome = AZ::Outcome<AZStd::string, EditRejectString>;

    //! Callback to implement that determines the new value to update the Combo Box Ctrl with
    //! or to provide the Combo Box Ctrl system with an error message as to why the new value could not be set
    //! i.e "new value not convertible to type", "new value is out of range", etc...
    //! @param currentValue The current value stored in the combo box
    //! @return Outcome which should store the new value to set in the combo box control or a string supplying the reason why the edit has been rejected.
    using EditButtonCallback = AZStd::function<EditResultOutcome(AZStd::string currentValue)>;

    class PropertyStringComboBoxCtrl
        : public GenericComboBoxCtrl<AZStd::string>
    {
        friend class StringEnumPropertyComboBoxHandler;
        template<typename T>
        friend class PropertyComboBoxHandlerCommon;
        Q_OBJECT
        using ComboBoxBase = GenericComboBoxCtrl<AZStd::string>;
    public:
        AZ_RTTI(PropertyStringComboBoxCtrl, "{886E5B2C-46F5-4046-B0A3-89C28CB28B38}", ComboBoxBase);
        AZ_CLASS_ALLOCATOR(PropertyStringComboBoxCtrl, AZ::SystemAllocator, 0);

        PropertyStringComboBoxCtrl(QWidget* pParent = NULL);
        ~PropertyStringComboBoxCtrl() override;

        //AZStd::string value() const;

        void Add(const AZStd::string& value);
        void Add(const AZStd::vector<AZStd::string>& value);
        int GetCount();
        uint32_t GetCurrentIndex();

        void UpdateTabOrder();

    private:
        //AZStd::vector<AZStd::string> m_values;
    };

    template <class ValueType>
    class PropertyComboBoxHandlerCommon
        : public PropertyHandler<ValueType, PropertyStringComboBoxCtrl>
    {
        AZ::u32 GetHandlerName(void) const override  { return AZ::Edit::UIHandlers::ComboBox; }
        void UpdateWidgetInternalTabbing(PropertyStringComboBoxCtrl* widget) override { widget->UpdateTabOrder(); }
        void ConsumeAttribute(PropertyStringComboBoxCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
    };

    class StringEnumPropertyComboBoxHandler
        : QObject
        , public PropertyComboBoxHandlerCommon < AZStd::string >
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(StringEnumPropertyComboBoxHandler, AZ::SystemAllocator, 0);

        void WriteGUIValuesIntoProperty(size_t index, PropertyStringComboBoxCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyStringComboBoxCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
        QWidget* CreateGUI(QWidget* pParent) override;
    };

    void RegisterStringComboBoxHandler();
};

#endif // UI_PROPERTYEDITOR_PROPERTYSTRINGCOMBOBOX_CTRL
