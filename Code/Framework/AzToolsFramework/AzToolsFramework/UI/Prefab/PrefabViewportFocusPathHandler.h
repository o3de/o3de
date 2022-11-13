/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <AzToolsFramework/Prefab/PrefabFocusNotificationBus.h>

#include <AzQtComponents/Components/Widgets/BreadCrumbs.h>

#include <QLayout>
#include <QToolButton>

namespace AzToolsFramework::Prefab
{
    class PrefabFocusPublicInterface;

    // Handler for the Prefab Focus Path widget used with the legacy Action Manager
    class PrefabViewportFocusPathHandler
        : public PrefabFocusNotificationBus::Handler
        , private QObject
    {
    public:
        PrefabViewportFocusPathHandler();
        ~PrefabViewportFocusPathHandler();

        void Initialize(AzQtComponents::BreadCrumbs* breadcrumbsWidget, QToolButton* backButton);

        // PrefabFocusNotificationBus overrides ...
        void OnPrefabFocusChanged(
            [[maybe_unused]] AZ::EntityId previousContainerEntityId, [[maybe_unused]] AZ::EntityId newContainerEntityId) override;
        void OnPrefabFocusRefreshed() override;

    private:
        void Refresh();

        AzQtComponents::BreadCrumbs* m_breadcrumbsWidget = nullptr;
        QToolButton* m_backButton = nullptr;

        AzFramework::EntityContextId m_editorEntityContextId = AzFramework::EntityContextId::CreateNull();

        PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
    };

    // Prefab Focus Path widget for use with the new Action Manager
    
    class PrefabFocusPathWidget
        : public AzQtComponents::BreadCrumbs
        , private PrefabFocusNotificationBus::Handler
    {
    public:
        PrefabFocusPathWidget();
        ~PrefabFocusPathWidget();

        // PrefabFocusNotificationBus overrides ...
        void OnPrefabFocusChanged(AZ::EntityId previousContainerEntityId, AZ::EntityId newContainerEntityId) override;
        void OnPrefabFocusRefreshed() override;

    private:
        void Refresh();

        AzFramework::EntityContextId m_editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
    };

} // namespace AzToolsFramework::Prefab
