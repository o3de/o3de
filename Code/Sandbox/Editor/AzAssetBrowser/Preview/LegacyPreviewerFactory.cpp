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

#include "EditorDefs.h"

#include "LegacyPreviewerFactory.h"

// AzToolsFramework
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>        // for AssetBrowserEntry::AssetEntryType
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>  // for EBusFindAssetTypeByName

// Editor
#include "LegacyPreviewer.h"


AzToolsFramework::AssetBrowser::Previewer* LegacyPreviewerFactory::CreatePreviewer(QWidget* parent) const
{
    return new LegacyPreviewer(parent);
}

bool LegacyPreviewerFactory::IsEntrySupported(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
{
    using namespace AzToolsFramework::AssetBrowser;

    EBusFindAssetTypeByName meshAssetTypeResult("Static Mesh");
    AZ::AssetTypeInfoBus::BroadcastResult(meshAssetTypeResult, &AZ::AssetTypeInfo::GetAssetType);
    EBusFindAssetTypeByName textureAssetTypeResult("Texture");
    AZ::AssetTypeInfoBus::BroadcastResult(textureAssetTypeResult, &AZ::AssetTypeInfo::GetAssetType);

    switch (entry->GetEntryType())
    {
    case AssetBrowserEntry::AssetEntryType::Source:
    {
        const auto* source = azrtti_cast < const SourceAssetBrowserEntry * > (entry);
        if (source->GetPrimaryAssetType() == textureAssetTypeResult.GetAssetType())
        {
            return true;
        }
        AZStd::vector < const ProductAssetBrowserEntry * > products;
        source->GetChildrenRecursively < ProductAssetBrowserEntry > (products);
        for (auto* product : products)
        {
            if (product->GetAssetType() == textureAssetTypeResult.GetAssetType() ||
                product->GetAssetType() == meshAssetTypeResult.GetAssetType())
            {
                return true;
            }
        }
        break;
    }
    case AssetBrowserEntry::AssetEntryType::Product:
        const auto* product = azrtti_cast < const ProductAssetBrowserEntry * > (entry);
        return product->GetAssetType() == textureAssetTypeResult.GetAssetType() ||
               product->GetAssetType() == meshAssetTypeResult.GetAssetType();
    }
    return false;
}

const QString& LegacyPreviewerFactory::GetName() const
{
    return LegacyPreviewer::Name;
}
