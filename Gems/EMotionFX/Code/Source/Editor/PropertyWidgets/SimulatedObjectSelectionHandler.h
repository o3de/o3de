/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#endif

namespace EMotionFX
{
    class SimulatedObjectPicker
        : public QWidget
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        SimulatedObjectPicker(QWidget* parent);
        ~SimulatedObjectPicker() = default;

        // Called when the UI wants to update the simulated objects
        void UpdateSimulatedObjects(const AZStd::vector<AZStd::string>& simulatedObjectNames);

        void SetSimulatedObjects(const AZStd::vector<AZStd::string>& simulatedObjectNames);
        const AZStd::vector<AZStd::string>& GetSimulatedObjects() const;

    signals:
        void SelectionChanged(const AZStd::vector<AZStd::string>& newSimualtedObjectNames);

    private slots:
        void OnPickClicked();
        void OnResetClicked();

    private:
        void UpdateInterface();

        AZStd::vector<AZStd::string>                m_simulatedObjectNames;

        QPushButton*                                m_pickButton = nullptr;
        QPushButton*                                m_resetButton = nullptr;
    };


    class SimulatedObjectSelectionHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, SimulatedObjectPicker>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(SimulatedObjectPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, SimulatedObjectPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, SimulatedObjectPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };
} // namespace EMotionFX
