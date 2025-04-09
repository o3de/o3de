/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <QtWidgets/QWidget>
#endif

class QRadioButton;
class QButtonGroup;

namespace AzToolsFramework
{
    /*
    * Class which handles displaying a boolean value as two radio buttons in the UI
    * Text defaults to "False" and "True", and the "False" radio button is rendered first, and has first tab order
    * To change the default display text for "False" and "True",
    * use AZ::Edit::Attributes::FalseText or AZ::Edit::Attributes::TrueText repsectively
    */
    class PropertyBoolRadioButtonsCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyBoolRadioButtonsCtrl, AZ::SystemAllocator);

        explicit PropertyBoolRadioButtonsCtrl(QWidget* pParent = nullptr);
        ~PropertyBoolRadioButtonsCtrl() override = default;

        bool value() const;

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();
        void SetButtonText(bool value, const char* description);

    signals:
        void valueChanged(bool newValue);

    public slots:
        void setValue(bool val);

    protected slots:
        void onRadioButtonSelected(int value);

    private:
        QButtonGroup* m_buttonGroup;
        static const int s_trueButtonIndex;
        static const int s_falseButtonIndex;
    };

    /*
    * Property handler for use with the PropertyBoolRadioButtonsCtrl
    * To use this handler, create a reflected boolean variable and use handler name AZ::Edit::Handlers::RadioButton
    */
    class BoolPropertyRadioButtonsHandler
        : QObject
        , public PropertyHandler<bool, PropertyBoolRadioButtonsCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(BoolPropertyRadioButtonsHandler, AZ::SystemAllocator);
        ~BoolPropertyRadioButtonsHandler() override = default;

        AZ::u32 GetHandlerName() const override;
        QWidget* GetFirstInTabOrder(PropertyBoolRadioButtonsCtrl* widget) override;
        QWidget* GetLastInTabOrder(PropertyBoolRadioButtonsCtrl* widget) override;
        void UpdateWidgetInternalTabbing(PropertyBoolRadioButtonsCtrl* widget) override;

        QWidget* CreateGUI(QWidget* pParent) override;
        bool ResetGUIToDefaults(PropertyBoolRadioButtonsCtrl* GUI) override;
        void ConsumeAttribute(PropertyBoolRadioButtonsCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyBoolRadioButtonsCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyBoolRadioButtonsCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
    };

    void RegisterBoolRadioButtonsHandler();
};
