/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <SharedPreview/SharedPreviewer.h>
#include <SharedPreview/SharedPreviewerFactory.h>
#include <SharedPreview/SharedPreviewUtils.h>

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
            return
                Thumbnails::GetAssetId(entry->GetThumbnailKey(), RPI::MaterialAsset::RTTI_Type()).IsValid() ||
                Thumbnails::GetAssetId(entry->GetThumbnailKey(), RPI::ModelAsset::RTTI_Type()).IsValid();
        }

        const QString& SharedPreviewerFactory::GetName() const
        {
            return m_name;
        }
    } // namespace LyIntegration
} // namespace AZ
