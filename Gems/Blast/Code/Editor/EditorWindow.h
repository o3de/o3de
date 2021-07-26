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
#include <Blast/BlastSystemBus.h>
#endif

namespace Ui
{
    class EditorWindowClass;
}

namespace Blast
{
    namespace Editor
    {
        /// Window pane wrapper for the Blast Configuration Widget.
        class EditorWindow : public QWidget
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(EditorWindow, AZ::SystemAllocator, 0);
            static void RegisterViewClass();

            explicit EditorWindow(QWidget* parent = nullptr);

        private:
            static void SaveConfiguration(const BlastGlobalConfiguration& materialLibrary);

            QScopedPointer<Ui::EditorWindowClass> m_ui;
        };
    } // namespace Editor
}; // namespace Blast
