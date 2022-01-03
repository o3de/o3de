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

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
#include <QPainterPath>
#include <QPointer>
#include <QProxyStyle>
#include <QScopedPointer>
#include <QVariant>
AZ_POP_DISABLE_WARNING
#endif

class QEvent;
class QIcon;
class QLineEdit;
class QMainWindow;
class QPainter;
class QPushButton;
class QToolBar;

namespace AzQtComponents
{
    namespace Internal
    {
        class DialogEventFilter;
    } // namespace Internal

    /**
    * The UI 2.0 Open 3D Engine Qt Style.
    * 
    * Should not need to be used directly; use the AzQtComponents::StyleManager instead.
    *
    */
    class AZ_QT_COMPONENTS_API Style
        : public QProxyStyle
    {
        Q_OBJECT

        struct Data;

    public:
        enum
        {
            CORNER_RECTANGLE = -1
        };

        explicit Style(QStyle* style = nullptr);
        ~Style() override;

        static QIcon icon(const QString& name);

        static QColor dropZoneColorOnHover();

        /*!
         * Tracks this widget so that when *Config.ini files change, this widget will get repolished.
         * Call this if your widget changes based on the config in it's polish() method, so that it
         * dynamically reloads when the file changes on disk.
         */
        void repolishOnSettingsChange(QWidget* widget);

        /*!
         * Returns true if the widget parameter has the css className applied
         */
        static bool hasClass(const QWidget* widget, const QString& className);

        /*!
         * Adds the css className to the input widget
         */
        static void addClass(QWidget* widget, const QString& className);

        /*!
         * Removes the css className from the input widget
         */
        static void removeClass(QWidget* widget, const QString& className);

        /*!
         * Finds or loads and then caches the pixmap referenced by fileName
         */
        static QPixmap cachedPixmap(const QString& fileName);

        /*!
         * QPainter is not guaranteed to have its QPaintEngine initialized in setRenderHint,
         * so call this work around that.
         * See: QTBUG-51247
         */
        static void prepPainter(QPainter* painter);

        /*!
         * Fixes up the parent of the current application style if need be.
         * This is used for internal bookkeeping to prevent crashes from styles being
         * parented badly.
         */
        static void fixProxyStyle(QProxyStyle* proxyStyle, QStyle* baseStyle);

        /*!
         * Simple helper to draw an anti-aliased frame
         */
        static void drawFrame(QPainter* painter, const QPainterPath& frameRect, const QPen& border, const QBrush& background);

        /*!
         * Call this to explicitly mark your widget to not use the UI 2.0 styling
         */
        static void flagToIgnore(QWidget* widget);

        /*!
         * Call this to remove the flags set from calling flagToIgnore()
         */
        static void removeFlagToIgnore(QWidget* widget);

        /*!
         * Call this to check whether the widget should use the UI 2.0 styling
         */
        static bool hasStyle(const QWidget* widget);
        
        QSize sizeFromContents(QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const override;

        void drawControl(QStyle::ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override;
        void drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override;
        void drawComplexControl(QStyle::ComplexControl element, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const override;
        void drawItemText(QPainter* painter, const QRect& rectangle, int alignment, const QPalette& palette, bool enabled, const QString& text, QPalette::ColorRole textRole) const override;

        void drawDragIndicator(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;

        QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* option) const override;

        QRect subControlRect(ComplexControl control, const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget) const override;
        QRect subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const override;

        int pixelMetric(QStyle::PixelMetric metric, const QStyleOption* option, const QWidget* widget) const override;

        using QProxyStyle::polish;
        void polish(QApplication* application) override;
        void polish(QWidget* widget) override;
        using QProxyStyle::unpolish;
        void unpolish(QWidget* widget) override;

        QPalette standardPalette() const override;
        QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption* option, const QWidget* widget) const override;

        int styleHint(QStyle::StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const override;

        // A path to draw a border frame when color != Qt::transparent
        QPainterPath borderLineEditRect(const QRect& contentsRect, int borderWidth = -1, int borderRadius = CORNER_RECTANGLE) const;
        // A path to draw a border frame when color == Qt::Transparent
        QPainterPath lineEditRect(const QRect& contentsRect, int borderWidth = -1, int borderRadius = CORNER_RECTANGLE) const;

        bool eventFilter(QObject* watched, QEvent* ev) override;

        // Use this class if you have a TableView and you need to wrap calls, indirectly
        // to Style::generatedIconPixmap in order to properly style icons.
        class DrawWidgetSentinel
        {
        public:
            DrawWidgetSentinel(const QWidget* widgetAboutToDraw);
            ~DrawWidgetSentinel();

        private:
            QPointer<const Style> m_style;
            QPointer<const QWidget> m_lastDrawWidget;
        };


#ifdef _DEBUG
    protected:
        bool event(QEvent*) override;
#endif
    Q_SIGNALS:
        void settingsReloaded(); // emitted when any config settings (*.ini) files reload

    private:
        void repolishWidgetDestroyed(QObject* obj);

        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // needs to have dll-interface to be used by clients of class 'AzQtComponents::LineEdit'
        QScopedPointer<Data> m_data;
        AZ_POP_DISABLE_WARNING

        // To be used when text alignment has to be forced from outside Qt
        mutable QVariant m_drawItemTextAlignmentOverride;

        mutable const QWidget* m_drawControlWidget = nullptr;
    };
} // namespace AzQtComponents
