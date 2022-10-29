/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QApplication>
#include <QDebug>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QFileInfo::d_ptr': class 'QSharedDataPointer<QFileInfoPrivate>' needs to have dll-interface to be used by clients of class 'QFileInfo'
#include <QDir>
AZ_POP_DISABLE_WARNING
#include <QFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QStyleFactory>
#include <QPalette>
#include <QPointer>
#include <QString>
#include <QStyle>
#include <QTextStream>
#include <QWidget>

#include <QtWidgets/private/qstylesheetstyle_p.h>

#include <AzQtComponents/Components/StylesheetPreprocessor.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzQtComponents/Components/StyleSheetCache.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>
#include <AzQtComponents/Components/AutoCustomWindowDecorations.h>

namespace AzQtComponents
{
    constexpr AZStd::string_view StylesheetVariablesKey = "theme_properties";

    constexpr QStringView g_styleSheetRelativePath {u"Code/Framework/AzQtComponents/AzQtComponents/Components/Widgets"};
    constexpr QStringView g_styleSheetResourcePath {u":AzQtComponents/Widgets"};
    constexpr QStringView g_globalStyleSheetName {u"BaseStyleSheet.qss"};
    constexpr QStringView g_searchPathPrefix {u"AzQtComponentWidgets"};

    StyleManager* StyleManager::s_instance = nullptr;

    static QStyle* createBaseStyle()
    {
        return QStyleFactory::create("Fusion");
    }

    bool StyleManager::IsStylePropertyDefined(const char* propertyKey) const
    {
        return (m_themeProperties.contains(propertyKey));
    };

    QString StyleManager::GetStylePropertyAsString(const char* propertyKey) const
    {
        if (m_themeProperties.contains(propertyKey))
        {
            return m_themeProperties.value(propertyKey);
        }

        return QString();
    };

    int StyleManager::GetStylePropertyAsInteger(const char* propertyKey) const
    {
        if (m_themeProperties.contains(propertyKey))
        {
            return m_themeProperties.value(propertyKey).toInt();
        }

        return int();
    };

    QColor StyleManager::GetStylePropertyAsColor(const char* propertyKey) const
    {
        if (m_themeProperties.contains(propertyKey))
        {
            return QColor(m_themeProperties.value(propertyKey));
        }

        return QColor();
    };

    void StyleManager::addSearchPaths(const QString& searchPrefix, const QString& pathOnDisk, const QString& qrcPrefix,
        const AZ::IO::PathView& engineRootPath)
    {
        if (!s_instance)
        {
            qFatal("StyleManager::addSearchPaths called before instance was created");
            return;
        }

        s_instance->m_stylesheetCache->addSearchPaths(searchPrefix, pathOnDisk, qrcPrefix, engineRootPath);
    }

    bool StyleManager::setStyleSheet(QWidget* widget, QString styleFileName)
    {
        if (!s_instance)
        {
            qFatal("StyleManager::setStyleSheet called before instance was created");
            return false;
        }

        if (!widget)
        {
            qFatal("StyleManager::setStyleSheet called with null widget pointer");
            return false;
        }

        if (!styleFileName.endsWith(StyleSheetCache::styleSheetExtension()))
        {
            styleFileName.append(StyleSheetCache::styleSheetExtension());
        }

        const auto styleSheet = s_instance->m_stylesheetCache->loadStyleSheet(styleFileName);
        if (styleSheet.isEmpty())
        {
            return false;
        }

        s_instance->m_widgetToStyleSheetMap.insert(widget, styleFileName);

        connect(widget, &QObject::destroyed, s_instance, &StyleManager::stopTrackingWidget, Qt::UniqueConnection);

        widget->setStyleSheet(s_instance->m_stylesheetPreprocessor->ProcessStyleSheet(styleSheet));

        return true;
    }

    QStyleSheetStyle* StyleManager::styleSheetStyle(const QWidget* widget)
    {
        Q_UNUSED(widget);
        // widget is currently unused, but would be required if Qt::AA_ManualStyleSheetStyle was
        // not set.

        if (!s_instance)
        {
            AZ_Warning("StyleManager", false, "StyleManager::styleSheetStyle called before instance was created");
            return nullptr;
        }

        if (!QApplication::testAttribute(Qt::AA_ManualStyleSheetStyle))
        {
            qFatal("StyleManager::styleSheetStyle has not been implemented for automatically created QStyleSheetStyles");
            return nullptr;
        }

        return s_instance->m_styleSheetStyle;
    }

