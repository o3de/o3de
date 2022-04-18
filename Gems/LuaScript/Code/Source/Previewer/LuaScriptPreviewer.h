/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: LuaScriptPreviewer.
 * Create: 2021-07-14
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/AssetBrowser/Previewer/Previewer.h>
#include "ui_LuaScriptPreviewer.h"
#include <QWidget>
#include <QScopedPointer>
#include <AzCore/PlatformIncl.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>

#endif

namespace Ui
{
    class LuaScriptPreviewerClass;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class ProductAssetBrowserEntry;
        class SourceAssetBrowserEntry;
        class AssetBrowserEntry;
    }
}

namespace LuaScript
{
    class LuaScriptPreviewer
        : public AzToolsFramework::AssetBrowser::Previewer
    {
        //Q_OBJECT
    public:
        LuaScriptPreviewer(QWidget* parent = nullptr);
        // AzToolsFramework::AssetBrowser::Previewer
        void Clear() const override;
        void Display(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) override;
        const QString& GetName() const override;

        static const QString Name;
    private:
        QScopedPointer<Ui::LuaScriptPreviewerClass> m_ui;
        QString m_name = "LuaScriptPreviewer";
    };
}//namespace LuaScript
