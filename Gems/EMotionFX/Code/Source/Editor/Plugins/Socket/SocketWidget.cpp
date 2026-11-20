/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Quaternion.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/SocketCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/SocketSetup.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/PluginManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/ViewportPluginBus.h>
#include <Editor/Plugins/Socket/SocketManipulators.h>
#include <Editor/Plugins/Socket/SocketWidget.h>
#include <QAction>
#include <QComboBox>
#include <QCursor>
#include <QEvent>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

namespace EMotionFX
{
    namespace
    {
        QDoubleSpinBox* CreateSpinBox(double minimum, double maximum, double step, int decimals)
        {
            QDoubleSpinBox* spinBox = new QDoubleSpinBox();
            spinBox->setRange(minimum, maximum);
            spinBox->setSingleStep(step);
            spinBox->setDecimals(decimals);
            // Only apply typed values on Enter or focus loss so that typing does not create one undo step per keystroke.
            spinBox->setKeyboardTracking(false);
            return spinBox;
        }

        QWidget* CreateVectorRow(QDoubleSpinBox* const (&spinBoxes)[3])
        {
            QWidget* row = new QWidget();
            QHBoxLayout* layout = new QHBoxLayout(row);
            layout->setContentsMargins(0, 0, 0, 0);
            for (QDoubleSpinBox* spinBox : spinBoxes)
            {
                layout->addWidget(spinBox);
            }
            return row;
        }

        // A wireframe arrow: a shaft plus a four-sided pyramid head, all plain lines so it stays cheap to draw.
        void AddArrowLines(AZStd::vector<AZ::Vector3>& lines, const AZ::Vector3& origin, const AZ::Vector3& forward, const AZ::Vector3& sideA, const AZ::Vector3& sideB, float length)
        {
            const float headLength = length * 0.3f;
            const float headRadius = length * 0.1f;
            const float shaftRadius = length * 0.035f;
            const AZ::Vector3 tip = origin + forward * length;
            const AZ::Vector3 headBase = tip - forward * headLength;

            // A square ring of four corners around the axis at the given position.
            const auto ringCorners = [&sideA, &sideB](const AZ::Vector3& center, float radius, AZ::Vector3 (&corners)[4])
            {
                corners[0] = center + sideA * radius;
                corners[1] = center + sideB * radius;
                corners[2] = center - sideA * radius;
                corners[3] = center - sideB * radius;
            };
            const auto addRing = [&lines](const AZ::Vector3 (&corners)[4])
            {
                for (int corner = 0; corner < 4; ++corner)
                {
                    lines.push_back(corners[corner]);
                    lines.push_back(corners[(corner + 1) % 4]);
                }
            };

            // The body is a thin square tube: rings at the start, middle and head base joined by four lengthwise lines.
            AZ::Vector3 startRing[4];
            AZ::Vector3 middleRing[4];
            AZ::Vector3 baseRing[4];
            ringCorners(origin, shaftRadius, startRing);
            ringCorners((origin + headBase) * 0.5f, shaftRadius, middleRing);
            ringCorners(headBase, shaftRadius, baseRing);
            addRing(startRing);
            addRing(middleRing);
            addRing(baseRing);
            for (int corner = 0; corner < 4; ++corner)
            {
                lines.push_back(startRing[corner]);
                lines.push_back(baseRing[corner]);
            }

            // The head is a wider four-sided pyramid.
            AZ::Vector3 headRing[4];
            ringCorners(headBase, headRadius, headRing);
            addRing(headRing);
            for (int corner = 0; corner < 4; ++corner)
            {
                lines.push_back(tip);
                lines.push_back(headRing[corner]);
            }
        }

        // The forward arrow (local +Y) is cyan and the side ticks are grey, so none of them can be mistaken for the red, green and blue move/rotate gizmo.
        void DrawSocket(AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Transform& worldTransform, const char* name, bool selected)
        {
            const AZ::Vector3 origin = worldTransform.GetTranslation();
            const AZ::Vector3 axisX = worldTransform.GetBasisX().GetNormalizedSafe();
            const AZ::Vector3 axisY = worldTransform.GetBasisY().GetNormalizedSafe();
            const AZ::Vector3 axisZ = worldTransform.GetBasisZ().GetNormalizedSafe();
            const float axisLength = selected ? 0.08f : 0.05f;
            const float forwardLength = selected ? 0.2f : 0.12f;

            debugDisplay.SetColor(AZ::Color(0.7f, 0.7f, 0.7f, 1.0f));
            debugDisplay.DrawLine(origin - axisX * axisLength, origin + axisX * axisLength);
            debugDisplay.DrawLine(origin - axisZ * axisLength, origin + axisZ * axisLength);

            AZStd::vector<AZ::Vector3> arrowLines;
            arrowLines.reserve(48);
            AddArrowLines(arrowLines, origin, axisY, axisX, axisZ, forwardLength);
            debugDisplay.DrawLines(arrowLines, selected ? AZ::Color(0.2f, 1.0f, 1.0f, 1.0f) : AZ::Color(0.0f, 0.75f, 0.85f, 1.0f));

            debugDisplay.SetColor(selected ? AZ::Color(1.0f, 1.0f, 0.2f, 1.0f) : AZ::Color(0.9f, 0.9f, 0.9f, 1.0f));
            debugDisplay.DrawTextLabel(origin, 1.0f, name, /*center=*/true);
        }
    }

