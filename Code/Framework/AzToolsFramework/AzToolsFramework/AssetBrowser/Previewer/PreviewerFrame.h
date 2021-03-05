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
