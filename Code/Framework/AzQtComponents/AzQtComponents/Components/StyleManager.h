/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/StyleManagerInterface.h>

#include <AzCore/std/containers/unordered_map.h>

#include <AzCore/IO/Path/Path_fwd.h>

#include <QColor>
#include <QFileSystemWatcher>
#include <QHash>
#include <QJsonObject>
#include <QObject>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::StyleManager::m_widgetToStyleSheetMap': class 'QHash<QWidget *,QString>' needs to have dll-interface to be used by clients of class 'AzQtComponents::StyleManager'
#include <QPointer>
#endif
AZ_POP_DISABLE_WARNING

class QApplication;
class QStyle;
class QStyleSheetStyle;
class QWidget;

namespace AzQtComponents
{
    class StyleSheetCache;
    class StylesheetPreprocessor;
    class TitleBarOverdrawHandler;
    class AutoCustomWindowDecorations;

    /**
     * Wrapper around classes dealing with Open 3D Engine style.
     *
     * New applications should work like this:
     *   
     *   int main(int argv, char **argc)
     *   {
     *       QApplication app(argv, argc);
     *       AZ::IO::FixedMaxPath engineRootPath;
     *       {
     *          auto settingsRegistry = AZ::SettingsRegistry::Get();
     *          settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
     *       }
     *
     *       AzQtComponents::StyleManager styleManager(&app);
     *       styleManager.Initialize(&app, engineRootPath);
     *       ...
     *       
     *   }
     *
     */
    class AZ_QT_COMPONENTS_API StyleManager
        : public QObject
        , public AzQtComponents::StyleManagerInterface
    {
        Q_OBJECT

        static StyleManager* s_instance;

    public:
        explicit StyleManager(QObject* parent);
        ~StyleManager() override;

        // StyleManagerInterface overrides ...
        bool IsStylePropertyDefined(const char* propertyKey) const override;
        QString GetStylePropertyAsString(const char* propertyKey) const override;
        int GetStylePropertyAsInteger(const char* propertyKey) const override;
        QColor GetStylePropertyAsColor(const char* propertyKey) const override;

        static bool isInstanced() { return s_instance; }

        static void addSearchPaths(const QString& searchPrefix, const QString& pathOnDisk, const QString& qrcPrefix,
            const AZ::IO::PathView& engineRootPath);

        static bool setStyleSheet(QWidget* widget, QString styleFileName);

        static QStyleSheetStyle* styleSheetStyle(const QWidget* widget);
        static QStyle* baseStyle(const QWidget* widget);

        static void repolishStyleSheet(QWidget* widget);

        /*!
        * Call to initialize the StyleManager, allowing it to hook into the application and apply the global style
        * The AzQtComponents does not hook in the AZ::Environment, so the Settings Registry isn't available
        * Therefore the engine root path must be supplied via function arguments
        */
        void initialize(QApplication* application, const AZ::IO::PathView& engineRootPath);
        void InitializeThemeProperties(QFileSystemWatcher* watcher, const QString& filePath);

        /*!
        * Call this to force a refresh of the global stylesheet and a reload of any settings files.
        * Note that you should never need to do this manually.
        */
        void refresh();
        // deprecated; introduced before the new camelCase Qt based method names were adopted.
        void Refresh() { refresh(); }

    private Q_SLOTS:
        void cleanupStyles();
        void stopTrackingWidget(QObject* object);

    private:
        void initializeFonts();
        void initializeSearchPaths(QApplication* application, const AZ::IO::PathView& engineRootPath);

        void LoadThemeProperties(const QString& jsonString);
        void LoadThemePropertiesRecursively(const QString prefix, const QJsonObject& jsonObject);

        void resetWidgetSheets();

        StylesheetPreprocessor* m_stylesheetPreprocessor = nullptr;
        StyleSheetCache* m_stylesheetCache = nullptr;
        TitleBarOverdrawHandler* m_titleBarOverdrawHandler = nullptr;

        using WidgetToStyleSheetMap = QHash<QWidget*, QString>;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::StyleManager::m_widgetToStyleSheetMap': class 'QHash<QWidget *,QString>' needs to have dll-interface to be used by clients of class 'AzQtComponents::StyleManager'
        WidgetToStyleSheetMap m_widgetToStyleSheetMap;
        QStyleSheetStyle* m_styleSheetStyle = nullptr;

        // Track the style as a QPointer, as the QApplication will delete it if it still has a pointer to it
        QPointer<QStyle> m_style;

        QHash<QString, QString> m_themeProperties;
        AZ_POP_DISABLE_WARNING

        AutoCustomWindowDecorations* m_autoCustomWindowDecorations = nullptr;

    };
} // namespace AzQtComponents

