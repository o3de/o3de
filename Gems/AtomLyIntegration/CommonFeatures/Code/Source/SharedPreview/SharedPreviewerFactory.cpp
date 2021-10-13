/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <SharedPreview/SharedPreviewUtils.h>
#include <SharedPreview/SharedPreviewer.h>
#include <SharedPreview/SharedPreviewerFactory.h>

namespace AZ
{
    namespace LyIntegration
    {
        AzToolsFramework::AssetBrowser::Previewer* SharedPreviewerFactory::CreatePreviewer(QWidget* parent) const
        {
            return new SharedPreviewer(parent);
        }

        bool SharedPreviewerFactory::IsEntrySupported(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
        {
            return SharedPreviewUtils::IsSupportedAssetType(entry->GetThumbnailKey());
        }

        const QString& SharedPreviewerFactory::GetName() const
        {
            return m_name;
        }
    } // namespace LyIntegration
} // namespace AZ
