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
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QWidget>
#include <QPushButton>
#endif


namespace EMotionFX
{
    class ActorGoalNodePicker
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR_DECL

        ActorGoalNodePicker(QWidget* parent);

        void SetGoalNode(const AZStd::pair<AZStd::string, int>& goalNode);
        AZStd::pair<AZStd::string, int> GetGoalNode() const;

    signals:
        void SelectionChanged();

        private slots:
        void OnPickClicked();
        void OnResetClicked();

    private:
        void UpdateInterface();

        AZStd::pair<AZStd::string, int> m_goalNode; // Node name and the parent depth (0=current, 1=parent, 2=parent of parent, 3=parent of parent of parent, etc.)
        QPushButton*                    m_pickButton;
        QPushButton*                    m_resetButton;
    };


    class ActorGoalNodeHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::pair<AZStd::string, int>, ActorGoalNodePicker>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(ActorGoalNodePicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, ActorGoalNodePicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, ActorGoalNodePicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };
} // namespace EMotionFX