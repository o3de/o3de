/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: LuaScriptPreviewerFactory.
 * Create: 2021-07-14
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/AssetBrowser/Previewer/PreviewerFactory.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <QString>
#include <AzCore/PlatformIncl.h>

namespace LuaScript
{
    class LuaScriptPreviewerFactory final
        : public AzToolsFramework::AssetBrowser::PreviewerFactory
    {
    public:
        AZ_CLASS_ALLOCATOR(LuaScriptPreviewerFactory, AZ::SystemAllocator, 0);

        LuaScriptPreviewerFactory() = default;
        ~LuaScriptPreviewerFactory() = default;

        //! AzToolsFramework::AssetBrowser::PreviewerFactory overrides
        AzToolsFramework::AssetBrowser::Previewer* CreatePreviewer(QWidget* parent = nullptr) const override;
        bool IsEntrySupported(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const override;
        const QString& GetName() const override;

    private:
        QString m_name = "LuaScriptPreviewer";
    };
} //namespace LuaScript
