/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

class ViewportNudge
{
public:

    static void Nudge(EditorWindow* editorWindow,
        ViewportInteraction::InteractionMode interactionMode,
        ViewportWidget* viewport,
        ViewportInteraction::NudgeDirection direction,
        ViewportInteraction::NudgeSpeed speed,
        const QTreeWidgetItemRawPtrQList& selectedItems,
        ViewportInteraction::CoordinateSystem coordinateSystem,
        const AZ::Uuid& transformComponentType);
};
