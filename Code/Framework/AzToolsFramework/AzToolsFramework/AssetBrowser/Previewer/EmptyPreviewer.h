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