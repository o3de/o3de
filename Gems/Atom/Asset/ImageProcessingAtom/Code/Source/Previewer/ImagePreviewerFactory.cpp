/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ImageProcessing_precompiled.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT

#include <Source/Previewer/ImagePreviewer.h>
#include <Source/Previewer/ImagePreviewerFactory.h>

#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>

#include <ImageLoader/ImageLoaders.h>

AZ_POP_DISABLE_WARNING

namespace ImageProcessingAtom
{
    AzToolsFramework::AssetBrowser::Previewer* ImagePreviewerFactory::CreatePreviewer(QWidget* parent) const
    {
        return new ImagePreviewer(parent);
    }

    bool ImagePreviewerFactory::IsEntrySupported(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
    {
        using namespace AzToolsFramework::AssetBrowser;

        EBusFindAssetTypeByName textureAssetTypeResult("StreamingImage");
        AZ::AssetTypeInfoBus::BroadcastResult(textureAssetTypeResult, &AZ::AssetTypeInfo::GetAssetType);

        switch (entry->GetEntryType())
        {
        case AssetBrowserEntry::AssetEntryType::Source:
        {
            const auto source = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
            AZStd::string extention = source->GetExtension();
            // Skip the '.' when check the extension 
            return IsExtensionSupported(extention.c_str() + 1);
        }
        case AssetBrowserEntry::AssetEntryType::Product:
            const auto product = azrtti_cast<const ProductAssetBrowserEntry*>(entry);
            return product->GetAssetType() == textureAssetTypeResult.GetAssetType();
        }
        return false;
    }

    const QString& ImagePreviewerFactory::GetName() const
    {
        return m_name;
    }
}//namespace ImageProcessingAtom
