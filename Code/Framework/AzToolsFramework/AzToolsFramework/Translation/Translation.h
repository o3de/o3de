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
#include <QSettings>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
 
namespace AzToolsFramework
{
    enum class Language
    {
        English,
        Chinese
    };
 
    inline QTranslator* LoadTranslator(const QString& filename)
    {
        AZStd::array<char, AZ::IO::MaxPathLength> unresolvedPath;
        QString dirpath = "@engroot@/Assets/Editor/Translations/" + filename;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(dirpath.toStdString().c_str(), unresolvedPath.data(), unresolvedPath.size());
        QString translationFilePath(unresolvedPath.data());
 
        QTranslator *translator = new QTranslator();
        if (translator->load(translationFilePath))
        {
            if ( !qApp->installTranslator(translator) )
            {
                AZ_Warning("Translation", false, "Error installing translation %s!", unresolvedPath.data());
            }
        }
        else
        {
            AZ_Warning("Translation", false, "Error loading translation file %s", unresolvedPath.data());
        }
        
        return translator;
    }
 
    inline void UnloadTranslator(QTranslator* translator)
    {
        qApp->removeTranslator(translator);
        delete translator;
        translator = nullptr;
    }
 
    inline Language GetLanguage()
    {
        QSettings settings("O3DE", "O3DE Editor");
        int language = settings.value("Settings/Language").toInt();
        return static_cast<Language>(language);
    }
 
    inline QTranslator* CreateAndLoadTranslator(const QString& modulename)
    {
        QString filename;
        switch (AzToolsFramework::GetLanguage())
        {
        case AzToolsFramework::Language::Chinese:
            filename = "zh_CN/" + modulename + "_zh_CN.qm";
            break;
        default:
            break;
        }
 
        return LoadTranslator(filename);        
    }
} // namespace AzToolsFramework
