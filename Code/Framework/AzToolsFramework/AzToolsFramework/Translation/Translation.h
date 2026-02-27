/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QApplication>
#include <QTranslator>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>

namespace AzToolsFramework
{
    // Load a .qm translation file from the Translations directory and install it on qApp.
    // @param filename Relative path under @engroot@/Assets/Editor/Translations/ (e.g., "zh_CN/Editor_zh_CN.qm")
    // @return The installed QTranslator on success, or nullptr on failure.
    inline QTranslator* LoadTranslator(const QString& filename)
    {
        AZStd::array<char, AZ::IO::MaxPathLength> unresolvedPath;
        QString dirpath = "@engroot@/Assets/Editor/Translations/" + filename;
        QByteArray dirpathUtf8 = dirpath.toUtf8();
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(dirpathUtf8.constData(), unresolvedPath.data(), unresolvedPath.size());
        QString translationFilePath(unresolvedPath.data());

        QTranslator *translator = new QTranslator();
        if (translator->load(translationFilePath))
        {
            if (qApp->installTranslator(translator))
            {
                return translator;
            }
            AZ_Warning("Translation", false, "Error installing translation %s!", unresolvedPath.data());
        }
        else
        {
            AZ_Warning("Translation", false, "Error loading translation file %s", unresolvedPath.data());
        }

        // Failed to load or install - clean up and return nullptr
        delete translator;
        return nullptr;
    }

    // Remove and delete a QTranslator.
    inline void UnloadTranslator(QTranslator* translator)
    {
        if (translator)
        {
            qApp->removeTranslator(translator);
            delete translator;
        }
    }

    // Load a translator for a specific module and language code.
    // @param modulename Module name (e.g., "Editor", "ScriptCanvas")
    // @param languageCode Language code (e.g., "zh_CN")
    // @return Loaded and installed QTranslator, or nullptr for English / on failure
    inline QTranslator* CreateAndLoadTranslatorForLanguage(const QString& modulename, const QString& languageCode)
    {
        // Don't load translation for English (source language)
        if (languageCode == "en_US" || languageCode.isEmpty())
        {
            return nullptr;
        }

        QString filename = languageCode + "/" + modulename + "_" + languageCode + ".qm";
        return LoadTranslator(filename);
    }
} // namespace AzToolsFramework
