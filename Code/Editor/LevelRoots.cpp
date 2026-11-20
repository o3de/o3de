/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "LevelRoots.h"

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Gem/GemInfo.h>

#include <QDir>
#include <QFileInfo>

#include <algorithm>

namespace LevelRoots
{
    //======================================================================
    // Constants
    //======================================================================
    static constexpr const char* kLevelsFolderName = "Levels";

    //======================================================================
    // Internal helpers
    //======================================================================
    static QString MakeProjectLevelsPath()
    {
        const QString projectRoot = QString::fromUtf8(Path::GetEditingGameDataFolder().c_str());
        return QDir::cleanPath(QStringLiteral("%1/%2").arg(projectRoot, kLevelsFolderName));
    }

    static bool DirectoryExists(const QString& path)
    {
        return QFileInfo(path).isDir();
    }

    //======================================================================
    // Public API
    //======================================================================
    QVector<Root> Enumerate(Mode mode)
    {
        QVector<Root> roots;

        //--- Project root (always listed) ---
        Root projectRoot;
        projectRoot.displayName = QStringLiteral("Levels");
        projectRoot.absolutePath = MakeProjectLevelsPath();
        projectRoot.isProject = true;
        roots.push_back(projectRoot);

        //--- Active gems ---
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (!settingsRegistry)
        {
            return roots;
        }

        AZStd::vector<AzFramework::GemInfo> gemInfoList;
        if (!AzFramework::GetGemsInfo(gemInfoList, *settingsRegistry))
        {
            return roots;
        }

        for (const AzFramework::GemInfo& gem : gemInfoList)
        {
            for (const AZ::IO::Path& sourcePath : gem.m_absoluteSourcePaths)
            {
                AZ::IO::Path candidate = sourcePath;
                candidate /= AzFramework::GemInfo::GetGemAssetFolder();
                candidate /= kLevelsFolderName;

                const QString candidateQt = QString::fromUtf8(candidate.c_str());
                // ExistingOnly drops gems whose Assets/Levels folder is
                // missing; AllActive keeps every active gem so the user
                // can create the very first level there.
                if (mode == Mode::ExistingOnly && !DirectoryExists(candidateQt))
                {
                    continue;
                }

                Root gemRoot;
                gemRoot.displayName = QStringLiteral("%1/%2")
                                          .arg(QString::fromUtf8(gem.m_gemName.c_str()))
                                          .arg(kLevelsFolderName);
                gemRoot.absolutePath = QDir::cleanPath(candidateQt);
                gemRoot.isProject = false;

                // De-dupe in the unusual case where two source paths point to the same folder.
                const auto exists = std::any_of(roots.begin(), roots.end(), [&](const Root& r) {
                    return r.absolutePath.compare(gemRoot.absolutePath, Qt::CaseInsensitive) == 0;
                });
                if (!exists)
                {
                    roots.push_back(gemRoot);
                }
            }
        }

        // Alphabetize gem entries by display name. The project root stays
        // at index 0 so it is always the default option in any combo or
        // tree fed by this list.
        if (roots.size() > 1)
        {
            std::sort(roots.begin() + 1, roots.end(), [](const Root& a, const Root& b) {
                return a.displayName.localeAwareCompare(b.displayName) < 0;
            });
        }

        return roots;
    }

    //----------------------------------------------------------------------
    // Returns true if "child" sits anywhere under "parent". Shared by the
    // path-membership and display-formatting helpers.
    //----------------------------------------------------------------------
    static bool IsContainedIn(const QString& child, const QString& parent)
    {
        if (parent.isEmpty())
        {
            return false;
        }
        const QDir parentDir(parent);
        QDir cursor(child); // initially points at the file itself
        while (cursor.cdUp())
        {
            if (cursor == parentDir)
            {
                return true;
            }
        }
        return false;
    }

    QString FormatRelativeDisplay(const QString& absolutePath)
    {
        if (absolutePath.isEmpty())
        {
            return absolutePath;
        }

        const QString cleaned = QDir::cleanPath(absolutePath);

        //--- Project-rooted: strip the project prefix (legacy form). ---
        const QString projectRoot = QDir::cleanPath(
            QString::fromUtf8(Path::GetEditingGameDataFolder().c_str()));
        if (!projectRoot.isEmpty() && IsContainedIn(cleaned, projectRoot))
        {
            return QDir::toNativeSeparators(QDir(projectRoot).relativeFilePath(cleaned));
        }

        //--- Gem-rooted: prefix with the gem's display name. ---
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            AZStd::vector<AzFramework::GemInfo> gemInfoList;
            if (AzFramework::GetGemsInfo(gemInfoList, *settingsRegistry))
            {
                for (const AzFramework::GemInfo& gem : gemInfoList)
                {
                    for (const AZ::IO::Path& sourcePath : gem.m_absoluteSourcePaths)
                    {
                        const QString gemSource = QDir::cleanPath(QString::fromUtf8(sourcePath.c_str()));
                        if (!IsContainedIn(cleaned, gemSource))
                        {
                            continue;
                        }

                        QString relativeUnderGem = QDir(gemSource).relativeFilePath(cleaned);

                        // Drop a leading "Assets/" so the on-disk layout is
                        // hidden in the UI and gem levels read as
                        // "<GemName>/Levels/...".
                        const QString assetsPrefix =
                            QString::fromUtf8(AzFramework::GemInfo::GetGemAssetFolder()) + QStringLiteral("/");
                        if (relativeUnderGem.startsWith(assetsPrefix, Qt::CaseInsensitive))
                        {
                            relativeUnderGem.remove(0, assetsPrefix.size());
                        }

                        const QString gemName = QString::fromUtf8(gem.m_gemName.c_str());
                        return QDir::toNativeSeparators(QStringLiteral("%1/%2").arg(gemName, relativeUnderGem));
                    }
                }
            }
        }

        //--- Fallback: return the input as-is, with native separators. ---
        return QDir::toNativeSeparators(cleaned);
    }

    bool IsPathUnderActiveSource(const QString& absolutePath)
    {
        if (absolutePath.isEmpty())
        {
            return false;
        }

        const QString normalized = QDir::cleanPath(absolutePath);

        //--- Project root (whole project tree, not just Levels/) ---
        const QString projectRoot = QString::fromUtf8(Path::GetEditingGameDataFolder().c_str());
        if (IsContainedIn(normalized, projectRoot))
        {
            return true;
        }

        //--- Every active gem's full source tree ---
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (!settingsRegistry)
        {
            return false;
        }

        AZStd::vector<AzFramework::GemInfo> gemInfoList;
        if (!AzFramework::GetGemsInfo(gemInfoList, *settingsRegistry))
        {
            return false;
        }

        for (const AzFramework::GemInfo& gem : gemInfoList)
        {
            for (const AZ::IO::Path& sourcePath : gem.m_absoluteSourcePaths)
            {
                if (IsContainedIn(normalized, QString::fromUtf8(sourcePath.c_str())))
                {
                    return true;
                }
            }
        }

        return false;
    }
}
