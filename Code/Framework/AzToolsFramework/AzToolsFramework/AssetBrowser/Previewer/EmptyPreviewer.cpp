/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/base.h>
#include <AzToolsFramework/AssetBrowser/Previewer/EmptyPreviewer.h>

// Disables warning messages triggered by the Qt library
// 4251: class needs to have dll-interface to be used by clients of class 
// 4800: forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <AzToolsFramework/AssetBrowser/Previewer/ui_EmptyPreviewer.h>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        const QString EmptyPreviewer::Name{ QStringLiteral("EmptyPreviewer") };

        EmptyPreviewer::EmptyPreviewer(QWidget* parent)
            : Previewer(parent)
            , m_ui(new Ui::EmptyPreviewerClass())
        {
            m_ui->setupUi(this);
        }

        void EmptyPreviewer::Display(const AssetBrowserEntry* entry)
        {
            AZ_UNUSED(entry);
        }

        const QString& EmptyPreviewer::GetName() const
        {
            return Name;
        }
    }
}

#include <AssetBrowser/Previewer/moc_EmptyPreviewer.cpp>
