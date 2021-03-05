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

#pragma once

#if !defined(Q_MOC_RUN)
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QWidget>
#include <QPushButton>
#endif


namespace EMotionFX
{
    class TransitionStateFilterPicker
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR_DECL

        TransitionStateFilterPicker(AnimGraphStateMachine* stateMachine, QWidget* parent);
        void SetStateMachine(AnimGraphStateMachine* stateMachine);

        void SetStateFilter(const AnimGraphStateTransition::StateFilterLocal& stateFilter);
        AnimGraphStateTransition::StateFilterLocal GetStateFilter() const;

    signals:
        void SelectionChanged();

    private slots:
        void OnPickClicked();
        void OnResetClicked();

    private:
        void UpdateInterface();

        AnimGraphStateMachine*                      m_stateMachine;
        AnimGraphStateTransition::StateFilterLocal  m_stateFilter;
        QPushButton*                                m_pickButton;
        QPushButton*                                m_resetButton;
    };


    class TransitionStateFilterLocalHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AnimGraphStateTransition::StateFilterLocal, TransitionStateFilterPicker>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        TransitionStateFilterLocalHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(TransitionStateFilterPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, TransitionStateFilterPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, TransitionStateFilterPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    private:
        AnimGraphStateMachine* m_stateMachine;
    };
} // namespace EMotionFX