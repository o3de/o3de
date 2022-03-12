/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecentFiles.h"
#include "MysticQtManager.h"
#include <AzCore/IO/Path/Path.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtGui/QHelpEvent>
#include <QtWidgets/QToolTip>


namespace MysticQt
{
    class ToolTipMenu
        : public QMenu
    {
    public:
        ToolTipMenu(const QString title, QWidget* parent)
            : QMenu(title, parent)
        {
            m_parent = parent;
        }

        bool event(QEvent* e) override
        {
            bool result = QMenu::event(e);
            const QHelpEvent* helpEvent = static_cast<QHelpEvent*>(e);

            if (helpEvent->type() == QEvent::ToolTip)
            {
                QAction* action = activeAction();
                if (action)
                {
                    QToolTip::showText(helpEvent->globalPos(), action->toolTip(), m_parent);
                }
            }
            else
            {
                QToolTip::hideText();
            }

            return result;
        }

    private:
        QWidget* m_parent;
    };


    RecentFiles::RecentFiles()
        : QObject()
    {
        m_maxNumRecentFiles = 10;
    }


    void RecentFiles::Init(QMenu* parentMenu, size_t numRecentFiles, const char* subMenuName, const char* configStringName)
    {
        m_configStringName   = configStringName;
        m_maxNumRecentFiles  = numRecentFiles;

        Load();

        m_resetRecentFilesAction = nullptr;
        m_recentFilesMenu = new ToolTipMenu(subMenuName, parentMenu);
        m_recentFilesMenu->setObjectName("EMFX.MainWindow.RecentFilesMenu");
        parentMenu->addMenu(m_recentFilesMenu);

        SetMaxRecentFiles(numRecentFiles);
        UpdateMenu();
    }


    void RecentFiles::OnClearRecentFiles()
    {
        m_recentFiles.clear();

        Save();
        UpdateMenu();
    }


    void RecentFiles::Save()
    {
        QSettings settings(this);
        settings.beginGroup("EMotionFX");
        settings.setValue(m_configStringName, m_recentFiles);
        settings.endGroup();
    }


    void RecentFiles::Load()
    {
        QSettings settings(this);
        settings.beginGroup("EMotionFX");
        m_recentFiles = settings.value(m_configStringName).toStringList();
        settings.endGroup();

        // Remove deleted or non-existing files from the recent files.
        QString filename;
        for (int i = 0; i < m_recentFiles.size(); )
        {
            if (!QFile::exists(m_recentFiles[i]))
            {
                m_recentFiles.removeAt(i);
            }
            else
            {
                i++;
            }
        }

        // Remove legacy duplicates or duplicated filenames with inconsistent casing.
        RemoveDuplicates();
    }


