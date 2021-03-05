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
