/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <QWidget>
#endif

namespace AzPhysics
{
    class CollisionConfiguration;
    struct SceneConfiguration;
}

namespace Ui
{
    class EditorWindowClass;
}

namespace PhysX
{
    struct PhysXSystemConfiguration;
    namespace Debug
    {
        struct DebugConfiguration;
    }

    namespace Editor
    {
        /// Window pane wrapper for the PhysX Configuration Widget.
        ///
        class EditorWindow
            : public QWidget
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(EditorWindow, AZ::SystemAllocator);
            static void RegisterViewClass();

            explicit EditorWindow(QWidget* parent = nullptr);

        private:
            static void SaveConfiguration(
                const PhysX::PhysXSystemConfiguration& physXSystemConfiguration,
                const PhysX::Debug::DebugConfiguration& physXDebugConfiguration,
                const AzPhysics::SceneConfiguration& defaultSceneConfiguration);

            QScopedPointer<Ui::EditorWindowClass> m_ui;
        };
    }
};

