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
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <MCore/Source/CommandManagerCallback.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>
#include <QWidget>
#include <QPushButton>
#include <QToolTip>
#include <QVBoxLayout>
#endif


namespace EMotionFX
{
    class AnimGraphStateMachine;
    class AnimGraphStateTransition;

    class AnimGraphTransitionIdSelector
    {
    public:
        AnimGraphTransitionIdSelector();
        ~AnimGraphTransitionIdSelector();
        
        void StartSelection(AnimGraphStateMachine* stateMachine, const AZStd::vector<AZ::u64>& transitionIds);
        void StopSelection(AnimGraphStateTransition* transition);

        bool IsSelecting() const { return m_isSelecting; }
        void ResetUI();

    private:

        bool m_isSelecting = false;

        static const QColor s_graphWindowBorderOverwriteColor;
        static const float s_graphWindowBorderOverwriteWidth;
    };

    class AnimGraphTransitionIdPicker
        : public QWidget
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphTransitionIdPicker(QWidget* parent);
        ~AnimGraphTransitionIdPicker();
        void SetTransition(AnimGraphStateTransition* transition);

        void SetTransitionIds(const AZStd::vector<AZ::u64>& transitionIds);
        const AZStd::vector<AZ::u64>& GetTransitionIds() const;

    signals:
        void SelectionChanged();

    private slots:
        void OnPickClicked();
        void OnAboutToBeRemoved(const QModelIndex &parent, int first, int last);

    private:
        AnimGraphStateMachine* GetStateMachine() const;
        void Reinit();
        void UpdateInterface();
        QString GetTransitionNameById(const AnimGraphConnectionId transitionId);

        AnimGraphStateTransition* m_transition = nullptr;
        AZStd::vector<AZ::u64> m_transitionIds;

        QVBoxLayout* m_mainLayout = nullptr;
        QWidget* m_widget = nullptr;
        QLabel* m_label = nullptr;
        AZStd::vector<QPushButton*> m_removeButtons;
        QPushButton* m_pickButton = nullptr;

        AnimGraphTransitionIdSelector m_transitionSelector;
    };


    class AnimGraphMultiTransitionIdHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZ::u64>, AnimGraphTransitionIdPicker>
    {
        Q_OBJECT // AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphMultiTransitionIdHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(AnimGraphTransitionIdPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, AnimGraphTransitionIdPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, AnimGraphTransitionIdPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    protected:
        AnimGraphStateTransition* m_transition = nullptr;
    };
} // namespace EMotionFX