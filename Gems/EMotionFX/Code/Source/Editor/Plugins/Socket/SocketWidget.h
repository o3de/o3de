/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <Editor/ActorEditorBus.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <MCore/Source/Command.h>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QDoubleSpinBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QListWidget)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace EMotionFX
{
    class Actor;
    class ActorInstance;
    class ActorSocket;
    class SocketManipulators;

    // Dock window for authoring the sockets of the selected actor; every edit goes through the socket commands so it is undoable.
    class SocketWidget
        : public EMStudio::DockWidgetPlugin
        , private ActorEditorNotificationBus::Handler
    {
        Q_OBJECT //AUTOMOC

    public:
        enum
        {
            CLASS_ID = 0x00861165
        };

        static constexpr const char* PluginName = "Sockets";

        SocketWidget();
        ~SocketWidget() override;
        SocketWidget(const SocketWidget&) = delete;
        SocketWidget(SocketWidget&&) = delete;
        SocketWidget& operator=(const SocketWidget&) = delete;
        SocketWidget& operator=(SocketWidget&&) = delete;

        // EMStudioPlugin overrides
        const char* GetName() const override { return PluginName; }
        uint32 GetClassID() const override { return CLASS_ID; }
        bool GetIsClosable() const override { return true; }
        bool GetIsFloatable() const override { return true; }
        bool GetIsVertical() const override { return false; }
        EMStudioPlugin* Clone() const override { return new SocketWidget(); }
        bool Init() override;

        void Render(EMotionFX::ActorRenderFlags renderFlags) override;

        // Rebuilds the socket list from the actor, keeping the current selection or selecting selectName when given.
        void Refresh(const AZStd::string& selectName = AZStd::string());

        // Clicking an empty area of the socket list deselects, which a single-selection list does not do on its own.
        bool eventFilter(QObject* watched, QEvent* event) override;

        // ActorEditorNotificationBus overrides
        void ActorSelectionChanged(Actor* actor) override;
        void ActorInstanceSelectionChanged(ActorInstance* actorInstance) override;

    private slots:
        void OnSocketSelectionChanged();
        void OnAddSocket();
        void OnRemoveSocket();
        void OnNameEdited();
        void OnJointChanged();
        void OnPositionEdited();
        void OnRotationEdited();

    private:
        void SetActor(Actor* actor, ActorInstance* actorInstance);
        const ActorSocket* GetCurrentSocket() const;
        void UpdateEditors();

        Actor* m_actor = nullptr;
        ActorInstance* m_actorInstance = nullptr;

        QLabel* m_noActorLabel = nullptr;
        QWidget* m_content = nullptr;
        QListWidget* m_list = nullptr;
        QPushButton* m_addButton = nullptr;
        QPushButton* m_removeButton = nullptr;
        QWidget* m_details = nullptr;
        QLineEdit* m_nameEdit = nullptr;
        QComboBox* m_jointCombo = nullptr;
        QDoubleSpinBox* m_positionSpin[3] = {};
        QDoubleSpinBox* m_rotationSpin[3] = {};

        AZStd::unique_ptr<SocketManipulators> m_manipulators;

        // Set while the editors are being filled from the data, so that doing so does not execute commands.
        bool m_updatingUi = false;

        MCORE_DEFINECOMMANDCALLBACK(DataChangedCallback);
        MCORE_DEFINECOMMANDCALLBACK(AddSocketCallback);
        AZStd::vector<MCore::Command::Callback*> m_commandCallbacks;
    };
} // namespace EMotionFX
