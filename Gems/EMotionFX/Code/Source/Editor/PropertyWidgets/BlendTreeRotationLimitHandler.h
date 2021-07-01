/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <EMotionFX/Source/BlendTreeRotationLimitNode.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzCore/std/containers/array.h>
#endif

namespace EMotionFX
{
    class RotationLimitWdget : public QWidget
    {
        Q_OBJECT //AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL
        RotationLimitWdget(QWidget* parent);

        void SetRotationLimit(const BlendTreeRotationLimitNode::RotationLimit& rotationLimit);
        void UpdateGui();
        float GetMin() const;
        float GetMax() const;
    signals:
        void DataChanged();

    private slots:
        void HandleMinValueChanged(double value);
        void HandleMaxValueChanged(double value);

    private:
        static const int s_decimalPlaces;
        QString m_tooltipText;
        AzQtComponents::DoubleSpinBox* m_spinBoxMin = nullptr;
        AzQtComponents::DoubleSpinBox* m_spinBoxMax = nullptr;
        const BlendTreeRotationLimitNode::RotationLimit* m_rotationLimit = nullptr;
    };

    class BlendTreeRotationLimitHandler
        : public QObject,
        public AzToolsFramework::PropertyHandler<BlendTreeRotationLimitNode::RotationLimit, RotationLimitWdget>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        // Method to create the new widget - returns a GrowTextEdit*
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        // Method to return the name of the new handler - returns AZ::Edit::UIHandlers::MultiLineEdit, so it can be used
        // by when reflecting properties
        AZ::u32 GetHandlerName() const override;

        // Method used to parse attributes specified when a property is reflected, to configure the
        // the property handler
        void ConsumeAttribute(RotationLimitWdget* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        // Used to pull the values out of the widget
        void WriteGUIValuesIntoProperty(size_t index, RotationLimitWdget* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

        // Used to set the values on the widget
        bool ReadValuesIntoGUI(size_t index, RotationLimitWdget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };






    class RotationLimitContainerWdget : public QWidget
    {
        Q_OBJECT //AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL
        RotationLimitContainerWdget(QWidget* parent);
    };

    class BlendTreeRotationLimitContainerHandler
        : public QObject,
        public AzToolsFramework::PropertyHandler<AZStd::array<BlendTreeRotationLimitNode::RotationLimit, 3>, RotationLimitContainerWdget>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        // Method to create the new widget - returns a GrowTextEdit*
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        // Method to return the name of the new handler - returns AZ::Edit::UIHandlers::MultiLineEdit, so it can be used
        // by when reflecting properties
        AZ::u32 GetHandlerName() const override;

        // Method used to parse attributes specified when a property is reflected, to configure the
        // the property handler
        void ConsumeAttribute(RotationLimitContainerWdget* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        // Used to pull the values out of the widget
        void WriteGUIValuesIntoProperty(size_t index, RotationLimitContainerWdget* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

        // Used to set the values on the widget
        bool ReadValuesIntoGUI(size_t index, RotationLimitContainerWdget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };
}
