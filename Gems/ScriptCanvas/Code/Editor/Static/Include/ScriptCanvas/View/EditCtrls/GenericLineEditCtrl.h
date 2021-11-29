/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <QtWidgets/QWidget>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <ScriptCanvas/Core/Attributes.h>

#include <QValidator>
#endif

class QLineEdit;

namespace ScriptCanvasEditor
{
    namespace EditCtrl
    {
        template<typename T> using PropertyToStringCB = AZStd::function<bool(AZStd::string&, const T&)>;
        template<typename T> using StringToPropertyCB = AZStd::function<bool(T&, AZStd::string_view)>;
        using StringValidatorCB = AZStd::function<QValidator::State(QString&, int&)>;
    }

    template<typename T>
    class GenericLineEditHandler;

    class GenericLineEditCtrlBase
        : public QWidget
    {
        Q_OBJECT
    public:
        template<typename T>
        friend class GenericLineEditHandler;
        AZ_RTTI(GenericLineEditCtrlBase, "{0EC84840-666F-424E-9443-D20D8FEF743B}");
        AZ_CLASS_ALLOCATOR(GenericLineEditCtrlBase, AZ::SystemAllocator, 0);

        GenericLineEditCtrlBase(QWidget* pParent = nullptr);
        ~GenericLineEditCtrlBase() override = default;

        AZStd::string value() const;

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

    signals:
        void valueChanged(AZStd::string& newValue);

    public:
        void setValue(AZStd::string_view val);
        void setMaxLen(int maxLen);
        void onChildLineEditValueChange(const QString& value);

    protected:
        void focusInEvent(QFocusEvent* e) override;

    private:
        QLineEdit* m_pLineEdit;
    };

    template<typename T>
    class GenericLineEditCtrl
        : public GenericLineEditCtrlBase
    {
    public:
        friend class GenericLineEditHandler<T>;
        AZ_RTTI(((GenericLineEditCtrl<T>), "{4A094311-8956-40C9-95B5-7D50C2574B45}", T), GenericLineEditCtrlBase);
        AZ_CLASS_ALLOCATOR(GenericLineEditCtrl, AZ::SystemAllocator, 0);

        GenericLineEditCtrl(QWidget* pParent = nullptr)
            : GenericLineEditCtrlBase(pParent)
        {}
        ~GenericLineEditCtrl() override = default;

    private:
        // Stores per ctrl instance string <-> T conversion functions
        EditCtrl::PropertyToStringCB<T> m_propertyToStringCB;
        EditCtrl::StringToPropertyCB<T> m_stringToPropertyCB;
    };

    template<typename T>
    class GenericLineEditHandler
        : QObject
        , public AzToolsFramework::PropertyHandler<T, GenericLineEditCtrlBase>
    {
    public:
        AZ_CLASS_ALLOCATOR(GenericLineEditHandler, AZ::SystemAllocator, 0);
        
        GenericLineEditHandler(const EditCtrl::PropertyToStringCB<T>& propertyToStringCB, const EditCtrl::StringToPropertyCB<T>& stringToPropertyCB,
            const EditCtrl::StringValidatorCB& stringValidatorCB = {});

        AZ::u32 GetHandlerName(void) const override { return ScriptCanvas::Attributes::UIHandlers::GenericLineEdit; }
        QWidget* GetFirstInTabOrder(GenericLineEditCtrlBase* widget) override { return widget->GetFirstInTabOrder(); }
        QWidget* GetLastInTabOrder(GenericLineEditCtrlBase* widget) override { return widget->GetLastInTabOrder(); }
        void UpdateWidgetInternalTabbing(GenericLineEditCtrlBase* widget) override { widget->UpdateTabOrder(); }

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(GenericLineEditCtrlBase* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, GenericLineEditCtrlBase* GUI, typename GenericLineEditHandler::property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, GenericLineEditCtrlBase* GUI, const typename GenericLineEditHandler::property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

        bool AutoDelete() const override { return false; }

    private:
        // Stores per handler string <-> T conversion functions
        // There is only 1 handler per instantiated T
        EditCtrl::PropertyToStringCB<T> m_propertyToStringCB;
        EditCtrl::StringToPropertyCB<T> m_stringToPropertyCB;
        EditCtrl::StringValidatorCB m_stringValidatorCB;
    };

    template<typename T>
    AzToolsFramework::PropertyHandlerBase* RegisterGenericLineEditHandler(const EditCtrl::PropertyToStringCB<T>& propertyToStringCB, const EditCtrl::StringToPropertyCB<T>& stringToPropertyCB)
    {
        if (!AzToolsFramework::PropertyTypeRegistrationMessages::Bus::FindFirstHandler())
        {
            return nullptr;
        }

        auto propertyHandler(aznew GenericLineEditHandler<T>(propertyToStringCB, stringToPropertyCB));
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, propertyHandler);
        return propertyHandler;
    }
}

#include <ScriptCanvas/View/EditCtrls/GenericLineEditCtrl.inl>