    SocketWidget::SocketWidget()
        : EMStudio::DockWidgetPlugin()
    {
    }

    SocketWidget::~SocketWidget()
    {
        for (MCore::Command::Callback* callback : m_commandCallbacks)
        {
            CommandSystem::GetCommandManager()->RemoveCommandCallback(callback, true);
        }
        m_commandCallbacks.clear();

        ActorEditorNotificationBus::Handler::BusDisconnect();
        m_manipulators.reset();
    }

    bool SocketWidget::Init()
    {
        m_noActorLabel = new QLabel(tr("Select an actor to edit its sockets."));
        m_noActorLabel->setWordWrap(true);

        m_list = new QListWidget();
        m_list->setObjectName("EMFX.SocketWidget.List");
        m_list->setSelectionMode(QAbstractItemView::SingleSelection);
        m_list->viewport()->installEventFilter(this);

        m_addButton = new QPushButton(tr("Add socket"));
        m_addButton->setObjectName("EMFX.SocketWidget.AddButton");
        m_addButton->setToolTip(tr("Add a socket on the joint selected in the Skeleton Outliner, or on the root joint."));
        m_removeButton = new QPushButton(tr("Remove socket"));
        m_removeButton->setObjectName("EMFX.SocketWidget.RemoveButton");

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->addWidget(m_addButton);
        buttonLayout->addWidget(m_removeButton);

        m_nameEdit = new QLineEdit();
        m_nameEdit->setObjectName("EMFX.SocketWidget.NameEdit");
        m_nameEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[^\"]*"), m_nameEdit));

        m_jointCombo = new QComboBox();
        m_jointCombo->setObjectName("EMFX.SocketWidget.JointCombo");

        for (QDoubleSpinBox*& spinBox : m_positionSpin)
        {
            spinBox = CreateSpinBox(-10000.0, 10000.0, 0.01, 4);
        }
        for (QDoubleSpinBox*& spinBox : m_rotationSpin)
        {
            spinBox = CreateSpinBox(-360.0, 360.0, 1.0, 2);
        }

        QLabel* hintLabel = new QLabel(tr("Offsets are relative to the joint. The socket's forward direction is its local +Y axis (the green arrow); rotation is XYZ Euler degrees."));
        hintLabel->setWordWrap(true);

        m_details = new QWidget();
        QGridLayout* detailsLayout = new QGridLayout(m_details);
        detailsLayout->setContentsMargins(0, 0, 0, 0);
        detailsLayout->addWidget(new QLabel(tr("Name")), 0, 0);
        detailsLayout->addWidget(m_nameEdit, 0, 1);
        detailsLayout->addWidget(new QLabel(tr("Joint")), 1, 0);
        detailsLayout->addWidget(m_jointCombo, 1, 1);
        detailsLayout->addWidget(new QLabel(tr("Position")), 2, 0);
        detailsLayout->addWidget(CreateVectorRow(m_positionSpin), 2, 1);
        detailsLayout->addWidget(new QLabel(tr("Rotation")), 3, 0);
        detailsLayout->addWidget(CreateVectorRow(m_rotationSpin), 3, 1);
        detailsLayout->addWidget(hintLabel, 4, 0, 1, 2);

        m_content = new QWidget();
        QVBoxLayout* contentLayout = new QVBoxLayout(m_content);
        contentLayout->setContentsMargins(0, 0, 0, 0);
        contentLayout->addLayout(buttonLayout);
        contentLayout->addWidget(m_list, /*stretch=*/1);
        contentLayout->addWidget(m_details);

        QWidget* mainWidget = new QWidget();
        QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
        mainLayout->addWidget(m_noActorLabel);
        mainLayout->addWidget(m_content, /*stretch=*/1);
        m_dock->setWidget(mainWidget);

        connect(m_list, &QListWidget::itemSelectionChanged, this, &SocketWidget::OnSocketSelectionChanged);
        connect(m_addButton, &QPushButton::clicked, this, &SocketWidget::OnAddSocket);
        connect(m_removeButton, &QPushButton::clicked, this, &SocketWidget::OnRemoveSocket);
        connect(m_nameEdit, &QLineEdit::editingFinished, this, &SocketWidget::OnNameEdited);
        connect(m_jointCombo, QOverload<int>::of(&QComboBox::activated), this, &SocketWidget::OnJointChanged);
        for (QDoubleSpinBox* spinBox : m_positionSpin)
        {
            connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SocketWidget::OnPositionEdited);
        }
        for (QDoubleSpinBox* spinBox : m_rotationSpin)
        {
            connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SocketWidget::OnRotationEdited);
        }

        m_manipulators = AZStd::make_unique<SocketManipulators>();

        // Pick up the actor that is already selected.
        ActorInstance* actorInstance = nullptr;
        ActorEditorRequestBus::BroadcastResult(actorInstance, &ActorEditorRequests::GetSelectedActorInstance);
        if (actorInstance)
        {
            SetActor(actorInstance->GetActor(), actorInstance);
        }
        else
        {
            Actor* actor = nullptr;
            ActorEditorRequestBus::BroadcastResult(actor, &ActorEditorRequests::GetSelectedActor);
            SetActor(actor, nullptr);
        }

        // Command callbacks keep the list in sync with undo and redo.
        m_commandCallbacks.emplace_back(new DataChangedCallback(/*executePreUndo*/ false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandAddSocket::s_commandName, m_commandCallbacks.back());
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandRemoveSocket::s_commandName, m_commandCallbacks.back());
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandAdjustSocket::s_commandName, m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new AddSocketCallback(/*executePreUndo*/ false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandAddSocket::s_commandName, m_commandCallbacks.back());

        ActorEditorNotificationBus::Handler::BusConnect();

        return true;
    }

    bool SocketWidget::eventFilter(QObject* watched, QEvent* event)
    {
        if (m_list && watched == m_list->viewport() && event->type() == QEvent::MouseButtonPress)
        {
            const QPoint cursorPosition = m_list->viewport()->mapFromGlobal(QCursor::pos());
            if (!m_list->indexAt(cursorPosition).isValid())
            {
                m_list->clearSelection();
                m_list->setCurrentRow(-1);
            }
        }
        return EMStudio::DockWidgetPlugin::eventFilter(watched, event);
    }

    void SocketWidget::SetActor(Actor* actor, ActorInstance* actorInstance)
    {
        m_actor = actor;
        m_actorInstance = actorInstance;

        // Joint indices match the combo box rows, so selecting a joint by index needs no lookup.
        m_updatingUi = true;
        m_jointCombo->clear();
        if (m_actor)
        {
            const Skeleton* skeleton = m_actor->GetSkeleton();
            for (size_t nodeIndex = 0; nodeIndex < skeleton->GetNumNodes(); ++nodeIndex)
            {
                m_jointCombo->addItem(QString::fromUtf8(skeleton->GetNode(nodeIndex)->GetName()));
            }
        }
        m_updatingUi = false;

        Refresh();
    }

    void SocketWidget::ActorSelectionChanged(Actor* actor)
    {
        SetActor(actor, nullptr);
    }

    void SocketWidget::ActorInstanceSelectionChanged(ActorInstance* actorInstance)
    {
        SetActor(actorInstance ? actorInstance->GetActor() : nullptr, actorInstance);
    }

    void SocketWidget::Refresh(const AZStd::string& selectName)
    {
        const ActorSocket* currentSocket = GetCurrentSocket();
        const AZStd::string keepName = !selectName.empty() ? selectName : (currentSocket ? currentSocket->GetName() : AZStd::string());

        m_updatingUi = true;
        m_list->clear();
        int selectRow = -1;
        if (m_actor)
        {
            const SocketSetup& setup = *m_actor->GetSocketSetup();
            for (size_t socketIndex = 0; socketIndex < setup.GetNumSockets(); ++socketIndex)
            {
                const ActorSocket& socket = setup.GetSocket(socketIndex);
                QListWidgetItem* item = new QListWidgetItem(QString::fromUtf8(socket.GetName().c_str()), m_list);
                if (socket.GetJointIndex() >= m_actor->GetNumNodes())
                {
                    item->setForeground(Qt::red);
                    item->setToolTip(tr("The joint '%1' is missing from the skeleton.").arg(QString::fromUtf8(socket.GetJointName().c_str())));
                }
                if (socket.GetName() == keepName)
                {
                    selectRow = static_cast<int>(socketIndex);
                }
            }
        }
        // Nothing is selected unless the previous or requested socket still exists, so the gizmos only show for a selected socket.
        m_list->clearSelection();
        m_list->setCurrentRow(-1);
        if (selectRow >= 0)
        {
            m_list->setCurrentRow(selectRow);
        }

        m_noActorLabel->setVisible(m_actor == nullptr);
        m_content->setVisible(m_actor != nullptr);
        m_addButton->setEnabled(m_actor != nullptr);
        m_updatingUi = false;

        UpdateEditors();
    }

    const ActorSocket* SocketWidget::GetCurrentSocket() const
    {
        // The selection, not the current item, because clicking an empty area clears the selection but keeps the current item.
        const QList<QListWidgetItem*> selectedItems = m_list ? m_list->selectedItems() : QList<QListWidgetItem*>();
        if (!m_actor || selectedItems.isEmpty())
        {
            return nullptr;
        }
        return m_actor->GetSocketSetup()->FindSocket(AZStd::string(selectedItems.first()->text().toUtf8().constData()));
    }

    void SocketWidget::UpdateEditors()
    {
        const ActorSocket* socket = GetCurrentSocket();

        m_updatingUi = true;
        m_details->setEnabled(socket != nullptr);
        m_removeButton->setEnabled(socket != nullptr);

        if (socket)
        {
            m_nameEdit->setText(QString::fromUtf8(socket->GetName().c_str()));
            const size_t jointIndex = socket->GetJointIndex();
            m_jointCombo->setCurrentIndex(jointIndex < static_cast<size_t>(m_jointCombo->count()) ? static_cast<int>(jointIndex) : -1);

            const AZ::Vector3& position = socket->GetLocalPosition();
            const AZ::Vector3 euler = AZ::ConvertQuaternionToEulerDegrees(socket->GetLocalRotation());
            for (int axis = 0; axis < 3; ++axis)
            {
                m_positionSpin[axis]->setValue(position.GetElement(axis));
                m_rotationSpin[axis]->setValue(euler.GetElement(axis));
            }
        }
        else
        {
            m_nameEdit->clear();
            m_jointCombo->setCurrentIndex(-1);
            for (int axis = 0; axis < 3; ++axis)
            {
                m_positionSpin[axis]->setValue(0.0);
                m_rotationSpin[axis]->setValue(0.0);
            }
        }
        m_updatingUi = false;

        // The viewport gizmos follow the selected socket.
        if (m_manipulators)
        {
            m_manipulators->SetTarget(socket ? m_actorInstance : nullptr, socket ? socket->GetName() : AZStd::string());
        }
    }

    void SocketWidget::OnSocketSelectionChanged()
    {
        if (!m_updatingUi)
        {
            UpdateEditors();
        }
    }

    void SocketWidget::OnAddSocket()
    {
        if (!m_actor || m_actor->GetNumNodes() == 0)
        {
            return;
        }

        Node* selectedNode = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(selectedNode, &SkeletonOutlinerRequests::GetSingleSelectedNode);
        const Node* jointNode = selectedNode ? selectedNode : m_actor->GetSkeleton()->GetNode(0);
        CommandSocketHelpers::AddSocket(m_actor->GetID(), AZStd::string(), jointNode->GetNameString(), AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity());
    }

    void SocketWidget::OnRemoveSocket()
    {
        const ActorSocket* socket = GetCurrentSocket();
        if (socket)
        {
            // Copy the name first; the command invalidates the socket.
            const AZStd::string name = socket->GetName();
            CommandSocketHelpers::RemoveSocket(m_actor->GetID(), name);
        }
    }

    void SocketWidget::OnNameEdited()
    {
        const ActorSocket* socket = GetCurrentSocket();
        if (m_updatingUi || !socket)
        {
            return;
        }

        const AZStd::string oldName = socket->GetName();
        const AZStd::string newName = m_nameEdit->text().toUtf8().constData();
        if (newName == oldName)
        {
            return;
        }

        if (CommandSocketHelpers::AdjustSocket(m_actor->GetID(), oldName, newName, AZStd::nullopt, AZStd::nullopt, AZStd::nullopt))
        {
            Refresh(newName);
        }
        else
        {
            // Empty or duplicate name; put the old one back.
            UpdateEditors();
        }
    }

    void SocketWidget::OnJointChanged()
    {
        const ActorSocket* socket = GetCurrentSocket();
        if (m_updatingUi || !socket || m_jointCombo->currentIndex() < 0)
        {
            return;
        }

        const AZStd::string name = socket->GetName();
        const AZStd::string jointName = m_jointCombo->currentText().toUtf8().constData();
        CommandSocketHelpers::AdjustSocket(m_actor->GetID(), name, AZStd::nullopt, jointName, AZStd::nullopt, AZStd::nullopt);
    }

    void SocketWidget::OnPositionEdited()
    {
        const ActorSocket* socket = GetCurrentSocket();
        if (m_updatingUi || !socket)
        {
            return;
        }

        const AZ::Vector3 position(
            static_cast<float>(m_positionSpin[0]->value()),
            static_cast<float>(m_positionSpin[1]->value()),
            static_cast<float>(m_positionSpin[2]->value()));
        const AZStd::string name = socket->GetName();
        CommandSocketHelpers::AdjustSocket(m_actor->GetID(), name, AZStd::nullopt, AZStd::nullopt, position, AZStd::nullopt);
    }

    void SocketWidget::OnRotationEdited()
    {
        const ActorSocket* socket = GetCurrentSocket();
        if (m_updatingUi || !socket)
        {
            return;
        }

        const AZ::Vector3 euler(
            static_cast<float>(m_rotationSpin[0]->value()),
            static_cast<float>(m_rotationSpin[1]->value()),
            static_cast<float>(m_rotationSpin[2]->value()));
        const AZStd::string name = socket->GetName();
        CommandSocketHelpers::AdjustSocket(m_actor->GetID(), name, AZStd::nullopt, AZStd::nullopt, AZStd::nullopt, AZ::ConvertEulerDegreesToQuaternion(euler));
    }

    void SocketWidget::Render(EMotionFX::ActorRenderFlags renderFlags)
    {
        // The Sockets entry of the viewport's Render Options menu hides both the drawing and the gizmos.
        const bool visible = AZ::RHI::CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::Sockets);
        if (m_manipulators)
        {
            m_manipulators->SetVisible(visible);
        }

        if (!visible || !m_actor || !m_actorInstance || m_actor->GetSocketSetup()->GetNumSockets() == 0)
        {
            return;
        }

        AZ::s32 viewportId = -1;
        EMStudio::ViewportPluginRequestBus::BroadcastResult(viewportId, &EMStudio::ViewportPluginRequestBus::Events::GetViewportId);
        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, viewportId);
        AzFramework::DebugDisplayRequests* debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
        if (!debugDisplay)
        {
            return;
        }

        const ActorSocket* selectedSocket = GetCurrentSocket();
        const Pose* pose = m_actorInstance->GetTransformData()->GetCurrentPose();
        const SocketSetup& setup = *m_actor->GetSocketSetup();
        for (size_t socketIndex = 0; socketIndex < setup.GetNumSockets(); ++socketIndex)
        {
            const ActorSocket& socket = setup.GetSocket(socketIndex);
            if (socket.GetJointIndex() >= m_actor->GetNumNodes())
            {
                continue;
            }

            // While a gizmo is being dragged the socket is drawn at the dragged value, before the command commits it.
            AZ::Transform localTransform = socket.GetLocalTransform();
            AZ::Vector3 previewPosition;
            AZ::Quaternion previewRotation;
            if (m_manipulators && m_manipulators->GetPreview(socket.GetName(), previewPosition, previewRotation))
            {
                localTransform = AZ::Transform::CreateFromQuaternionAndTranslation(previewRotation, previewPosition);
            }

            const AZ::Transform worldTransform = pose->GetWorldSpaceTransform(socket.GetJointIndex()).ToAZTransform() * localTransform;
            DrawSocket(*debugDisplay, worldTransform, socket.GetName().c_str(), &socket == selectedSocket);
        }
    }

    bool SocketWidget::DataChangedCallback::Execute([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        SocketWidget* plugin = static_cast<SocketWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(SocketWidget::CLASS_ID));
        if (plugin)
        {
            plugin->Refresh();
        }
        return true;
    }

    bool SocketWidget::DataChangedCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        return Execute(command, commandLine);
    }

    bool SocketWidget::AddSocketCallback::Execute(MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        SocketWidget* plugin = static_cast<SocketWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(SocketWidget::CLASS_ID));
        if (plugin)
        {
            plugin->Refresh(static_cast<CommandAddSocket*>(command)->GetSocketName());
        }
        return true;
    }

    bool SocketWidget::AddSocketCallback::Undo([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return true;
    }
} // namespace EMotionFX
