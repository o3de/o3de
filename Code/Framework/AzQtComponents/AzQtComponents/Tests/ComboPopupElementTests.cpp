/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Elements/ComboPopupElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QImage>
#include <QListView>
#include <QPainter>
#include <QStyleFactory>
#include <QStyleOption>

#include <string>

namespace AzQtComponents
{
    namespace
    {
        constexpr QRgb PopupSurface = 0x222222;         // ComboBox.qss:81
        constexpr QRgb PopupSelectedFill = 0x444444;    // ComboBox.qss:104

        //! The colour of one pixel, without alpha
        QRgb popupPixel(const QImage& image, int x, int y)
        {
            return image.pixelColor(x, y).rgb() & 0xFFFFFF;
        }

        //! The pixels on the border of the image that are not the given colour
        int popupBorderPixelsNot(const QImage& image, QRgb color)
        {
            int count = 0;
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    const bool onBorder = x == 0 || y == 0 || x == image.width() - 1 || y == image.height() - 1;
                    if (onBorder && popupPixel(image, x, y) != color)
                    {
                        ++count;
                    }
                }
            }
            return count;
        }

        //! A rect as text, for the failure message of a comparison
        std::string popupRectText(const QRect& rect)
        {
            return QStringLiteral("(%1, %2, %3 x %4)").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height()).toStdString();
        }
    } // namespace

    // ----------------------------------------------------------------------------------
    // Fixture: the flattened style chain (Style -> Fusion) under an application
    // ----------------------------------------------------------------------------------

    class ComboPopupElementTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        ComboPopupElementTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

            // As StyleManager::initialize builds it (StyleManager.cpp:244-246); the application owns it
            m_style = new Style(QStyleFactory::create(QStringLiteral("Fusion")));
            QApplication::setStyle(m_style);
        }

        ~ComboPopupElementTests() override
        {
            m_application.reset();
        }

        //! The surface, text and selection colors of the popup element, in the palette of one widget of the popup
        static void expectPopupPalette(const QPalette& palette)
        {
            EXPECT_TRUE(palette.color(QPalette::Active, QPalette::Window) == QColor(0x22, 0x22, 0x22));
            EXPECT_TRUE(palette.color(QPalette::Active, QPalette::Base) == QColor(0x22, 0x22, 0x22));
            EXPECT_TRUE(palette.color(QPalette::Active, QPalette::Text) == QColor(Qt::white));
            EXPECT_TRUE(palette.color(QPalette::Active, QPalette::Highlight) == QColor(0x44, 0x44, 0x44));
            EXPECT_TRUE(palette.color(QPalette::Disabled, QPalette::Text) == QColor(0x55, 0x55, 0x55));
        }

        //! The option an item view hands the style for one item of the list
        static QStyleOptionViewItem itemOption(const QAbstractItemView* view, QStyle::State state)
        {
            QStyleOptionViewItem option;
            option.initFrom(view);
            option.widget = view;
            option.rect = QRect(0, 0, 80, 20);
            option.state = state;
            option.features = QStyleOptionViewItem::HasDisplay;
            option.text = QStringLiteral("Two");
            option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
            return option;
        }

        //! One item of the popup list painted through the style, over the surface of the list
        QImage paintItem(const QAbstractItemView* view, QStyle::State state) const
        {
            const QStyleOptionViewItem option = itemOption(view, state);

            QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(QColor::fromRgb(PopupSurface));
            QPainter painter(&image);
            m_style->drawControl(QStyle::CE_ItemViewItem, &option, &painter, view);
            painter.end();
            return image;
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
        Style* m_style = nullptr;                           // owned by the application
    };

    // ----------------------------------------------------------------------------------
    // T-CP.1: the element holds the winners of ComboBox.qss:79-117, and selects the popup of a combo box
    // ----------------------------------------------------------------------------------

    TEST_F(ComboPopupElementTests, Element_HoldsTheWinnersOfTheComboBoxSheet)
    {
        const ComboPopupElement& popup = ComboPopupElement::global();

        EXPECT_STREQ(popup.m_name, "Global.ComboPopup.Items");
        EXPECT_TRUE(popup.m_surface == QColor(0x22, 0x22, 0x22));           // :81 QComboBox QAbstractItemView { background-color }
        EXPECT_TRUE(popup.m_text == QColor(0xFF, 0xFF, 0xFF));              // :89 ::item { color }
        EXPECT_TRUE(popup.m_selectedFill == QColor(0x44, 0x44, 0x44));      // :104 ::item:selected { background-color }
        EXPECT_TRUE(popup.m_selectedText == QColor(0xFF, 0xFF, 0xFF));      // :105 ::item:selected { color }
        EXPECT_TRUE(popup.m_disabledText == QColor(0x55, 0x55, 0x55));      // :110 ::item:disabled { color }
        EXPECT_EQ(popup.m_itemPaddingLeft, 24);                             // :92 ::item { padding-left }
        EXPECT_EQ(popup.m_itemPaddingRight, 24);                            // :90 ::item { padding-right }
    }

    TEST_F(ComboPopupElementTests, Selectors_CoverTheContainerAndTheViewOfAComboBox)
    {
        QComboBox combo;
        combo.addItems({ QStringLiteral("One"), QStringLiteral("Two") });
        QAbstractItemView* view = combo.view();
        QWidget* container = view->parentWidget();
        ASSERT_TRUE(container != nullptr);

        // The class name the element selects the container by: if Qt renames it, the popup is no longer covered
        ASSERT_TRUE(container->inherits("QComboBoxPrivateContainer"));

        EXPECT_TRUE(ComboPopupElement::covers(container));
        EXPECT_TRUE(ComboPopupElement::covers(view));
        EXPECT_TRUE(ComboPopupElement::coversView(view));
        EXPECT_FALSE(ComboPopupElement::coversView(container));
        EXPECT_FALSE(ComboPopupElement::covers(&combo));
    }

    TEST_F(ComboPopupElementTests, Selectors_LeaveOtherWidgetsAlone)
    {
        QListView list;
        QWidget plain;

        EXPECT_FALSE(ComboPopupElement::covers(&list));
        EXPECT_FALSE(ComboPopupElement::coversView(&list));
        EXPECT_FALSE(ComboPopupElement::covers(&plain));
        EXPECT_FALSE(ComboPopupElement::covers(nullptr));
        EXPECT_FALSE(ComboPopupElement::coversView(nullptr));
    }

    // ----------------------------------------------------------------------------------
    // T-CP.2: the palette of the element, and the two ways the popup takes it
    // ----------------------------------------------------------------------------------

    TEST_F(ComboPopupElementTests, Palette_SetsSurfaceTextAndSelectionInEveryGroup)
    {
        const QPalette palette = ComboPopupElement::global().palette(QPalette());

        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            EXPECT_TRUE(palette.color(group, QPalette::Window) == QColor(0x22, 0x22, 0x22));
            EXPECT_TRUE(palette.color(group, QPalette::Base) == QColor(0x22, 0x22, 0x22));
            EXPECT_TRUE(palette.color(group, QPalette::Highlight) == QColor(0x44, 0x44, 0x44));
        }
        for (const auto role : { QPalette::Text, QPalette::WindowText, QPalette::ButtonText, QPalette::HighlightedText })
        {
            EXPECT_TRUE(palette.color(QPalette::Active, role) == QColor(Qt::white));
            EXPECT_TRUE(palette.color(QPalette::Inactive, role) == QColor(Qt::white));
            EXPECT_TRUE(palette.color(QPalette::Disabled, role) == QColor(0x55, 0x55, 0x55));
        }
    }

    TEST_F(ComboPopupElementTests, Polish_ViewAndContainerTakeThePaletteOfTheElement)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QComboBox combo;
        combo.addItems({ QStringLiteral("One"), QStringLiteral("Two") });
        QAbstractItemView* view = combo.view();
        QWidget* container = view->parentWidget();
        ASSERT_TRUE(container != nullptr);

        container->ensurePolished();
        view->ensurePolished();

        expectPopupPalette(container->palette());
        expectPopupPalette(view->palette());
    }

    TEST_F(ComboPopupElementTests, Polish_ViewKeepsThePaletteWhenTheComboBoxReplacesTheContainers)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QComboBox combo;
        combo.addItems({ QStringLiteral("One"), QStringLiteral("Two") });
        QAbstractItemView* view = combo.view();
        view->ensurePolished();

        // The combo box hands its palette to the container whenever its own changes (qcombobox.cpp:1430-1448,
        // 3295-3297), and replaces the container's palette whole. The roles the view set on itself stay.
        QPalette changed = combo.palette();
        changed.setColor(QPalette::Base, Qt::red);
        changed.setColor(QPalette::Text, Qt::red);
        changed.setColor(QPalette::Highlight, Qt::red);
        combo.setPalette(changed);

        expectPopupPalette(view->palette());
    }

    // ----------------------------------------------------------------------------------
    // T-CP.3: an item is painted from the element, not from the palette Qt hands the view
    // ----------------------------------------------------------------------------------

    TEST_F(ComboPopupElementTests, PrepareItem_SelectedRowIsFilledAndLosesItsOutline)
    {
        QImage image(40, 20, QImage::Format_ARGB32_Premultiplied);
        image.fill(QColor::fromRgb(PopupSurface));

        QStyleOptionViewItem option;
        option.rect = image.rect();
        option.state = QStyle::State_Enabled | QStyle::State_Active | QStyle::State_Selected | QStyle::State_HasFocus | QStyle::State_MouseOver;

        QPainter painter(&image);
        ComboPopupElement::global().prepareItem(option, &painter);
        painter.end();

        // ComboBox.qss:104 `::item:selected { background-color: #444444 }`, :83 `outline: none`
        EXPECT_EQ(popupPixel(image, 0, 0), PopupSelectedFill);
        EXPECT_EQ(popupPixel(image, 39, 19), PopupSelectedFill);
        EXPECT_TRUE(option.state.testFlag(QStyle::State_Enabled));
        EXPECT_TRUE(option.state.testFlag(QStyle::State_Active));
        EXPECT_FALSE(option.state.testFlag(QStyle::State_Selected));
        EXPECT_FALSE(option.state.testFlag(QStyle::State_HasFocus));
        EXPECT_FALSE(option.state.testFlag(QStyle::State_MouseOver));
        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            EXPECT_TRUE(option.palette.color(group, QPalette::Text) == QColor(Qt::white));   // :105 ::item:selected { color }
        }
    }

    TEST_F(ComboPopupElementTests, PrepareItem_PlainRowIsNotFilled)
    {
        QImage image(40, 20, QImage::Format_ARGB32_Premultiplied);
        image.fill(QColor::fromRgb(PopupSurface));

        QStyleOptionViewItem option;
        option.rect = image.rect();
        option.state = QStyle::State_Enabled | QStyle::State_Active;

        QPainter painter(&image);
        ComboPopupElement::global().prepareItem(option, &painter);
        painter.end();

        EXPECT_EQ(popupBorderPixelsNot(image, PopupSurface), 0);
        EXPECT_TRUE(option.palette.color(QPalette::Active, QPalette::Text) == QColor(Qt::white));   // :89 ::item { color }
    }

    TEST_F(ComboPopupElementTests, PrepareItem_DisabledRowHasGreyTextAndNoFill)
    {
        QImage image(40, 20, QImage::Format_ARGB32_Premultiplied);
        image.fill(QColor::fromRgb(PopupSurface));

        QStyleOptionViewItem option;
        option.rect = image.rect();
        option.state = QStyle::State_Selected;      // :107-111 `::item:disabled` has no fill, whatever the selection says

        QPainter painter(&image);
        ComboPopupElement::global().prepareItem(option, &painter);
        painter.end();

        EXPECT_EQ(popupBorderPixelsNot(image, PopupSurface), 0);
        for (const auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            EXPECT_TRUE(option.palette.color(group, QPalette::Text) == QColor(0x55, 0x55, 0x55));   // :110 ::item:disabled { color }
        }
    }

    TEST_F(ComboPopupElementTests, DrawItem_SelectedItemOfTheListIsOneSolidFillWithoutARim)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QComboBox combo;
        combo.addItems({ QStringLiteral("One"), QStringLiteral("Two") });

        // The state a delegate gives the current item of the popup: selected, with the focus and the mouse over it
        const QImage image = paintItem(combo.view(),
            QStyle::State_Enabled | QStyle::State_Active | QStyle::State_Selected | QStyle::State_HasFocus | QStyle::State_MouseOver);

        EXPECT_EQ(popupBorderPixelsNot(image, PopupSelectedFill), 0);
        EXPECT_EQ(popupPixel(image, 2, 10), PopupSelectedFill);
    }

    TEST_F(ComboPopupElementTests, DrawItem_PlainItemOfTheListIsLeftOnTheSurface)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QComboBox combo;
        combo.addItems({ QStringLiteral("One"), QStringLiteral("Two") });

        const QImage image = paintItem(combo.view(), QStyle::State_Enabled | QStyle::State_Active | QStyle::State_HasFocus);

        EXPECT_EQ(popupBorderPixelsNot(image, PopupSurface), 0);
    }

    // ----------------------------------------------------------------------------------
    // T-CP.4: the room for the check mark the delegate draws (ComboBox.qss:90, :92)
    // ----------------------------------------------------------------------------------

    TEST_F(ComboPopupElementTests, TextRect_IsPaddedBy24OnBothSidesOnlyInThePopup)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        QComboBox combo;
        combo.addItems({ QStringLiteral("One"), QStringLiteral("Two") });
        QAbstractItemView* view = combo.view();
        QListView plain;

        QStyleOptionViewItem option = itemOption(view, QStyle::State_Enabled);
        option.rect = QRect(0, 0, 120, 20);

        const QRect popupText = m_style->subElementRect(QStyle::SE_ItemViewItemText, &option, view);
        const QRect popupBase = m_style->baseStyle()->subElementRect(QStyle::SE_ItemViewItemText, &option, view);
        EXPECT_EQ(popupText, popupBase.adjusted(24, 0, -24, 0)) << popupRectText(popupText) << " of " << popupRectText(popupBase);

        // An item view that is not in a combo box keeps the base style's text rect
        option.widget = &plain;
        const QRect plainText = m_style->subElementRect(QStyle::SE_ItemViewItemText, &option, &plain);
        const QRect plainBase = m_style->baseStyle()->subElementRect(QStyle::SE_ItemViewItemText, &option, &plain);
        EXPECT_EQ(plainText, plainBase) << popupRectText(plainText) << " of " << popupRectText(plainBase);
    }
} // namespace AzQtComponents
