/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzFramework/Physics/Ragdoll.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <QModelIndex>
#include <MCore/Source/Command.h>
#endif


QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace EMotionFX
{
    class ObjectEditor;

    class RagdollJointLimitWidget
        : public AzQtComponents::Card
    {
        Q_OBJECT //AUTOMOC

    public:
        explicit RagdollJointLimitWidget(const AZStd::string& copiedJointLimits, QWidget* parent = nullptr);
        ~RagdollJointLimitWidget() override;

        void Update(const QModelIndex& modelIndex);
        void Update() { Update(m_nodeIndex); }
        bool HasJointLimit() const;

    signals:
        void JointLimitCopied(const AZStd::string& serializedJointLimits) const;

    private slots:
        void OnJointTypeChanged(int index);
        void OnHasLimitStateChanged(int state);
        void OnCardContextMenu(const QPoint& position);

    private:
        Physics::RagdollNodeConfiguration* GetRagdollNodeConfig() const;
        void ChangeLimitType(const AZ::TypeId& type);
        void ChangeLimitType(int supportedTypeIndex);

        QModelIndex                     m_nodeIndex;
        QIcon                           m_cardHeaderIcon;
        EMotionFX::ObjectEditor*        m_objectEditor;
        QCheckBox*                      m_hasLimitCheckbox;
        QLabel*                         m_typeLabel;
        QComboBox*                      m_typeComboBox;
        const AZStd::string&            m_copiedJointLimits;
        static int                      s_leftMargin;
        static int                      s_textColumnWidth;

        class DEFINECOMMANDCALLBACK_API DataChangedCallback
            : public MCore::Command::Callback
        {
            MCORE_MEMORYOBJECTCATEGORY(DataChangedCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MCore::MCORE_MEMCATEGORY_COMMANDSYSTEM);
        public:
            explicit DataChangedCallback(const RagdollJointLimitWidget* widget, bool executePreUndo, bool executePreCommand = false)
                : MCore::Command::Callback(executePreUndo, executePreCommand)
                , m_widget(widget)
            {}
            bool Execute(MCore::Command * command, const MCore::CommandLine & commandLine) override;
            bool Undo(MCore::Command * command, const MCore::CommandLine & commandLine) override;

        private:
            const RagdollJointLimitWidget* m_widget{};
        };
        AZStd::vector<AZStd::unique_ptr<MCore::Command::Callback>> m_commandCallbacks;

        friend class DataChangedCallback;
    };
} // namespace EMotionFX
