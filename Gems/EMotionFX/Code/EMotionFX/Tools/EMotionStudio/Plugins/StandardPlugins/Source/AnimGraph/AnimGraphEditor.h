/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/Command.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <Editor/AnimGraphEditorBus.h>
#include <QWidget>
#endif


QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
}

namespace EMotionFX
{
    class AnimGraph;

    class AnimGraphEditor
        : public QWidget
        , public EMotionFX::AnimGraphEditorRequestBus::Handler
    {
        Q_OBJECT //AUTOMOC

    public:
        AnimGraphEditor(EMotionFX::AnimGraph* animGraph, AZ::SerializeContext* serializeContext, QWidget* parent);
        virtual ~AnimGraphEditor();

        // AnimGraphEditorRequests
        EMotionFX::MotionSet* GetSelectedMotionSet() override;
        AnimGraph* GetAnimGraph() const { return m_animGraph; }
        void SetAnimGraph(AnimGraph* animGraph);

        void UpdateMotionSetComboBox() override;

        QComboBox* GetMotionSetComboBox() const;
    private slots:
        void OnMotionSetChanged(int index);

    private:
        AZ::Outcome<size_t> GetMotionSetIndex(int comboBoxIndex) const;

        MCORE_DEFINECOMMANDCALLBACK(UpdateMotionSetComboBoxCallback)

        AnimGraph*                                  m_animGraph;
        QLabel*                                     m_filenameLabel;
        AzToolsFramework::ReflectedPropertyEditor*  m_propertyEditor;
        static const int                            s_propertyLabelWidth;
        QComboBox*                                  m_motionSetComboBox;
        static QString                              s_lastMotionSetText;
        AZStd::vector<MCore::Command::Callback*>    m_commandCallbacks;

    };
} // namespace EMotionFX
