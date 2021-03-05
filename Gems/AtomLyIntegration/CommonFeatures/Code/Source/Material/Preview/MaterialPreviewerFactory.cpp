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
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>

#include <Source/Material/Preview/MaterialPreviewer.h>
#include <Source/Material/Preview/MaterialPreviewerFactory.h>

namespace AZ
{
    namespace LyIntegration
    {
        AzToolsFramework::AssetBrowser::Previewer* MaterialPreviewerFactory::CreatePreviewer(QWidget* parent) const
        {
            return new MaterialPreviewer(parent);
        }

        bool MaterialPreviewerFactory::IsEntrySupported(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
        {
            using namespace AzToolsFramework::AssetBrowser;

            switch (entry->GetEntryType())
            {
            case AssetBrowserEntry::AssetEntryType::Source:
            {
                const auto source = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
                return source->GetPrimaryAssetType() == RPI::MaterialAsset::RTTI_Type();
            }
            case AssetBrowserEntry::AssetEntryType::Product:
                const auto product = azrtti_cast<const ProductAssetBrowserEntry*>(entry);
                return product->GetAssetType() == RPI::MaterialAsset::RTTI_Type();
            }
            return false;
        }

        const QString& MaterialPreviewerFactory::GetName() const
        {
            return m_name;
        }
    } // namespace LyIntegration
} // namespace AZ
