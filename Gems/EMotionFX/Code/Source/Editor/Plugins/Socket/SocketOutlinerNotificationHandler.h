/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <QObject>

namespace EMotionFX
{
    // Adds the "Socket" entries to the skeleton outliner's right-click menu, next to the hit detection and ragdoll ones.
    class SocketOutlinerNotificationHandler
        : public QObject
        , private SkeletonOutlinerNotificationBus::Handler
    {
        Q_OBJECT //AUTOMOC

    public:
        SocketOutlinerNotificationHandler();
        ~SocketOutlinerNotificationHandler() override;

        // SkeletonOutlinerNotificationBus overrides
        void OnContextMenu(QMenu* menu, const QModelIndexList& selectedRowIndices) override;

    public slots:
        void OnAddSockets();
        void OnRemoveSockets();
    };
} // namespace EMotionFX
