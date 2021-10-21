/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: LuaScriptPreviewerFactory.
 * Create: 2021-07-14
 */

#include "LuaScriptPreviewer.h"
#include "LuaScriptPreviewerFactory.h"
#include <AzCore/Script/ScriptAsset.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT

AZ_POP_DISABLE_WARNING

namespace LuaScript
{
    AzToolsFramework::AssetBrowser::Previewer* LuaScriptPreviewerFactory::CreatePreviewer(QWidget* parent) const
    {
        return new LuaScriptPreviewer(parent);
    }

    bool LuaScriptPreviewerFactory::IsEntrySupported(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
    {
        using namespace AzToolsFramework::AssetBrowser;
        switch (entry->GetEntryType())
        {
            case AssetBrowserEntry::AssetEntryType::Source:
            {
                const auto source = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
                AZStd::string extension = source->GetExtension();
                QString ext = QString(extension.c_str() + 1).toLower();
                return ext == "lua";
            }
            case AssetBrowserEntry::AssetEntryType::Product:
                const auto product = azrtti_cast<const ProductAssetBrowserEntry*>(entry);
                return product->GetAssetType() == AZ::AzTypeInfo<AZ::ScriptAsset>::Uuid();
        }
        return false;
    }

    const QString& LuaScriptPreviewerFactory::GetName() const
    {
        return m_name;
    }
}//namespace LuaScript
