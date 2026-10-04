/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QModelIndexList>

namespace EMotionFX
{
    class Actor;
    class Node;

    namespace SocketHelpers
    {
        // Floats the Sockets window if it is not open, so that added sockets can be seen and edited.
        void OpenSocketPanel();

        bool JointHasSockets(const Actor& actor, const Node& joint);

        // Adds one socket on every selected skeleton outliner joint as a single undo step, then opens the Sockets window.
        bool AddSocketsToJoints(const QModelIndexList& selectedRowIndices);

        // Removes every socket that sits on one of the selected joints as a single undo step.
        bool RemoveSocketsFromJoints(const QModelIndexList& selectedRowIndices);
    } // namespace SocketHelpers
} // namespace EMotionFX
