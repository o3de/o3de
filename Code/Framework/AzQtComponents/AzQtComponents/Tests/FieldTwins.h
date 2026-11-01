/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// ----------------------------------------------------------------------------------
// Twins of the field and label widgets: one widget tree built twice, shown, and judged by what the legacy GUI drew. The legacy
// twin is Qt's own style sheet style over Fusion with the application's real sheet (BaseStyleSheet.qss and the files it imports,
// from the library's resources); the flat twin is the flattened Style with the registered Global elements. Same box, same hints,
// same pixels. Everything lives in the namespace FieldTwins, so that a unity build joining this file with the other test files
// finds no second definition.
// ----------------------------------------------------------------------------------

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Elements/ControlElements.h>
#include <AzQtComponents/Components/Elements/PropertyEditorElements.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/ComboBox.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFrame>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QRect>
#include <QRegularExpression>
#include <QString>
#include <QStyleFactory>
#include <QStyleOption>
#include <QWidget>

#include <functional>
#include <initializer_list>
#include <string>
#include <utility>

namespace AzQtComponents
{
    namespace FieldTwins
    {
        // ----------------------------------------------------------------------------------
        // The legacy sheet
        // ----------------------------------------------------------------------------------

        //! The selectors of a sheet to replace: {a class selector of the sheet, what the twin has instead}
        using Replacements = std::initializer_list<std::pair<const char*, const char*>>;

        //! What StyleSheetCache makes of BaseStyleSheet.qss: each @import line replaced by the file it names, from the real resources,
        //! then the selectors named in the replacements replaced
        inline QString appSheet(Replacements replacements = {})
        {
            static QString expanded;
            if (expanded.isEmpty())
            {
                const auto read = [](const QString& name)
                {
                    QFile file(QStringLiteral(":/AzQtComponents/Widgets/") + name);
                    EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << name.toUtf8().constData();
                    return QString::fromUtf8(file.readAll()).replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
                };
                expanded = read(QStringLiteral("BaseStyleSheet.qss"));
                const QRegularExpression import(QStringLiteral("@import\\s+\"([^\"]+)\"\\s*;"));
                for (QRegularExpressionMatch match = import.match(expanded); match.hasMatch(); match = import.match(expanded))
                {
                    expanded.replace(match.capturedStart(), match.capturedLength(), read(match.captured(1)));
                }
            }
            QString sheet = expanded;
            for (const auto& replacement : replacements)
            {
                sheet.replace(QString::fromLatin1(replacement.first), QString::fromLatin1(replacement.second));
            }
            return sheet;
        }

        // ----------------------------------------------------------------------------------
        // Reading what a twin drew
        // ----------------------------------------------------------------------------------

        //! The pixels of two images that differ
        inline int differing(const QImage& a, const QImage& b)
        {
            if (a.size() != b.size())
            {
                return a.width() * a.height();
            }
            int count = 0;
            for (int y = 0; y < a.height(); ++y)
            {
                for (int x = 0; x < a.width(); ++x)
                {
                    count += (a.pixel(x, y) != b.pixel(x, y)) ? 1 : 0;
                }
            }
            return count;
        }

        //! Whether anything was drawn on the image: it is not all of one colour. Two images that nothing was drawn on are equal, which
        //! proves nothing, so a comparison that is meant to prove a drawing checks this of its legacy twin first
        inline bool isDrawnOn(const QImage& image)
        {
            const QRgb first = image.pixel(0, 0);
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    if (image.pixel(x, y) != first)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        //! The image as text, for the failure message of a comparison: the first pixel that differs
        inline std::string firstDifference(const QImage& a, const QImage& b)
        {
            if (a.size() != b.size())
            {
                return QStringLiteral("sizes %1 x %2 against %3 x %4").arg(a.width()).arg(a.height()).arg(b.width()).arg(b.height()).toStdString();
            }
            for (int y = 0; y < a.height(); ++y)
            {
                for (int x = 0; x < a.width(); ++x)
                {
                    if (a.pixel(x, y) != b.pixel(x, y))
                    {
                        return QStringLiteral("(%1, %2): %3 against %4")
                            .arg(x).arg(y).arg(a.pixelColor(x, y).name(QColor::HexArgb), b.pixelColor(x, y).name(QColor::HexArgb)).toStdString();
                    }
                }
            }
            return "equal";
        }

        //! A rect as text, for the failure message of a comparison
        inline std::string rectText(const QRect& rect)
        {
            return QStringLiteral("(%1, %2, %3 x %4)").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height()).toStdString();
        }

        //! A size as text, for the failure message of a comparison
        inline std::string sizeText(const QSize& size)
        {
            return QStringLiteral("%1 x %2").arg(size.width()).arg(size.height()).toStdString();
        }

