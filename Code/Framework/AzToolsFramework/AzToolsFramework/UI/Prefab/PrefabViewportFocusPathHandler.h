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

    class PrefabViewportFocusPathHandler
        : public PrefabFocusNotificationBus::Handler
        , private QObject
    {
    public:
        PrefabViewportFocusPathHandler();
        ~PrefabViewportFocusPathHandler();

        void Initialize(AzQtComponents::BreadCrumbs* breadcrumbsWidget, QToolButton* backButton);

        // PrefabFocusNotificationBus overrides ...
        void OnPrefabFocusChanged() override;

    private:
        AzQtComponents::BreadCrumbs* m_breadcrumbsWidget = nullptr;
        QToolButton* m_backButton = nullptr;

        AzFramework::EntityContextId m_editorEntityContextId = AzFramework::EntityContextId::CreateNull();

        PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
    };
} // namespace AzToolsFramework::Prefab
