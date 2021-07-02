/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/Previewer/Previewer.h>
#endif

namespace Ui
{
    class EmptyPreviewerClass;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;

        //! Widget displaying "no preview available" text
        class EmptyPreviewer
            : public Previewer
        {
            Q_OBJECT
        public:
            EmptyPreviewer(QWidget* parent = nullptr);

            //////////////////////////////////////////////////////////////////////////
            // AzToolsFramework::AssetBrowser::Previewer
            //////////////////////////////////////////////////////////////////////////
            void Clear() const override {}
            void Display(const AssetBrowserEntry* entry) override;
            const QString& GetName() const override;

            static const QString Name;

        private:
            QScopedPointer<Ui::EmptyPreviewerClass> m_ui;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
