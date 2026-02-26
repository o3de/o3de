/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/string/string.h>

#include <QString>
#include <QStringList>
#include <QDir>
#include <QLocale>
#include <QTranslator>
#include <QCoreApplication>
#include <QDebug>
#include <QLibraryInfo>

namespace AzToolsFramework
{
    /**
     * @brief Centralized Translation Manager for Cross-Process Language Synchronization
     *
     * Provides a unified way for all O3DE tools (Editor, ProjectManager, AssetProcessor, etc.)
     * to access and manage translations with a shared language setting.
     *
     * ARCHITECTURE:
     * - Each tool is a SEPARATE PROCESS (independent .exe files)
     * - Language preference is stored in Settings Registry (file-based, persistent)
     * - Settings Registry is shared across all processes via the filesystem
     * - When Editor changes language → saved to Settings Registry file
     * - When tools start → read language from Settings Registry file
     *
     * CROSS-PROCESS FLOW:
     * 1. User sets language in Editor Preferences
     * 2. Editor saves to: {UserHome}/.o3de/Registry/user.setreg
     * 3. ProjectManager/AssetProcessor/etc. read from same file on startup
     * 4. All tools automatically use the same language
     *
     * This ensures all tools use the same language preference without requiring IPC.
     */
    class TranslationManager
    {
    public:
        // Settings Registry key for storing language preference (cross-process)
        // This is persisted to disk in user.setreg
        static constexpr const char* LanguageSettingKey = "/O3DE/Editor/Language";
        static constexpr const char* DefaultLanguage = "en_US";

        /**
         * @brief Get the current language setting
         * @return Language code (e.g., "zh_CN", "en_US")
         *
         * Reads from Settings Registry if available, otherwise uses system locale.
         */
        static AZStd::string GetCurrentLanguage()
        {
            AZStd::string savedLanguage;

            // Try to load from Settings Registry (shared between all tools)
            if (auto registry = AZ::SettingsRegistry::Get())
            {
                registry->Get(savedLanguage, LanguageSettingKey);
            }

            // If no saved language, use system locale
            if (savedLanguage.empty())
            {
                QString systemLocale = QLocale::system().name();
                return qPrintable(systemLocale);
            }

            return savedLanguage;
        }

        /**
         * @brief Set the language preference
         * @param languageCode Language code to set (e.g., "zh_CN")
         * @return true if successfully saved
         *
         * Saves to Settings Registry, making it available to all tools.
         */
        static bool SetLanguage(const QString& languageCode)
        {
            if (auto registry = AZ::SettingsRegistry::Get())
            {
                return registry->Set(LanguageSettingKey, languageCode.toUtf8().constData());
            }
            return false;
        }

        /**
         * @brief Load translator for a module with the current language
         * @param moduleName Name of the module (e.g., "ProjectManager")
         * @param app QCoreApplication instance to install translator on
         * @return Loaded translator or nullptr
         *
         * Automatically uses the current language setting and handles fallback.
         */
        static QTranslator* LoadModuleTranslator(const QString& moduleName, QCoreApplication* app = nullptr)
        {
            QString language = GetCurrentLanguage().c_str();
            return LoadModuleTranslatorForLanguage(moduleName, language, app);
        }