    QStyle *StyleManager::baseStyle(const QWidget *widget)
    {
        const auto sss = styleSheetStyle(widget);
        return sss ? sss->baseStyle() : nullptr;
    }

    void StyleManager::repolishStyleSheet(QWidget* widget)
    {
        StyleManager::styleSheetStyle(widget)->repolish(widget);
    }

    StyleManager::StyleManager(QObject* parent)
        : QObject(parent)
        , m_stylesheetPreprocessor(new StylesheetPreprocessor(this))
        , m_stylesheetCache(new StyleSheetCache(this))
    {
        if (s_instance)
        {
            qFatal("A StyleManager already exists");
        }
    }

    StyleManager::~StyleManager()
    {
        AZ::Interface<StyleManagerInterface>::Unregister(this);

        delete m_stylesheetPreprocessor;
        s_instance = nullptr;

        if (m_style)
        {
            delete m_style.data();
            m_style.clear();
            m_styleSheetStyle = nullptr;
        }
    }

    void StyleManager::InitializeThemeProperties([[maybe_unused]] QFileSystemWatcher* watcher, const QString& filePath)
    {
        if (QFile::exists(filePath))
        {
            QFile themeFile;
            themeFile.setFileName(filePath);
            if (themeFile.open(QFile::ReadOnly))
            {
                QString loadedThemeFile = themeFile.readAll();
                LoadThemeProperties(loadedThemeFile);
            }
        }
    }

    void StyleManager::initialize([[maybe_unused]] QApplication* application, const AZ::IO::PathView& engineRootPath)
    {
        if (s_instance)
        {
            qFatal("StyleManager::Initialize called more than once");
            return;
        }
        s_instance = this;

        QApplication::setAttribute(Qt::AA_ManualStyleSheetStyle, true);
        QApplication::setAttribute(Qt::AA_PropagateStyleToChildren, true);

        connect(application, &QCoreApplication::aboutToQuit, this, &StyleManager::cleanupStyles);

        initializeSearchPaths(application, engineRootPath);
        initializeFonts();

        QFont defaultFont("Open Sans");
        defaultFont.setPixelSize(12);
        QApplication::setFont(defaultFont);

        AZ::Interface<StyleManagerInterface>::Register(this);
        m_stylesheetPreprocessor->Initialize();

        m_titleBarOverdrawHandler = TitleBarOverdrawHandler::createHandler(application, this);

        // The window decoration wrappers require the titlebar overdraw handler
        // so we can't initialize the custom window decoration monitor until the
        // titlebar overdraw handler has been initialized.
        m_autoCustomWindowDecorations = new AutoCustomWindowDecorations(this);
        m_autoCustomWindowDecorations->setMode(AutoCustomWindowDecorations::Mode_AnyWindow);

        InitializeThemeProperties(nullptr, "D:\\GitRepos\\O3DE\\o3de\\Code\\Framework\\AzQtComponents\\AzQtComponents\\Themes\\O3DE_Light\\o3de_light.json");

        // Style is chained as: Style -> QStyleSheetStyle -> native, meaning any CSS limitation can be tackled in Style.cpp
        m_styleSheetStyle = new QStyleSheetStyle(createBaseStyle());
        m_style = new Style(m_styleSheetStyle);

        QApplication::setStyle(m_style);
        m_style->setParent(this);
        refresh();

        connect(m_stylesheetCache, &StyleSheetCache::styleSheetsChanged, this, [this]
        {
            refresh();
        });
    }

    void StyleManager::cleanupStyles()
    {
        QApplication::setStyle(createBaseStyle());
    }

    void StyleManager::stopTrackingWidget(QObject* object)
    {
        const auto widget = qobject_cast<QWidget* const>(object);
        if (!widget)
        {
            return;
        }

        m_widgetToStyleSheetMap.remove(widget);

        // Remove any old stylesheet
        widget->setStyleSheet(QString());
    }

