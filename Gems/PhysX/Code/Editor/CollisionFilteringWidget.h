/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <AzQtComponents/Components/Widgets/SegmentControl.h>
#include <QWidget>
#endif

namespace PhysX
{
    class Configuration;

    namespace Editor
    {
        class CollisionLayersWidget;
        class CollisionGroupsWidget;
        class DocumentationLinkWidget;

        /// Container widget for wrapping the collision filtering ux
        /// Wraps the CollisionLayers and CollisionGroups widgets
        /// and presents them in a segment control.
        class CollisionFilteringWidget
            : public QWidget
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(CollisionFilteringWidget, AZ::SystemAllocator);

            explicit CollisionFilteringWidget(QWidget* parent = nullptr);

            void SetConfiguration(const AzPhysics::CollisionLayers& layers, const AzPhysics::CollisionGroups& groups);

            void ShowLayersTab();

            void ShowGroupsTab();

        signals:
            void onConfigurationChanged(const AzPhysics::CollisionLayers& layers, const AzPhysics::CollisionGroups& groups);

        private:
            void ConnectSignals();
            AzQtComponents::SegmentControl* m_tabs;
            CollisionLayersWidget* m_collisionLayersWidget;
            CollisionGroupsWidget* m_collisionGroupsWidget;
            DocumentationLinkWidget* m_documentationLinkWidget;
            AzPhysics::CollisionLayers m_layersConfig;
            AzPhysics::CollisionGroups m_groupsConfig;
        };
    }
}
