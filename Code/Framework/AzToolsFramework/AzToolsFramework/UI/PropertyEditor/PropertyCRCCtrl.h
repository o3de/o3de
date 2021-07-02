/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <QtWidgets/QWidget>

#include "PropertyEditorAPI.h"
#endif

class QLineEdit;
class QString;

namespace AzToolsFramework
{
    // a control for storing a CRC-like u32 hex value and displaying it as hex.
    class PropertyCRCCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyCRCCtrl, AZ::SystemAllocator, 0);

        PropertyCRCCtrl(QWidget* pParent = NULL);
        virtual ~PropertyCRCCtrl();

        AZ::u32 value() const;

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();
        bool eventFilter(QObject* target, QEvent* event) override;

    Q_SIGNALS:
        void valueChanged(AZ::u32 newValue);

    public Q_SLOTS:
        void setValue(AZ::u32 val);
        void onLineEditChange(QString newText);

    private:
        QLineEdit* m_lineEdit;
        AZ::u32 m_currentValue = 0;

        void UpdateValueText();
        AZ::u32 TextToValue();

    protected:
        void FocusChangedEdit(bool hasFocus);
        
    };

    // note:  QT Objects cannot themselves be templates, else I would templatize creategui as well.
    class U32CRCHandler  
        : public QObject
        , public PropertyHandler<AZ::u32, PropertyCRCCtrl>
    {
    Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(U32CRCHandler, AZ::SystemAllocator, 0);

        AZ::u32 GetHandlerName(void) const override;
        QWidget* GetFirstInTabOrder(PropertyCRCCtrl* widget) override;
        QWidget* GetLastInTabOrder(PropertyCRCCtrl* widget) override;
        void UpdateWidgetInternalTabbing(PropertyCRCCtrl* widget) override;

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(PropertyCRCCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyCRCCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyCRCCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
    };

    void RegisterCrcHandler();
};
