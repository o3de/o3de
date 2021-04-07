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

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Source/Thumbnails/Preview/CommonPreviewer.h>
#include <Source/Thumbnails/Preview/CommonPreviewerFactory.h>
#include <Source/Thumbnails/ThumbnailUtils.h>

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
            return
                Thumbnails::GetAssetId(entry->GetThumbnailKey(), RPI::MaterialAsset::RTTI_Type()).IsValid() ||
                Thumbnails::GetAssetId(entry->GetThumbnailKey(), RPI::ModelAsset::RTTI_Type()).IsValid();
        }

        const QString& CommonPreviewerFactory::GetName() const
        {
            return m_name;
        }
    } // namespace LyIntegration
} // namespace AZ