        /**
         * @brief Load translator for a specific language
         * @param moduleName Name of the module
         * @param languageCode Language code
         * @param app QCoreApplication instance to install translator on
         * @return Loaded translator or nullptr
         */
        static QTranslator* LoadModuleTranslatorForLanguage(
            const QString& moduleName,
            const QString& languageCode,
            QCoreApplication* app = nullptr)
        {
            // Don't load for English (source language)
            if (languageCode == "en_US" || languageCode.isEmpty())
            {
                return nullptr;
            }

            // Try to load translation file
            // Path format: {engroot}/Assets/Editor/Translations/{language}/{module}_{language}.qm
            QTranslator* translator = new QTranslator();

            QString resolvedPath;
            auto* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (fileIO)
            {
                // Use FileIOBase to resolve the @engroot@ alias
                AZStd::array<char, AZ::IO::MaxPathLength> resolvedBuffer;
                QString translationPath = QString("@engroot@/Assets/Editor/Translations/%1/%2_%1.qm")
                    .arg(languageCode, moduleName);

                QByteArray translationPathUtf8 = translationPath.toUtf8();
                if (fileIO->ResolvePath(
                    translationPathUtf8.constData(),
                    resolvedBuffer.data(),
                    resolvedBuffer.size()))
                {
                    resolvedPath = QString(resolvedBuffer.data());
                }
            }

            // Fallback: resolve engine root from SettingsRegistry when FileIOBase is not available
            if (resolvedPath.isEmpty())
            {
                if (auto registry = AZ::SettingsRegistry::Get())
                {
                    AZ::IO::FixedMaxPath engineRootPath;
                    if (registry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
                    {
                        resolvedPath = QString::fromUtf8(engineRootPath.c_str(), static_cast<int>(engineRootPath.Native().size()))
                            + QString("/Assets/Editor/Translations/%1/%2_%1.qm").arg(languageCode, moduleName);
                    }
                }
            }

            if (translator->load(resolvedPath))
            {
                // Install on the application if provided
                if (app)
                {
                    app->installTranslator(translator);
                }
                else if (qApp)
                {
                    qApp->installTranslator(translator);
                }

                return translator;
            }
            else
            {
                delete translator;
                return nullptr;
            }
        }

        /**
         * @brief Initialize translations for a tool
         * @param toolName Name of the tool (e.g., "ProjectManager", "AssetProcessor")
         * @param app QCoreApplication instance
         * @return Loaded translator or nullptr
         *
         * Convenience function that:
         * 1. Gets the current language
         * 2. Loads the translator
         * 3. Installs it on the application
         *
         * Usage example:
         * @code
         * int main(int argc, char** argv)
         * {
         *     QApplication app(argc, argv);
         *
         *     // Initialize translations
         *     AzToolsFramework::TranslationManager::InitializeToolTranslations("ProjectManager", &app);
         *
         *     // ... rest of application ...
         * }
         * @endcode
         */
        static QTranslator* InitializeToolTranslations(const QString& toolName, QCoreApplication* app)
        {
            QString language = GetCurrentLanguage().c_str();

            // Log the language being used
            qDebug() << "[i18n]" << toolName << "using language:" << language;

            // Load Qt's base translations first (for standard button text: OK, Cancel, Save, etc.)
            // This ensures standard Qt widget text is translated regardless of O3DE module translations.
            LoadQtBaseTranslations(language, app);

            QTranslator* translator = LoadModuleTranslatorForLanguage(toolName, language, app);

            if (translator)
            {
                qDebug() << "[i18n] Loaded translation for" << toolName << "(" << language << ")";
            }
            else if (language != "en_US")
            {
                qDebug() << "[i18n] No translation file found for" << toolName << "(" << language << "), using English";
            }

            return translator;
        }

        /**
         * @brief Get list of available languages by scanning the translations directory.
         * @return List of language codes (always includes "en_US" as the source language)
         *
         * Dynamically discovers languages by looking for subdirectories under
         * {engroot}/Assets/Editor/Translations/ that contain .qm files.
         * This ensures newly added languages are automatically available
         * without code changes.
         */
        static QStringList GetAvailableLanguages()
        {
            QStringList languages;
            // English is always available as the source language
            languages << "en_US";

            // Resolve the translations root directory
            QString translationsRoot;
            auto* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (fileIO)
            {
                AZStd::array<char, AZ::IO::MaxPathLength> resolvedBuffer;
                if (fileIO->ResolvePath("@engroot@/Assets/Editor/Translations",
                    resolvedBuffer.data(), resolvedBuffer.size()))
                {
                    translationsRoot = QString(resolvedBuffer.data());
                }
            }

            // Fallback: resolve engine root from SettingsRegistry
            if (translationsRoot.isEmpty())
            {
                if (auto registry = AZ::SettingsRegistry::Get())
                {
                    AZ::IO::FixedMaxPath engineRootPath;
                    if (registry->Get(engineRootPath.Native(),
                        AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
                    {
                        translationsRoot = QString::fromUtf8(
                            engineRootPath.c_str(),
                            static_cast<int>(engineRootPath.Native().size()))
                            + "/Assets/Editor/Translations";
                    }
                }
            }

            // Scan for subdirectories that contain .qm files
            if (!translationsRoot.isEmpty())
            {
                QDir rootDir(translationsRoot);
                if (rootDir.exists())
                {
                    const QStringList subdirs = rootDir.entryList(
                        QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
                    for (const QString& subdir : subdirs)
                    {
                        // Skip English directories (already added above)
                        if (subdir == "en_US")
                        {
                            continue;
                        }

                        // Check if the directory contains at least one .qm file
                        QDir langDir(rootDir.filePath(subdir));
                        QStringList qmFiles = langDir.entryList(
                            QStringList() << "*.qm", QDir::Files);
                        if (!qmFiles.isEmpty())
                        {
                            languages << subdir;
                        }
                    }
                }
            }

            return languages;
        }

        /**
         * @brief Load Qt's base translations for standard widget text
         * @param languageCode Language code (e.g., "zh_CN")
         * @param app QCoreApplication instance to install translators on
         * @return List of loaded translators (caller takes ownership)
         *
         * Qt standard buttons (OK, Cancel, Save, Discard, Yes, No, Close, etc.)
         * get their text from Qt's own translation files (qtbase_*.qm, qt_*.qm).
         * Without loading these files, standard buttons remain in English even
         * when the application has translations installed.
         *
         * This method loads Qt's base translation files from Qt's installation
         * directory (QLibraryInfo::TranslationsPath).
         */
        static QList<QTranslator*> LoadQtBaseTranslations(
            const QString& languageCode,
            QCoreApplication* app = nullptr)
        {
            QList<QTranslator*> translators;

            // Don't load for English (source language)
            if (languageCode == "en_US" || languageCode.isEmpty())
            {
                return translators;
            }

            // Get Qt's translations directory
            #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                const QString qtTranslationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
            #else
                const QString qtTranslationsPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
            #endif

            // Qt base translation files to load:
            // - "qtbase": Contains standard widget translations (QMessageBox buttons, QDialogButtonBox, etc.)
            //   This covers: OK, Cancel, Save, Discard, Yes, No, Close, Apply, Reset, etc.
            // - "qt": Contains additional Qt module translations
            const QStringList qtTranslationBases = { "qtbase", "qt" };

            // Build language fallback: "zh_CN" -> try "zh_CN" first, then "zh"
            QStringList languagesToTry;
            languagesToTry << languageCode;
            if (languageCode.contains('_'))
            {
                languagesToTry << languageCode.split('_').first();
            }

            for (const QString& baseName : qtTranslationBases)
            {
                bool loaded = false;
                for (const QString& lang : languagesToTry)
                {
                    QTranslator* translator = new QTranslator();
                    if (translator->load(baseName + "_" + lang, qtTranslationsPath))
                    {
                        if (app)
                        {
                            app->installTranslator(translator);
                        }
                        else if (qApp)
                        {
                            qApp->installTranslator(translator);
                        }

                        translators.append(translator);
                        qDebug() << "[i18n] Loaded Qt base translation:" << baseName + "_" + lang
                                 << "from" << qtTranslationsPath;
                        loaded = true;
                        break; // Successfully loaded, move to next base name
                    }
                    else
                    {
                        delete translator;
                    }
                }

                if (!loaded)
                {
                    qDebug() << "[i18n] Qt base translation not found:" << baseName + "_" + languageCode
                             << "in" << qtTranslationsPath;
                }
            }

            return translators;
        }

        /**
         * @brief Get friendly name for a language code
         * @param languageCode Language code
         * @return Friendly name (e.g., "简体中文" for "zh_CN")
         */
        static QString GetLanguageName(const QString& languageCode)
        {
            static QMap<QString, QString> languageNames = {
                {"en_US", "English (US)"},
                {"zh_CN", "简体中文"}
            };

            return languageNames.value(languageCode, languageCode);
        }
    };

} // namespace AzToolsFramework

