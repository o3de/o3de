/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
//AZTF-SHARED

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <AzToolsFramework/Prefab/PrefabFocusNotificationBus.h>

#include <AzQtComponents/Components/Widgets/BreadCrumbs.h>

#include <QLayout>
#include <QToolButton>

namespace AzToolsFramework::Prefab
{
    class PrefabFocusPublicInterface;

    // Displays the Prefab Path to the currently focused prefab file.
    class AZTF_API PrefabFocusPathWidget
        : public AzQtComponents::BreadCrumbs
        , private PrefabFocusNotificationBus::Handler
        , private ViewportEditorModeNotificationsBus::Handler
    {
    public:
        PrefabFocusPathWidget();
        ~PrefabFocusPathWidget();

        // PrefabFocusNotificationBus overrides ...
        void OnPrefabFocusChanged(AZ::EntityId previousContainerEntityId, AZ::EntityId newContainerEntityId) override;
        void OnPrefabFocusRefreshed() override;

    private:
        // ViewportEditorModeNotificationsBus overrides ...
        void OnEditorModeActivated(const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode) override;
        void OnEditorModeDeactivated(const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode) override;

        void Refresh();

        AzFramework::EntityContextId m_editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
    };

} // namespace AzToolsFramework::Prefab
