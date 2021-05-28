/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            AZ_CLASS_ALLOCATOR(EditorWindow, AZ::SystemAllocator, 0);
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

