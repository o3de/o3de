/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Editor/EditorContextMenuBus.h>

class ContextMenuBottomHandler : private AzToolsFramework::EditorContextMenuBus::Handler
{
public:
    void Setup();
    void Teardown();

private:
    // EditorContextMenu overrides ...
    void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override;
    int GetMenuPosition() const override;
};