    void RecentFiles::UpdateMenu()
    {
        m_recentFilesMenu->clear();

        AZStd::string sourceFolder = EMotionFX::GetEMotionFX().GetAssetSourceFolder();
        AZStd::string cacheFolder = EMotionFX::GetEMotionFX().GetAssetCacheFolder();
        AzFramework::StringFunc::Path::Normalize(sourceFolder);
        AzFramework::StringFunc::Path::Normalize(cacheFolder);
        AzFramework::StringFunc::Strip(sourceFolder, AZ_CORRECT_FILESYSTEM_SEPARATOR, true, false, true);
        AzFramework::StringFunc::Strip(cacheFolder, AZ_CORRECT_FILESYSTEM_SEPARATOR, true, false, true);

        int recentFilesAdded = 0;
        QString menuItemText;
        AZStd::string folder;
        const int recentFileCount = m_recentFiles.size();
        for (int i = 0; i < recentFileCount; ++i)
        {
            const QFileInfo fileInfo(m_recentFiles[i]);
            folder = fileInfo.absolutePath().toUtf8().data();
            AzFramework::StringFunc::Path::Normalize(folder);
            AzFramework::StringFunc::Strip(folder, AZ_CORRECT_FILESYSTEM_SEPARATOR, true, false, true);

            auto CharacterCompareIgnoreCase = [](const char lhs, const char rhs)
            {
                return tolower(lhs) == tolower(rhs);
            };
            auto PathCompareIgnoreCase = [&CharacterCompareIgnoreCase](const AZ::IO::PathView& lhs, const AZ::IO::PathView& rhs)
            {
                AZStd::string_view lhsStringView = lhs.Native();
                AZStd::string_view rhsStringView = rhs.Native();
                return AZStd::equal(lhsStringView.begin(), lhsStringView.end(), rhsStringView.begin(), rhsStringView.end(),
                    CharacterCompareIgnoreCase);
            };

            AZ::IO::PathView folderPathView(folder);
            AZ::IO::PathView assetSourceView(sourceFolder);
            AZ::IO::PathView assetCacheView(cacheFolder);
            // The source folder is case-sensitive, so use the normal path compare
            auto [folderPathIter, assetSourceIter] = AZStd::mismatch(folderPathView.begin(), folderPathView.end(),
                assetSourceView.begin(), assetSourceView.end());
            // The Cache folder is always lowercase, so compare while ignoring case
            auto [folderPathIter2, assetCacheIter] = AZStd::mismatch(folderPathView.begin(), folderPathView.end(),
                assetCacheView.begin(), assetCacheView.end(), PathCompareIgnoreCase);
            // Both of the above mismatch checks if folder path is a sub-directory of the asset source path or
            // the asset cache path. If either asset source path or asset cache path PathView reaches the end
            // iterator, then the folder path is either equal to one of them or a sub-directory of one of them

            // Skip files that are not part of the current game directory.
            if (sourceFolder == folder
                || assetSourceIter == assetSourceView.end()
                || cacheFolder == folder
                || assetCacheIter == assetCacheView.end())
            {
                menuItemText = QString("&%1 %2").arg(i + 1).arg(fileInfo.fileName());

                QAction* action = new QAction(m_recentFilesMenu);
                action->setText(menuItemText);
                action->setData(m_recentFiles[i]);
                action->setToolTip(m_recentFiles[i]);

                m_recentFilesMenu->addAction(action);
                recentFilesAdded++;

                connect(action, &QAction::triggered, this, &MysticQt::RecentFiles::OnRecentFileSlot);
            }
        }

        if (recentFilesAdded > 0)
        {
            m_recentFilesMenu->addSeparator();
            m_resetRecentFilesAction = m_recentFilesMenu->addAction("Reset Recent Files", this, &RecentFiles::OnClearRecentFiles);
            m_resetRecentFilesAction->setObjectName("EMFX.RecentFiles.ResetRecentFilesAction");
        }
    }


    void RecentFiles::SetMaxRecentFiles(size_t numRecentFiles)
    {
        m_maxNumRecentFiles = numRecentFiles;

        if (m_recentFiles.size() > m_maxNumRecentFiles)
        {
            while (m_recentFiles.size() > m_maxNumRecentFiles)
            {
                m_recentFiles.removeLast();
            }

            Save();
            UpdateMenu();
        }
    }


    AZStd::string RecentFiles::GetLastRecentFileName() const
    {
        if (m_recentFiles.empty())
        {
            return "";
        }

        return m_recentFiles[0].toUtf8().data();
    }


    void RecentFiles::AddRecentFile(AZStd::string filename)
    {
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        // Remove filename duplicates and add the new filename to the top of the recent files.
        if (!filename.empty())
        {
            m_recentFiles.removeAll(filename.c_str());
            m_recentFiles.prepend(filename.c_str());
        }

        // Remove the oldest filenames.
        const int maxRecentFiles = static_cast<int>(m_maxNumRecentFiles);
        while (m_recentFiles.size() > maxRecentFiles)
        {
            m_recentFiles.removeLast();
        }

        Save();
        UpdateMenu();
    }


    void RecentFiles::OnRecentFileSlot()
    {
        QAction* action = qobject_cast<QAction*>(sender());
        if (action == nullptr)
        {
            return;
        }

        emit OnRecentFile(action);
    }


    void RecentFiles::RemoveDuplicates()
    {
        for (int i = 0; i < m_recentFiles.size(); i++)
        {
            for (int j = i + 1; j < m_recentFiles.size(); )
            {
                if (m_recentFiles[i].compare(m_recentFiles[j], Qt::CaseSensitivity::CaseInsensitive) == 0)
                {
                    m_recentFiles.removeAt(j);
                }
                else
                {
                    j++;
                }
            }
        }
    }
} // namespace MysticQt

#include <MysticQt/Source/moc_RecentFiles.cpp>
