/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <AzCore/IO/Path/Path_fwd.h>

#include <QObject>
#include <QHash>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::StyleManager::m_widgetToStyleSheetMap': class 'QHash<QWidget *,QString>' needs to have dll-interface to be used by clients of class 'AzQtComponents::StyleManager'
#include <QPointer>
AZ_POP_DISABLE_WARNING

class QApplication;
class QStyle;
class QWidget;
class QColor;

namespace AzQtComponents
{
    class StyleSheetCache;
    class StylesheetPreprocessor;
    class AutoCustomWindowDecorations;
    class StyleManagerTests;

    /**
     * Wrapper around classes dealing with Open 3D Engine style.
     *
     * New applications should work like this:
     *   
     *   int main(int argv, char **argc)
     *   {
     *           QApplication app(argv, argc);
     *
     *           AzQtComponents::StyleManager styleManager(&app);
     *           const bool useUI10 = false;
     *           styleManager.Initialize(&app, useUI10);
     *           .
     *           .
     *           .
     *   }
     *
     */
    class AZ_QT_COMPONENTS_API StyleManager
        : public QObject
    {
        Q_OBJECT

        friend StyleManagerTests;

        static StyleManager* s_instance;

    public:
        static bool isInstanced() { return s_instance; }

        static void addSearchPaths(const QString& searchPrefix, const QString& pathOnDisk, const QString& qrcPrefix,
            const AZ::IO::PathView& engineRootPath);

        static bool setStyleSheet(QWidget* widget, QString styleFileName);

        //! setStyleSheet for the Gallery's purposeful sample pages, whose sheets stay live in the flattened GUI as they are in the legacy
        //! one: the file is loaded as setStyleSheet loads it and set on the widget in both GUIs, where setStyleSheet sets none in the
        //! flattened one (elements carry its sheets). The sheet is tracked and reloaded by refresh in both. In the flattened GUI the
        //! application's sheet is not there to give a label its margin above and below (Text.qss:93-94), which the legacy GUI draws a
        //! sample's fill inside, so the flattened GUI sets that margin ahead of the file's rules.
        static bool setSampleStyleSheet(QWidget* widget, QString styleFileName);

        //! Flattened GUI only (see stylesheetsDisabled): the function that defines the rules of a style sheet as
        //! elements (TextElement::registerElement, ...), registered under the name the sheet is set by (with or
        //! without the extension). setStyleSheet runs it the first time that sheet is set on a widget, and a
        //! registration after that runs at once, so the order of calls does not matter. Several functions may
        //! register for one sheet (a sheet shared by modules); legacy mode never runs any.
        using FlatSheetRegistrar = void (*)();
        static void registerFlatSheet(const QString& styleFileName, FlatSheetRegistrar registrar);

        //! A widget's own style sheet that only the legacy GUI sets. The flattened GUI (stylesheetsDisabled) carries the
        //! sheet's look in elements, so there the call sets nothing and the widget never gets a QStyleSheetStyle. For the
        //! sites the flattening has converted: a site whose look no element carries yet calls QWidget::setStyleSheet.
        static void setLegacyStyleSheet(QWidget* widget, const QString& styleSheet);

        static QStyle* styleSheetStyle(const QWidget* widget);
        static QStyle* baseStyle(const QWidget* widget);

        static void repolishStyleSheet(QWidget* widget);

        //! Utility - set an object property, and return true if it changed.
        static bool setObjectProperty(QWidget* widget, const char* propertyName, const QVariant& value);

        //! True when the O3DE_DISABLE_STYLESHEETS environment variable is set or the application
        //! was launched with --disable-stylesheets (GUI flattening work). In that mode no Qt style
        //! sheets are installed anywhere and widgets render through the plain style chain
        //! (Style -> Fusion base), driven by the palette instead of .qss rules.
        //! The noop and passthrough rungs of the GuiPerfFloor ladder (--stylesheets-noop, --style-passthrough)
        //! imply it: it is true at flat, noop and passthrough.
        static bool stylesheetsDisabled();

        //! True when the flattened GUI's own styling is in effect: stylesheetsDisabled(), and not the passthrough
        //! rung of the GuiPerfFloor ladder, where Style hands every call to its base style and the widgets leave out
        //! what they set on themselves in place of a rule (palettes, sizes, their own painting).
        static bool flatStylingActive();

        //! The flat gate for code that cannot call stylesheetsDisabled (the Python tools, which have no binding for it and cannot see the
        //! command-line flag): the name of the dynamic property that initialize sets on the QApplication to true when stylesheetsDisabled()
        //! holds, and leaves unset in the legacy GUI. PySide reads it as `QApplication.instance().property("o3deFlatGui")`, which is
        //! true in the flattened GUI and None in the legacy one. The property is written once, by initialize: the rung does not change
        //! while the application runs.
        static constexpr const char* FlatGuiProperty = "o3deFlatGui";

        explicit StyleManager(QObject* parent);
        ~StyleManager() override;

        /*!
        * Call to initialize the StyleManager, allowing it to hook into the application and apply the global style
        * The AzQtComponents does not hook in the AZ::Environment, so the Settings Registry isn't available
        * Therefore the engine root path must be supplied via function arguments
        */
        void initialize(QApplication* application, const AZ::IO::PathView& engineRootPath);

        /*!
        * Call this to force a refresh of the global stylesheet and a reload of any settings files.
        * Note that you should never need to do this manually.
        */
        void refresh();
        // deprecated; introduced before the new camelCase Qt based method names were adopted.
        void Refresh() { refresh(); }

        /*!
        * Used to get a global color value by name.
        * Deprecated; do not use.
        * This was implemented to support skinning of the Editor,
        * but that functionality is no longer supported. If you
        * want to load a color instead of hard coding it, please
        * embed the color into a stylesheet instead of using
        * GetColorByName.
        */
        const QColor& getColorByName(const QString& name);
        // deprecated; introduced before the new camelCase Qt based method names were adopted.
        const QColor& GetColorByName(const QString& name) { return getColorByName(name); }

    private Q_SLOTS:
        void cleanupStyles();
        void stopTrackingWidget(QObject* object);

    private:
        //! setStyleSheet and setSampleStyleSheet. With `carriedByElements` the flattened GUI sets no sheet (applyFlatSheet).
        static bool setFileStyleSheet(QWidget* widget, QString styleFileName, bool carriedByElements);

        //! The flat branch of setStyleSheet
        static void applyFlatSheet(QWidget* widget, const QString& styleFileName);

        void initializeFonts();
        void initializeSearchPaths(QApplication* application, const AZ::IO::PathView& engineRootPath);

        void resetWidgetSheets();

        StylesheetPreprocessor* m_stylesheetPreprocessor = nullptr;
        StyleSheetCache* m_stylesheetCache = nullptr;

        using WidgetToStyleSheetMap = QHash<QWidget*, QString>;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::StyleManager::m_widgetToStyleSheetMap': class 'QHash<QWidget *,QString>' needs to have dll-interface to be used by clients of class 'AzQtComponents::StyleManager'
        WidgetToStyleSheetMap m_widgetToStyleSheetMap;

        // Track the style as a QPointer, as the QApplication will delete it if it still has a pointer to it
        QPointer<QStyle> m_style;
        AZ_POP_DISABLE_WARNING

        AutoCustomWindowDecorations* m_autoCustomWindowDecorations = nullptr;
    };
} // namespace AzQtComponents

