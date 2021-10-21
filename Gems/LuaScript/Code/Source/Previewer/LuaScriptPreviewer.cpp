/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: LuaScriptPreviewer.
 * Create: 2021-07-14
 */

#include <AzCore/base.h>
#include "LuaScriptPreviewer.h"
#include <AzCore/IO/SystemFile.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")

AZ_POP_DISABLE_WARNING

namespace LuaScript
{
    LuaScriptPreviewer::LuaScriptPreviewer(QWidget* parent)
        : Previewer(parent)
        , m_ui(new Ui::LuaScriptPreviewerClass())
    {
        m_ui->setupUi(this);
    }

    void LuaScriptPreviewer::Clear() const
    {
        m_ui->m_previewText->hide();
    }

    void LuaScriptPreviewer::Display(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
    {
        AZStd::string filename = entry->GetFullPath();
        if (!AZ::IO::SystemFile::Exists(filename.c_str()))
        {
            return;
        }

        uint64_t fileSize = AZ::IO::SystemFile::Length(filename.c_str());
        if (fileSize == 0)
        {
            return;
        }

        AZStd::vector<char> buffer(fileSize + 1);
        buffer[fileSize] = 0;
        if (!AZ::IO::SystemFile::Read(filename.c_str(), buffer.data()))
        {
            return;
        }
        AZStd::string contentStr;
        contentStr.assign(buffer.begin(), buffer.end());
        m_ui->m_previewText->setText(QString::fromStdString(contentStr.c_str()));
    }

    const QString& LuaScriptPreviewer::GetName() const
    {
        return m_name;
    }
}//namespace LuaScript
