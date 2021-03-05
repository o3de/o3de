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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorCommon_precompiled.h"
#include "BatchFileDialog.h"
#include "QPropertyTree/QPropertyDialog.h"
#include "Serialization/StringList.h"
#include "Serialization/STL.h"
#include "Serialization/IArchive.h"
#include "Serialization/STLImpl.h"
#include "IEditor.h"
#include "Pak/CryPakUtils.h"
#include <vector>
#include <AzFramework/Archive/IArchive.h>
#include <StringUtils.h>
#include <QApplication>
#include <QMessageBox>
#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <Util/PathUtil.h>
#include <QDirIterator>

struct SBatchFileItem
{
    bool selected;
    string path;
    bool checkable = false;

    bool operator<(const SBatchFileItem& rhs) const { return path < rhs.path; }

    void Serialize(Serialization::IArchive& ar)
    {
        // -------------------------------------------------------------------
        // Note: The property tree label modifiers used below indicate...
        //          ! - Readonly, you can't modify the path
        //          ^ - Raise location to the parent (the index indicator)
        //          < - Take up the rest of the space
        // -------------------------------------------------------------------
        if (!checkable)
        {
            ar(selected, "selected", "^");
        }

        auto gamePath = PathUtil::MakeGamePath(path);
        ar(gamePath, "path", "!^<");
    }
};
typedef std::vector<SBatchFileItem> SBatchFileItems;

struct CBatchFileDialog::SContent
{
    SBatchFileItems items;
    string listLabel;

    SContent(const char* itemsLabelText, bool readonlyList)
    {
        // Respect the readonly settings
        if (readonlyList)
        {
            listLabel += "!";
        }

        // Label Size (5px per character)
        auto labelSize = static_cast<int>(strlen(itemsLabelText)) * 5;
        {
            // Format is: >#>+ where the >#> indicates the label size, and the + indicates end of all row formatting
            char buffer[16];
            sprintf_s(buffer, sizeof(buffer), ">%i>+", labelSize);
            listLabel += buffer;
        }

        // Add the actual label text (the text that is seen)
        listLabel += itemsLabelText;
    }

    void Serialize(Serialization::IArchive& ar)
    {
        ar(items, "items", listLabel.c_str());
    }
};

static bool ReadFile(std::vector<char>* buffer, const char* path)
{
    FILE* f = nullptr;
    azfopen(&f, path, "rb");
    if (!f)
    {
        return false;
    }
    fseek(f, 0, SEEK_END);
    size_t len = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);

    buffer->resize(len);
    bool result = true;
    if (len)
    {
        result = fread(&(*buffer)[0], 1, len, f) == len;
    }
    fclose(f);
    return result;
}

static void SplitLines(std::vector<string>* lines, const char* start, const char* end)
{
    const char* p = start;
    const char* lineStart = start;
    while (true)
    {
        if (p == end || *p == '\r' || *p == '\n')
        {
            if (p != lineStart)
            {
                string line(lineStart, p);
                bool hasPrintableChars = false;
                for (size_t i = 0; i < line.size(); ++i)
                {
                    if (!isspace(line[i]))
                    {
                        hasPrintableChars = true;
                    }
                }
                if (hasPrintableChars)
                {
                    lines->push_back(line);
                }
            }
            lineStart = p + 1;
            if (p == end)
            {
                break;
            }
        }
        ++p;
    }
}

static string NormalizePath(const char* path)
{
    string result = path;
    result.replace('\\', '/');
    result.MakeLower();

    // strip .phys extensions in case list of .cdf is provided
    string::size_type dotPos = result.rfind('.');
    if (dotPos != string::npos)
    {
        if (_stricmp(result.c_str() + dotPos, ".phys"))
        {
            result.erase(dotPos, 5);
        }
    }
    return result;
}

static bool IsEquivalentPath(const char* pathA, const char* pathB)
{
    string normalizedA = NormalizePath(pathA);
    string normalizedB = NormalizePath(pathB);
    return normalizedA == normalizedB;
}

void CBatchFileDialog::OnLoadList()
{
    QString existingFile = QFileDialog::getOpenFileName(m_dialog, "Load file list...", QString(), QString("Text Files (*.txt)"));
    if (existingFile.isEmpty())
    {
        return;
    }

    string path = existingFile.toLocal8Bit().data();
    std::vector<char> content;
    ReadFile(&content, path.c_str());

    std::vector<string> lines;
    SplitLines(&lines, &content[0], &content[0] + content.size());

    for (size_t i = 0; i < m_content->items.size(); ++i)
    {
        m_content->items[i].selected = false;
    }

    for (size_t i = 0; i < lines.size(); ++i)
    {
        const char* line = lines[i].c_str();
        for (size_t j = 0; j < m_content->items.size(); ++j)
        {
            const char* itemPath = m_content->items[j].path.c_str();
            if (IsEquivalentPath(line, itemPath))
            {
                m_content->items[j].selected = true;
                break;
            }
        }
    }

    m_dialog->revert();
}