    void StyleManager::initializeFonts()
    {
        // yes, the path specifier could've included OpenSans- and .ttf, but I
        // wanted anyone searching for OpenSans-Bold.ttf to find something so left it this way
        QString openSansPathSpecifier = QStringLiteral(":/AzQtFonts/Fonts/Open_Sans/%1");
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Bold.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-BoldItalic.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-ExtraBold.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-ExtraBoldItalic.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Italic.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Light.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-LightItalic.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Regular.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Semibold.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-SemiboldItalic.ttf"));
    }

    void StyleManager::initializeSearchPaths([[maybe_unused]] QApplication* application, const AZ::IO::PathView& engineRootPath)
    {
        // now that QT is initialized, we can use its path manipulation functions to set the rest up:

        QString rootDir = QString::fromUtf8(engineRootPath.Native().data(), aznumeric_cast<int>(engineRootPath.Native().size()));

        if (!rootDir.isEmpty())
        {
            QDir appPath(rootDir);

            // Set the StyleSheetCache fallback prefix
            const auto pathOnDisk = appPath.absoluteFilePath(g_styleSheetRelativePath.toString());
            m_stylesheetCache->setFallbackSearchPaths(g_searchPathPrefix.toString(), pathOnDisk, g_styleSheetResourcePath.toString());

            // add the expected editor paths
            // this allows you to refer to your assets relative, like
            // STYLESHEETIMAGES:something.txt
            // UI:blah/blah.png
            // EDITOR:blah/something.txt
            QDir::addSearchPath("STYLESHEETIMAGES", appPath.filePath("Assets/Editor/Styles/StyleSheetImages"));
            QDir::addSearchPath("UI", appPath.filePath("Assets/Editor/UI"));
            QDir::addSearchPath("EDITOR", appPath.filePath("Assets/Editor"));
        }
    }

    void StyleManager::LoadThemeProperties(const QString& jsonString)
    {
        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
        QJsonObject rootObject = doc.object();

        // TODO: warn and do something if theme json file is invalid.

        // load in the stylesheet variables
        if (rootObject.contains(StylesheetVariablesKey.data()))
        {
            LoadThemePropertiesRecursively("", rootObject.value(StylesheetVariablesKey.data()).toObject());
        }
    }

    void StyleManager::LoadThemePropertiesRecursively(const QString prefix, const QJsonObject& jsonObject)
    {
        for (const QString& key : jsonObject.keys())
        {
            if (jsonObject[key].isObject())
            {
                LoadThemePropertiesRecursively(prefix + key, jsonObject[key].toObject());
            }
            else
            {
                AZ_TracePrintf(
                    "THEME LOADING", "%s - %s\n", (prefix + key).toUtf8().constData(), jsonObject[key].toString().toUtf8().constData());
                m_themeProperties[prefix + key] = jsonObject[key].toString();
            }
        }
    }

    void StyleManager::refresh()
    {
        const auto globalStyleSheet = m_stylesheetCache->loadStyleSheet(g_globalStyleSheetName.toString());
        m_styleSheetStyle->setGlobalSheet(s_instance->m_stylesheetPreprocessor->ProcessStyleSheet(globalStyleSheet));

        // Iterate widgets and update the stylesheet (the base style has already been set)
        auto i = m_widgetToStyleSheetMap.constBegin();
        while (i != m_widgetToStyleSheetMap.constEnd())
        {
            const auto styleSheet = m_stylesheetCache->loadStyleSheet(i.value());
            i.key()->setStyleSheet(styleSheet);
            ++i;
        }

        // QMessageBox uses "QMdiSubWindowTitleBar" class to query the titlebar font
        // through QApplication::font() and (buggily) calculate required width of itself
        // to fit the title. It bypassess stylesheets. See QMessageBoxPrivate::updateSize().
        QFont titleBarFont("Open Sans");
        titleBarFont.setPixelSize(18);
        QApplication::setFont(titleBarFont, "QMdiSubWindowTitleBar");
    }
} // namespace AzQtComponents

#include "Components/moc_StyleManager.cpp"

#if defined(AZ_QT_COMPONENTS_STATIC)
    // If we're statically compiling the lib, we need to include the compiled rcc resources
    // somewhere to ensure that the linker doesn't optimize the symbols out (with Visual Studio at least)
    // With dlls, there's no step to optimize out the symbols, so we don't need to do this.
    #include <Components/rcc_resources.h>
#endif // #if defined(AZ_QT_COMPONENTS_STATIC)


