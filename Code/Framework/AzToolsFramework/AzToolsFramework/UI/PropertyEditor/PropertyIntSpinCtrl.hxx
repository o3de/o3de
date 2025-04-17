/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyIntCtrlCommon.h>
#endif

namespace AzQtComponents
{
    class SpinBox;
}

namespace AzToolsFramework
{   
    class PropertyIntSpinCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyIntSpinCtrl, AZ::SystemAllocator);

        PropertyIntSpinCtrl(QWidget* parent = NULL);
        virtual ~PropertyIntSpinCtrl();

    public slots:
        void setValue(AZ::s64 val);
        void setMinimum(AZ::s64 val);
        void setMaximum(AZ::s64 val);
        void setStep(AZ::s64 val);
        void setMultiplier(AZ::s64 val);
        void setPrefix(QString val);
        void setSuffix(QString val);

        AZ::s64 value() const;
        AZ::s64 minimum() const;
        AZ::s64 maximum() const;
        AZ::s64 step() const;

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

    protected slots:
        void onChildSpinboxValueChange(int value);
    signals:
        void valueChanged(AZ::s64 newValue);
        void editingFinished();

    private:
        AzQtComponents::SpinBox* m_pSpinBox;
        AZ::s64 m_multiplier;

    protected:
        virtual void focusInEvent(QFocusEvent* e);
    };

    // Base class to allow QObject inheritance and definitions for IntSpinBoxHandlerCommon class template
    class IntSpinBoxHandlerQObject
        : public QObject
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    };

    template <class ValueType>
    class IntSpinBoxHandler
        : public IntWidgetHandler<ValueType, PropertyIntSpinCtrl, IntSpinBoxHandlerQObject>
    {
        using BaseHandler = IntWidgetHandler<ValueType, PropertyIntSpinCtrl, IntSpinBoxHandlerQObject>;
    public:
        AZ_CLASS_ALLOCATOR(IntSpinBoxHandler, AZ::SystemAllocator);
    protected:
        bool IsDefaultHandler() const override;
        AZ::u32 GetHandlerName(void) const override;
        QWidget* CreateGUI(QWidget* pParent) override;
    };

    template <class ValueType>
    bool IntSpinBoxHandler<ValueType>::IsDefaultHandler() const
    {
        return true;
    }

    template <class ValueType>
    AZ::u32 IntSpinBoxHandler<ValueType>::GetHandlerName(void) const
    {
        return AZ::Edit::UIHandlers::SpinBox;
    }

    template <class ValueType>
    QWidget* IntSpinBoxHandler<ValueType>::CreateGUI(QWidget* parent)
    {
        PropertyIntSpinCtrl* newCtrl = static_cast<PropertyIntSpinCtrl*>(BaseHandler::CreateGUI(parent));
        return newCtrl;
    }

    void RegisterIntSpinBoxHandlers();
};