void CBatchFileDialog::OnSelectAll()
{
    for (size_t i = 0; i < m_content->items.size(); ++i)
    {
        m_content->items[i].selected = true;
    }

    m_dialog->revert();
}

void CBatchFileDialog::OnSelectNone()
{
    for (size_t i = 0; i < m_content->items.size(); ++i)
    {
        m_content->items[i].selected = false;
    }

    m_dialog->revert();
}

bool EDITOR_COMMON_API ShowBatchFileDialog(Serialization::StringList* result, const SBatchFileSettings& settings, QWidget* parent)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    CBatchFileDialog::SContent content(settings.listLabel, settings.readonlyList);
    if (settings.scanExtension[0] != '\0')
    {
        if (settings.useCryPak)
        {
            AZStd::vector<AZStd::string> files;

            string mask = "*.";
            mask += settings.scanExtension;
            SDirectoryEnumeratorHelper helper;
            helper.ScanDirectoryRecursive(gEnv->pCryPak, Path::GetEditingGameDataFolder().c_str(), "", mask.c_str(), files);

            for (int k = 0; k < files.size(); ++k)
            {
                SBatchFileItem item;
                item.checkable = settings.filesAreCheckable;
                item.selected = true;
                item.path = { files[k].data(), files[k].size() };
                content.items.push_back(item);
            }
        }
        else
        {
            string gameFolder = Path::GetEditingGameDataFolder().c_str();
            int modIndex = 0;
            string gamePrefix = GetIEditor()->GetPrimaryCDFolder().toUtf8().data();
            if (!gamePrefix.empty() && gamePrefix[gamePrefix.size() - 1] != '\\')
            {
                gamePrefix += "\\";
            }
            gamePrefix += gameFolder;
            if (!gamePrefix.empty() && gamePrefix[gamePrefix.size() - 1] != '\\')
            {
                gamePrefix += "\\";
            }
            gamePrefix.replace('/', '\\');

            QString mask = "*." + QString(settings.scanExtension);
            QDirIterator dirIterator(QString(gamePrefix), QStringList() << mask, QDir::Files, QDirIterator::Subdirectories);
            while (dirIterator.hasNext())
            {
                SBatchFileItem item;
                item.selected = true;
                QByteArray array = dirIterator.next().toUtf8();
                item.path = string(array);
                item.path.replace('/', '\\');
                content.items.push_back(item);
            }

        }
    }

    content.items.reserve(content.items.size() + settings.explicitFileList.size());
    for (size_t i = 0; i < settings.explicitFileList.size(); ++i)
    {
        SBatchFileItem item;
        item.path = settings.explicitFileList[i].c_str();
        item.selected = true;
        content.items.push_back(item);
    }


    std::sort(content.items.begin(), content.items.end());
    QApplication::restoreOverrideCursor();

    QPropertyDialog dialog(parent);
    dialog.setSerializer(Serialization::SStruct(content));
    dialog.setWindowTitle(settings.title);
    dialog.setWindowStateFilename(settings.stateFilename);
    dialog.setSizeHint(QSize(settings.defaultWidth, settings.defaultHeight));
    dialog.setMinimumSize(QSize(540, 250));

    CBatchFileDialog handler;
    handler.m_dialog = &dialog;
    handler.m_content = &content;

    QBoxLayout* topRow = new QBoxLayout(QBoxLayout::LeftToRight);
    QLabel* label = new QLabel(settings.descriptionText);
    QFont font;
    font.setBold(true);
    label->setFont(font);
    topRow->addWidget(label, 1);
    {
        if (settings.allowListLoading && !settings.readonlyList)
        {
            QPushButton* loadListButton = new QPushButton("Load List...");
            QObject::connect(loadListButton, SIGNAL(pressed()), &handler, SLOT(OnLoadList()));
            topRow->addWidget(loadListButton);
        }
        QPushButton* selectAllButton = new QPushButton("Select All");
        QObject::connect(selectAllButton, SIGNAL(pressed()), &handler, SLOT(OnSelectAll()));
        topRow->addWidget(selectAllButton);
        QPushButton* selectNoneButton = new QPushButton("Select None");
        QObject::connect(selectNoneButton, SIGNAL(pressed()), &handler, SLOT(OnSelectNone()));
        topRow->addWidget(selectNoneButton);
    }
    dialog.layout()->insertLayout(0, topRow);

    if (parent)
    {
        QPoint center = parent->rect().center();
        dialog.window()->move(max(0, center.x() - dialog.width() / 2),
            max(0, center.y() - dialog.height() / 2));
    }

    int numFailed = 0;
    int numSaved = 0;
    std::vector<string> failedFiles;
    if (dialog.exec() == QDialog::Accepted)
    {
        result->clear();
        for (size_t i = 0; i < content.items.size(); ++i)
        {
            if (content.items[i].selected)
            {
                const char* path = content.items[i].path.c_str();
                result->push_back(path);
            }
        }
        return true;
    }
    return false;
}

#include <moc_BatchFileDialog.cpp>
