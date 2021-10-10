/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <Previewer/CommonPreviewer.h>
#include <Previewer/CommonPreviewerFactory.h>
#include <Previewer/CommonThumbnailUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        AzToolsFramework::AssetBrowser::Previewer* CommonPreviewerFactory::CreatePreviewer(QWidget* parent) const
        {
            return new CommonPreviewer(parent);
        }

        bool CommonPreviewerFactory::IsEntrySupported(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
        {
            return Thumbnails::IsSupportedThumbnail(entry->GetThumbnailKey());
        }

        const QString& CommonPreviewerFactory::GetName() const
        {
            return m_name;
        }
    } // namespace LyIntegration
} // namespace AZ
