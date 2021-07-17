/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QFrame>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class PreviewerFactory;
        class AssetBrowserEntry;
        class AssetPreviewer;
        class Previewer;

        //! Widget managing previewers.
        class PreviewerFrame
            : public QFrame
        {
            Q_OBJECT
        public:
            PreviewerFrame(QWidget* parent = nullptr);

            //! Preview an asset browser entry with a corresponding previewer.
            //! If none are registered to preview this entry, empty previewer will be displayed.
            void Display(const AssetBrowserEntry* entry);
            //! Unload current previewer and show empty previewer.
            void Clear();

        private:
            const PreviewerFactory* FindPreviewerFactory(const AssetBrowserEntry* entry) const;
            void InstallPreviewer(Previewer* previewer);

            Previewer* m_previewer = nullptr;
        };
    } // AssetBrowser
} // AzToolsFramework
