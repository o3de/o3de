/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QFont>
#include <QHeaderView>
#include <QImage>
#include <QPainter>
#include <QPair>
#include <QStandardItemModel>
#include <QStyleFactory>
#include <QStyleOptionHeader>
#include <QTableView>

namespace AzQtComponents
{
    namespace
    {
        // TableView.qss:62-76 QHeaderView::section, stated again here: the tests prove the arithmetic the
        // flattened header carries (and the draw), not that it reads the sheet.
        constexpr int SectionHeight = 24;               // height: 24px
        constexpr int SectionPaddingLeft = 7;           // padding-left: 7px
        constexpr int SectionWidth = 100;               // the width of the sections the tests paint

        QColor sectionFill()
        {
            return QColor(0x2D, 0x2D, 0x2D);            // background: #2d2d2d
        }

        // The rule as the legacy GUI applied it to the headers of a QTableView
        const char* const SectionRule = "QTableView QHeaderView::section { height: 24px; background: #2d2d2d; border: none;"
                                        " font-size: 12px; font-style: normal; padding-left: 7px; }";
    } // namespace

    // ----------------------------------------------------------------------------------
    // Fixture: an application, a Fusion base, and the style under test on its own
    // ----------------------------------------------------------------------------------

    class HeaderSectionTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        HeaderSectionTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
            QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion"))); // the base of the legacy sheet
            m_style = AZStd::make_unique<Style>(QStyleFactory::create(QStringLiteral("Fusion")));
        }

        ~HeaderSectionTests() override
        {
            m_style.reset();
            m_application.reset();
        }

        //! The option QHeaderView::paintSection builds for a section, with a short label
        static QStyleOptionHeader sectionOption(const QHeaderView* header, const QRect& rect, Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter)
        {
            QStyleOptionHeader option;
            option.initFrom(header);
            option.rect = rect;
            option.orientation = header->orientation();
            option.state |= QStyle::State_Enabled;
            if (header->orientation() == Qt::Horizontal)
            {
                option.state |= QStyle::State_Horizontal;
            }
            option.text = QStringLiteral("Name");
            option.textAlignment = alignment;
            option.position = QStyleOptionHeader::OnlyOneSection;
            return option;
        }

        //! One section of SectionWidth x SectionHeight painted through the style over magenta, so that a pixel the draw
        //! leaves shows. The painter's font stands for the header's viewport font (12px, upright by default).
        QImage drawSection(const QHeaderView* header, QStyleOptionHeader::SortIndicator sortIndicator, Qt::Alignment alignment,
            int fontPixelSize = 12, bool fontItalic = false) const
        {
            QImage image(SectionWidth, SectionHeight, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::magenta);

            QStyleOptionHeader option = sectionOption(header, image.rect(), alignment);
            option.sortIndicator = sortIndicator;

            QFont font;
            font.setPixelSize(fontPixelSize);
            font.setItalic(fontItalic);

            QPainter painter(&image);
            painter.setFont(font);
            m_style->drawControl(QStyle::CE_Header, &option, &painter, header);
            painter.end();
            return image;
        }

        //! The leftmost and rightmost columns holding something other than the fill (-1 when the section is all fill)
        static QPair<int, int> labelColumns(const QImage& image)
        {
            int first = -1;
            int last = -1;
            for (int x = 0; x < image.width(); ++x)
            {
                for (int y = 0; y < image.height(); ++y)
                {
                    if (image.pixelColor(x, y) != sectionFill())
                    {
                        first = (first < 0) ? x : first;
                        last = x;
                    }
                }
            }
            return { first, last };
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
        AZStd::unique_ptr<Style> m_style;
    };

    // ----------------------------------------------------------------------------------
    // T1: the legacy rule's arithmetic, in Qt's own sheet style (runs in every process)
    // ----------------------------------------------------------------------------------

    TEST_F(HeaderSectionTests, LegacyRule_SectionIsTheRulesHeightAndTheBaseWidthPlusThePadding)
    {
        QStandardItemModel model(1, 2);
        model.setHeaderData(0, Qt::Horizontal, QStringLiteral("Name"));
        model.setHeaderData(1, Qt::Horizontal, QStringLiteral("Default Value"));

        QTableView bare;
        bare.setModel(&model);
        QTableView legacy;
        legacy.setModel(&model);
        legacy.setStyleSheet(QString::fromUtf8(SectionRule));

        const QHeaderView* bareHeader = bare.horizontalHeader();
        const QHeaderView* legacyHeader = legacy.horizontalHeader();

        // qstylesheetstyle.cpp:5352-5382 (CT_HeaderSection): the rule's height, the base size's width plus the padding
        EXPECT_EQ(legacyHeader->sizeHint().height(), SectionHeight);
        for (int section = 0; section < model.columnCount(); ++section)
        {
            EXPECT_EQ(legacyHeader->sectionSizeHint(section), bareHeader->sectionSizeHint(section) + SectionPaddingLeft);
        }
    }

    TEST_F(HeaderSectionTests, LegacyRule_LabelBoxIsTheSectionLessThePaddingWhateverTheSortArrow)
    {
        QStandardItemModel model(1, 1);
        QTableView legacy;
        legacy.setModel(&model);
        legacy.setStyleSheet(QString::fromUtf8(SectionRule));
        const QHeaderView* header = legacy.horizontalHeader();
        header->ensurePolished();

        QStyleOptionHeader option;
        option.rect = QRect(10, 0, SectionWidth, SectionHeight);
        option.orientation = Qt::Horizontal;
        option.state = QStyle::State_Enabled | QStyle::State_Horizontal;

        // qstylesheetstyle.cpp:6264-6270 (SE_HeaderLabel): the contents rect of the rule, no room kept for the arrow
        const QRect labelBox(10 + SectionPaddingLeft, 0, SectionWidth - SectionPaddingLeft, SectionHeight);
        EXPECT_TRUE(header->style()->subElementRect(QStyle::SE_HeaderLabel, &option, header) == labelBox);

        option.sortIndicator = QStyleOptionHeader::SortDown;
        EXPECT_TRUE(header->style()->subElementRect(QStyle::SE_HeaderLabel, &option, header) == labelBox);
    }

    // ----------------------------------------------------------------------------------
    // T2: the flattened header
    // ----------------------------------------------------------------------------------

    TEST_F(HeaderSectionTests, Flat_SectionSizeIsTheRulesHeightAndTheBaseWidthPlusThePadding)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        QTableView table;
        const QHeaderView* horizontal = table.horizontalHeader();
        const QStyleOptionHeader option = sectionOption(horizontal, QRect());

        // The base of a horizontal header is TableView's (TableView.cpp:262-273): text + 16 + margin + margin
        const int margin = m_style->pixelMetric(QStyle::PM_HeaderMargin, &option, horizontal);
        const int textWidth = option.fontMetrics.size(0, option.text).width();
        const QSize horizontalSize = m_style->sizeFromContents(QStyle::CT_HeaderSection, &option, QSize(), horizontal);
        EXPECT_TRUE(horizontalSize == QSize(textWidth + 16 + margin + margin + SectionPaddingLeft, SectionHeight));

        // The base of a vertical header is Qt's
        const QHeaderView* vertical = table.verticalHeader();
        const QStyleOptionHeader verticalOption = sectionOption(vertical, QRect());
        const QSize qtSize = m_style->baseStyle()->sizeFromContents(QStyle::CT_HeaderSection, &verticalOption, QSize(), vertical);
        const QSize verticalSize = m_style->sizeFromContents(QStyle::CT_HeaderSection, &verticalOption, QSize(), vertical);
        EXPECT_TRUE(verticalSize == QSize(qtSize.width() + SectionPaddingLeft, SectionHeight));
    }

    TEST_F(HeaderSectionTests, Flat_HeaderOutsideAnItemViewKeepsTheBaseSize)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        // TableView.qss:62-65 reaches the headers of a QTableView, a QListView and a QTreeView only
        QHeaderView orphan(Qt::Horizontal);
        const QStyleOptionHeader option = sectionOption(&orphan, QRect());

        const int margin = m_style->pixelMetric(QStyle::PM_HeaderMargin, &option, &orphan);
        const int textWidth = option.fontMetrics.size(0, option.text).width();
        const QSize size = m_style->sizeFromContents(QStyle::CT_HeaderSection, &option, QSize(), &orphan);
        EXPECT_EQ(size.width(), textWidth + 16 + margin + margin);
    }

    TEST_F(HeaderSectionTests, Flat_SectionIsFilledToItsEdges)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        QTableView table;
        QHeaderView* header = table.horizontalHeader();
        m_style->polish(header);

        const QImage image = drawSection(header, QStyleOptionHeader::None, Qt::AlignLeft | Qt::AlignVCenter);

        // border: none -> the fill reaches every edge (the first flat draw put a #222222 line on the right and the bottom)
        const QPoint edges[] = { QPoint(0, 0), QPoint(SectionWidth - 1, 0), QPoint(0, SectionHeight - 1),
                                 QPoint(SectionWidth - 1, SectionHeight - 1), QPoint(SectionWidth - 1, SectionHeight / 2),
                                 QPoint(SectionWidth - 25, SectionHeight - 1) };
        for (const QPoint& edge : edges)
        {
            EXPECT_TRUE(image.pixelColor(edge) == sectionFill());
        }
    }

    TEST_F(HeaderSectionTests, Flat_LabelBoxIsTheSectionLessThePaddingOnTheLeftOnly)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        QTableView table;
        QHeaderView* header = table.horizontalHeader();
        m_style->polish(header);

        // Left: the label starts at the padding (a pixel of slack for the glyph's own edge)
        const QPair<int, int> left = labelColumns(drawSection(header, QStyleOptionHeader::None, Qt::AlignLeft | Qt::AlignVCenter));
        EXPECT_GE(left.first, SectionPaddingLeft - 1);

        // Right: the label runs to the section's edge; the first flat draw kept 4px back, so the text ended before x = 96
        const QPair<int, int> right = labelColumns(drawSection(header, QStyleOptionHeader::None, Qt::AlignRight | Qt::AlignVCenter));
        EXPECT_GE(right.second, SectionWidth - 4);
    }

    TEST_F(HeaderSectionTests, Flat_LabelIsTwelvePixelsAndUprightWhateverFontTheViewportHas)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        QTableView table;
        QHeaderView* header = table.horizontalHeader();
        m_style->polish(header);

        // font-size: 12px, font-style: normal (TableView.qss:72-73) over the painter's font (qstylesheetstyle.cpp:4142-4145)
        const QImage viewportFont = drawSection(header, QStyleOptionHeader::None, Qt::AlignLeft | Qt::AlignVCenter);

        const QImage codeFont = drawSection(header, QStyleOptionHeader::None, Qt::AlignLeft | Qt::AlignVCenter, 24, true);

        EXPECT_TRUE(viewportFont == codeFont);
    }

    TEST_F(HeaderSectionTests, Flat_SortArrowIsTheBaseStylesAndNothingElseChanges)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        QTableView table;
        QHeaderView* header = table.horizontalHeader();
        m_style->polish(header);

        const QImage plain = drawSection(header, QStyleOptionHeader::None, Qt::AlignLeft | Qt::AlignVCenter);
        const QImage sorted = drawSection(header, QStyleOptionHeader::SortDown, Qt::AlignLeft | Qt::AlignVCenter);

        // qcommonstyle.cpp:2172-2177: the arrow goes in SE_HeaderArrow, after the label, which does not move for it
        const QStyleOptionHeader option = sectionOption(header, plain.rect());
        const QRect arrow = m_style->subElementRect(QStyle::SE_HeaderArrow, &option, header).adjusted(-2, -2, 2, 2);

        bool differsInsideArrow = false;
        bool differsOutsideArrow = false;
        for (int y = 0; y < plain.height(); ++y)
        {
            for (int x = 0; x < plain.width(); ++x)
            {
                if (plain.pixel(x, y) != sorted.pixel(x, y))
                {
                    if (arrow.contains(x, y))
                    {
                        differsInsideArrow = true;
                    }
                    else
                    {
                        differsOutsideArrow = true;
                    }
                }
            }
        }
        EXPECT_TRUE(differsInsideArrow);
        EXPECT_FALSE(differsOutsideArrow);
    }

    TEST_F(HeaderSectionTests, Flat_PolishedHeaderHasWhiteLabelAndArrowInEveryState)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled()); // the test environment sets it (AzQtComponentTests.cpp)

        // No rule gives the label or the arrow a colour: the '*' rule's white (BaseStyleSheet.qss:18) reached them,
        // and reached them disabled too (the legacy palette dump of LuaIDE's disabled headers)
        QTableView table;
        QHeaderView* header = table.horizontalHeader();
        m_style->polish(header);

        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            EXPECT_TRUE(header->palette().color(group, QPalette::ButtonText) == QColor(Qt::white));
            EXPECT_TRUE(header->palette().color(group, QPalette::WindowText) == QColor(Qt::white));
        }
    }
} // namespace AzQtComponents