        //! The option QFrame::paintEvent builds for a frame (QFrame::initStyleOption)
        inline QStyleOptionFrame frameOption(const QFrame* frame)
        {
            QStyleOptionFrame option;
            option.initFrom(frame);
            option.frameShape = frame->frameShape();
            option.lineWidth = frame->lineWidth();
            option.midLineWidth = frame->midLineWidth();
            if (frame->frameShadow() == QFrame::Sunken)
            {
                option.state |= QStyle::State_Sunken;
            }
            else if (frame->frameShadow() == QFrame::Raised)
            {
                option.state |= QStyle::State_Raised;
            }
            return option;
        }

        //! Something a widget draws, through its own style onto an image of an explicit device ratio: QWidget::render would hand the
        //! style the widget as the paint device, and so the ratio of the screen the test runs on. The image is grey: what stays grey is
        //! what neither the sheet nor the element draws. A widget with a styled background has it drawn first (PE_Widget,
        //! QWidgetPrivate::paintBackground).
        inline QImage paintThroughStyle(const QWidget* widget, qreal ratio, const std::function<void(QPainter&)>& paint)
        {
            QImage image((QSizeF(widget->size()) * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(ratio);
            image.fill(QColor(0x80, 0x80, 0x80));
            QPainter painter(&image);
            if (widget->testAttribute(Qt::WA_StyledBackground))
            {
                QStyleOption background;
                background.initFrom(widget);
                widget->style()->drawPrimitive(QStyle::PE_Widget, &background, &painter, widget);
            }
            paint(painter);
            painter.end();
            return image;
        }

        // ----------------------------------------------------------------------------------
        // Fixture: the flattened Style over Fusion, Fusion as the base of the legacy sheet, the Global elements registered
        // ----------------------------------------------------------------------------------

        class Fixture
            : public ::UnitTest::LeakDetectionFixture
        {
        public:
            Fixture()
            {
                m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

                // The fonts and the text colour the application has when StyleManager sets the style (StyleManager.cpp:305-307,
                // Style.cpp buildO3DEBasePalette): the legacy sheet gives labels and buttons the same
                QFont applicationFont(QStringLiteral("Open Sans"));
                applicationFont.setPixelSize(12);
                QApplication::setFont(applicationFont);
                QPalette palette;
                for (const QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
                {
                    palette.setColor(role, Qt::white);
                }
                QApplication::setPalette(palette);

                QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
                m_style = AZStd::make_unique<Style>(QStyleFactory::create(QStringLiteral("Fusion")));
                TextElement::installClassFonts();

                // Registered once for the process, as the application does from Style::polish(QApplication*)
                PropertyEditorElements::registerGlobal();
                ControlElements::registerGlobal();
                ComboBox::registerFlatElements();

                // What Style::polish(QApplication*) installs: the events of every widget of the application (a class tag set after the
                // polish of a label, for one)
                QApplication::instance()->installEventFilter(m_style.get());
            }

            ~Fixture() override
            {
                m_style.reset();
                m_application.reset();
            }

            //! The legacy twin: the sheet on the root, over the application's Fusion
            static void legacy(QWidget& root, Replacements replacements = {})
            {
                root.setStyleSheet(appSheet(replacements));
            }

            //! The flat twin: the style on the root and on everything under it (a widget's style is not its children's)
            void flat(QWidget& root) const
            {
                root.setStyle(m_style.get());
                for (QWidget* child : root.findChildren<QWidget*>())
                {
                    child->setStyle(m_style.get());
                }
            }

            static void show(QWidget& root, const QSize& size)
            {
                root.setAttribute(Qt::WA_DontShowOnScreen);
                root.resize(size);
                root.show();
                QApplication::processEvents();
            }

            //! One tree twice, shown: the build function fills a root with it, and is told which twin the root is (a widget that sizes
            //! itself the flattened way in its constructor is put back to what the legacy GUI built for the twin of the sheet)
            struct Twins
            {
                template<typename Build>
                Twins(const Fixture& fixture, Build build, const QSize& size, Replacements replacements = {})
                {
                    build(m_legacy, true);
                    legacy(m_legacy, replacements);
                    show(m_legacy, size);
                    build(m_flat, false);
                    fixture.flat(m_flat);
                    show(m_flat, size);
                }

                template<typename W = QWidget>
                W* legacyChild(const char* name)
                {
                    return m_legacy.findChild<W*>(QString::fromLatin1(name));
                }

                template<typename W = QWidget>
                W* flatChild(const char* name)
                {
                    return m_flat.findChild<W*>(QString::fromLatin1(name));
                }

                QWidget m_legacy;
                QWidget m_flat;
            };

        protected:
            int m_argc = 1;                                         // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
            AZStd::unique_ptr<Style> m_style;
        };
    } // namespace FieldTwins
} // namespace AzQtComponents
