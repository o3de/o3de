/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#ifndef PROPERTY_BUTTON_CTRL
#define PROPERTY_BUTTON_CTRL

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <QtWidgets/QWidget>
#include "PropertyEditorAPI.h"
#endif

class QPushButton;
class InstanceDataNode;

namespace AzToolsFramework
{
    class PropertyButtonCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyButtonCtrl, AZ::SystemAllocator, 0);

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

    class ButtonHandlerCommon
        : public QObject
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ButtonHandlerCommon, AZ::SystemAllocator, 0);
        QWidget* CreateGUICommon(QWidget* pParent);
        void ConsumeAttributeCommon(PropertyButtonCtrl* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName);
    };

    class ButtonGenericHandler
        : public ButtonHandlerCommon
        , public GenericPropertyHandler<PropertyButtonCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ButtonGenericHandler, AZ::SystemAllocator, 0);

        QWidget* CreateGUI(QWidget* pParent) override;
        AZ::u32 GetHandlerName() const override { return AZ::Edit::UIHandlers::Button; }
        void WriteGUIValuesIntoProperty(size_t index, PropertyButtonCtrl* GUI, void* value, const AZ::Uuid& propertyType) override;
        bool ReadValueIntoGUI(size_t index, PropertyButtonCtrl* GUI, void* value, const AZ::Uuid& propertyType) override;
        void ConsumeAttribute(PropertyButtonCtrl* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
    };

    class ButtonBoolHandler
        : public ButtonHandlerCommon
        , public PropertyHandler<bool, PropertyButtonCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ButtonBoolHandler, AZ::SystemAllocator, 0);

        QWidget* CreateGUI(QWidget* pParent) override;
        AZ::u32 GetHandlerName() const override { return AZ::Edit::UIHandlers::Button; }
        void WriteGUIValuesIntoProperty(size_t index, PropertyButtonCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyButtonCtrl* GUI, const property_t& instance, InstanceDataNode* node) override;
        void ConsumeAttribute(PropertyButtonCtrl* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
    };
    
    class ButtonStringHandler
        : public ButtonHandlerCommon
        , public PropertyHandler<AZStd::string, PropertyButtonCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ButtonStringHandler, AZ::SystemAllocator, 0);

        QWidget* CreateGUI(QWidget* pParent) override;
        AZ::u32 GetHandlerName() const override { return AZ::Edit::UIHandlers::Button; }
        void WriteGUIValuesIntoProperty(size_t index, PropertyButtonCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyButtonCtrl* GUI, const property_t& instance, InstanceDataNode* node) override;
        void ConsumeAttribute(PropertyButtonCtrl* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
    };

    void RegisterButtonPropertyHandlers();

}; // namespace AzToolsFramework

#endif // PROPERTY_BUTTON_CTRL
