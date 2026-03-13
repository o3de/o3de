/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QAbstractNativeEventFilter>
#include <QColor>
#include <QMap>
#include <QTranslator>
#include <QSet>
#include <unordered_map>

#include <AzCore/PlatformDef.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <IEditor.h>
#include <AzQtComponents/Application/AzQtApplication.h>

class QFileInfo;
typedef QList<QFileInfo> QFileInfoList;
class QByteArray;

namespace AzQtComponents
{
    class O3DEStylesheet;
}

enum EEditorNotifyEvent;

// This contains the main "QApplication"-derived class for the editor itself.
// it performs the message hooking and other functions to allow CryEdit to function.

namespace Editor
{
    typedef void(* ScanDirectoriesUpdateCallBack)(void);
    /*!
      \param directoryList A list of directories to search. ScanDirectories will also search the subdirectories of each of these.
      \param filters A list of filename filters. Supports * and ? wildcards. The filters will not apply to the directory names.
      \param files Any file that is found and matches any of the filters will be added to files.
      \param updateCallback An optional callback function that will be called once for every directory and subdirectory that is scanned.
    */
    void ScanDirectories(QFileInfoList& directoryList, const QStringList& filters, QFileInfoList& files, ScanDirectoriesUpdateCallBack updateCallback = nullptr);

    class EditorQtApplication
        : public AzQtComponents::AzQtApplication
        , public QAbstractNativeEventFilter
        , public IEditorNotifyListener
        , public AZ::UserSettingsOwnerRequestBus::Handler
    {
        Q_OBJECT
        Q_PROPERTY(QSet<int> pressedKeys READ pressedKeys)
        Q_PROPERTY(int pressedMouseButtons READ pressedMouseButtons)
    public:
        // Call this before creating this object:
        static void InstallQtLogHandler();

        EditorQtApplication(int& argc, char** argv);
        virtual ~EditorQtApplication();
        void Initialize();

        void LoadSettings();
        void UnloadSettings();

        // AZ::UserSettingsOwnerRequestBus::Handler
        void SaveSettings() override;
        ////

        static EditorQtApplication* instance();
        static EditorQtApplication* newInstance(int& argc, char** argv);

        static bool IsActive();

        bool isMovingOrResizing() const;

        // IEditorNotifyListener:
        void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

        const QColor& GetColorByName(const QString& colorName);

        void EnableOnIdle(bool enable = true);
        bool OnIdleEnabled() const;

        bool eventFilter(QObject* object, QEvent* event) override;

        QSet<int> pressedKeys() const { return m_pressedKeys; }
        int pressedMouseButtons() const { return m_pressedButtons; }

        // Internationalization (i18n) API
        //! Get the current editor language (e.g., "en_US", "zh_CN")
        AZStd::string GetCurrentLanguage() const;

        //! Set the editor language at runtime and reload translators
        //! @param languageCode Language code (e.g., "en_US", "zh_CN", "ja_JP")
        //! @return true if language was successfully changed
        bool SetLanguage(const AZStd::string& languageCode);

        //! Get list of available languages by scanning translation files
        AZStd::vector<AZStd::string> GetAvailableLanguages() const;

        //! Register a translator module (allows Gems to add translations)
        //! @param moduleName Name of the module (e.g., "MyGem")
        //! @return true if successfully registered
        bool RegisterTranslatorModule(const AZStd::string& moduleName);

    public Q_SLOTS:

        void setIsMovingOrResizing(bool isMovingOrResizing);

    signals:
        void skinChanged();
        void languageChanged(const AZStd::string& languageCode);

    protected:

        bool m_isMovingOrResizing = false;

    private:
        static QColor InterpolateColors(QColor a, QColor b, float factor);
        void RefreshStyleSheet();
        void InstallFilters();
        void UninstallFilters();
        void maybeProcessIdle();

        AzQtComponents::O3DEStylesheet* m_stylesheet;

        // Translators
        void InstallEditorTranslators();
        void UninstallEditorTranslators();

        // i18n helper methods
        void InstallTranslatorWithFallback(const AZStd::string& moduleName, const AZStd::string& language);
        AZStd::vector<AZStd::string> BuildLanguageFallbackChain(const AZStd::string& language) const;
        AZStd::vector<AZStd::string> GetTranslatorModules() const;
        AZStd::string GetSavedLanguage() const;
        bool SaveLanguage(const AZStd::string& language);

        QVector<QTranslator*> m_translators;
        AZStd::string m_currentLanguage;
        AZStd::vector<AZStd::string> m_registeredModules;

        AZ::UserSettingsProvider m_localUserSettings;

        Qt::MouseButtons m_pressedButtons = Qt::NoButton;
        QSet<int> m_pressedKeys;

        bool m_activatedLocalUserSettings = false;
        bool m_applicationActive = false;
    };
} // namespace editor
