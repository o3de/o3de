/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
//AZTF-SHARED

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <QtWidgets/QWidget>
#include "PropertyEditorAPI.h"
#endif

class QPushButton;
class InstanceDataNode;

namespace AzToolsFramework
{
    class AZTF_API PropertyButtonCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyButtonCtrl, AZ::SystemAllocator);

        PropertyButtonCtrl(QWidget* pParent = NULL);
        virtual ~PropertyButtonCtrl();

        void SetButtonText(const char* text);
        void SetButtonToolTip(const char* description);

        QString GetButtonText() const;

Q_SIGNALS:
        void buttonPressed();

    private:
        bool eventFilter(QObject* object, QEvent* event) override;

        QPushButton* m_button;
    };

    class AZTF_API ButtonHandlerCommon
        : public QObject
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ButtonHandlerCommon, AZ::SystemAllocator);
        QWidget* CreateGUICommon(QWidget* pParent);
        void ConsumeAttributeCommon(PropertyButtonCtrl* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName);
    };

    class AZTF_API ButtonGenericHandler
        : public ButtonHandlerCommon
        , public GenericPropertyHandler<PropertyButtonCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ButtonGenericHandler, AZ::SystemAllocator);

        QWidget* CreateGUI(QWidget* pParent) override;
        AZ::u32 GetHandlerName() const override { return AZ::Edit::UIHandlers::Button; }
        void WriteGUIValuesIntoProperty(size_t index, PropertyButtonCtrl* GUI, void* value, const AZ::Uuid& propertyType) override;
        bool ReadValueIntoGUI(size_t index, PropertyButtonCtrl* GUI, void* value, const AZ::Uuid& propertyType) override;
        void ConsumeAttribute(PropertyButtonCtrl* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
    };

    class AZTF_API ButtonBoolHandler
        : public ButtonHandlerCommon
        , public PropertyHandler<bool, PropertyButtonCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ButtonBoolHandler, AZ::SystemAllocator);

        QWidget* CreateGUI(QWidget* pParent) override;
        AZ::u32 GetHandlerName() const override { return AZ::Edit::UIHandlers::Button; }
        void WriteGUIValuesIntoProperty(size_t index, PropertyButtonCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyButtonCtrl* GUI, const property_t& instance, InstanceDataNode* node) override;
        void ConsumeAttribute(PropertyButtonCtrl* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
    };
    
    class AZTF_API ButtonStringHandler
        : public ButtonHandlerCommon
        , public PropertyHandler<AZStd::string, PropertyButtonCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ButtonStringHandler, AZ::SystemAllocator);

        QWidget* CreateGUI(QWidget* pParent) override;
        AZ::u32 GetHandlerName() const override { return AZ::Edit::UIHandlers::Button; }
        void WriteGUIValuesIntoProperty(size_t index, PropertyButtonCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyButtonCtrl* GUI, const property_t& instance, InstanceDataNode* node) override;
        void ConsumeAttribute(PropertyButtonCtrl* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
    };

    AZTF_API void RegisterButtonPropertyHandlers();

}; // namespace AzToolsFramework
