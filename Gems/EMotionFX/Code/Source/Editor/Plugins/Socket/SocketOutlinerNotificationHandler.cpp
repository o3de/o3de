/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <Editor/Plugins/Socket/SocketHelpers.h>
#include <Editor/Plugins/Socket/SocketOutlinerNotificationHandler.h>
#include <Editor/SkeletonModel.h>
#include <QAction>
#include <QMenu>

namespace EMotionFX
{
    SocketOutlinerNotificationHandler::SocketOutlinerNotificationHandler()
    {
        SkeletonOutlinerNotificationBus::Handler::BusConnect();
    }

    SocketOutlinerNotificationHandler::~SocketOutlinerNotificationHandler()
    {
        SkeletonOutlinerNotificationBus::Handler::BusDisconnect();
    }

    void SocketOutlinerNotificationHandler::OnContextMenu(QMenu* menu, const QModelIndexList& selectedRowIndices)
    {
        if (selectedRowIndices.empty())
        {
            return;
        }

        const Actor* actor = selectedRowIndices[0].data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        if (!actor)
        {
            return;
        }

        bool anyJointHasSockets = false;
        for (const QModelIndex& index : selectedRowIndices)
        {
            const Node* joint = index.data(SkeletonModel::ROLE_POINTER).value<Node*>();
            if (joint && SocketHelpers::JointHasSockets(*actor, *joint))
            {
                anyJointHasSockets = true;
                break;
            }
        }

        QMenu* socketMenu = menu->addMenu(tr("Socket"));
        QAction* addSocketAction = socketMenu->addAction(tr("Add socket"));
        addSocketAction->setObjectName("EMFX.SocketOutlinerNotificationHandler.AddSocketAction");
        connect(addSocketAction, &QAction::triggered, this, &SocketOutlinerNotificationHandler::OnAddSockets);

        if (anyJointHasSockets)
        {
            QAction* removeSocketsAction = socketMenu->addAction(tr("Remove sockets"));
            removeSocketsAction->setObjectName("EMFX.SocketOutlinerNotificationHandler.RemoveSocketsAction");
            connect(removeSocketsAction, &QAction::triggered, this, &SocketOutlinerNotificationHandler::OnRemoveSockets);
        }
    }

    void SocketOutlinerNotificationHandler::OnAddSockets()
    {
        AZ::Outcome<QModelIndexList> selectedRowIndicesOutcome;
        SkeletonOutlinerRequestBus::BroadcastResult(selectedRowIndicesOutcome, &SkeletonOutlinerRequests::GetSelectedRowIndices);
        if (selectedRowIndicesOutcome.IsSuccess())
        {
            SocketHelpers::AddSocketsToJoints(selectedRowIndicesOutcome.GetValue());
        }
    }

    void SocketOutlinerNotificationHandler::OnRemoveSockets()
    {
        AZ::Outcome<QModelIndexList> selectedRowIndicesOutcome;
        SkeletonOutlinerRequestBus::BroadcastResult(selectedRowIndicesOutcome, &SkeletonOutlinerRequests::GetSelectedRowIndices);
        if (selectedRowIndicesOutcome.IsSuccess())
        {
            SocketHelpers::RemoveSocketsFromJoints(selectedRowIndicesOutcome.GetValue());
        }
    }
} // namespace EMotionFX
