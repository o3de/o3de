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
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzQtComponents/Components/Widgets/VectorInput.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QLineEdit>
#include <QStyleFactory>
#include <QStyleOption>

#include <string>

namespace AzQtComponents
{
    namespace
    {
        //! A rect as text, for the failure message of a comparison
        std::string spinBoxRectText(const QRect& rect)
        {
            return QStringLiteral("(%1, %2, %3 x %4)").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height()).toStdString();
        }
    } // namespace

    // ----------------------------------------------------------------------------------
    // Fixture: the flattened style chain (Style -> Fusion) under an application
    // ----------------------------------------------------------------------------------

    class SpinBoxEditFieldTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        SpinBoxEditFieldTests()
        {
            m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);

            // As StyleManager::initialize builds it (StyleManager.cpp:244-246); the application owns it
            m_style = new Style(QStyleFactory::create(QStringLiteral("Fusion")));
            QApplication::setStyle(m_style);
        }

        ~SpinBoxEditFieldTests() override
        {
            m_application.reset();
        }

        //! SC_SpinBoxEditField of a spin box whose box is the given rect, asked of the style as QAbstractSpinBox asks it
        QRect editField(const QAbstractSpinBox* spinBox, const QRect& rect) const
        {
            QStyleOptionSpinBox option;
            option.initFrom(spinBox);
            option.rect = rect;
            return m_style->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxEditField, spinBox);
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        AZStd::unique_ptr<QApplication> m_application;
        Style* m_style = nullptr;                           // owned by the application
    };

    // ----------------------------------------------------------------------------------
    // T-SB.1: the edit field of a spin box is inset from its box, whatever height a layout gives the box.
    // SpinBox.qss gave the spin box margin 2 (:13), padding-top -1 (:19), padding-bottom 1 (:18), padding-right 12
    // (:16) and 6px up/down buttons (:81): the field is the contents rect less the widest button on the right
    // (qstylesheetstyle.cpp:5846-5869), (x + 2, y + 1, w - 22, h - 4).
    // ----------------------------------------------------------------------------------

    TEST_F(SpinBoxEditFieldTests, EditField_IsFourLessThanTheBoxWhateverItsHeight)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // 23 is the height of a spin box by default, 18 and 22 those of rows of the inspector
        SpinBox spinBox;
        for (const int height : { 23, 18, 22 })
        {
            const QRect field = editField(&spinBox, QRect(0, 0, 100, height));
            EXPECT_EQ(field, QRect(2, 1, 78, height - 4)) << spinBoxRectText(field);
        }
    }

    TEST_F(SpinBoxEditFieldTests, EditField_FollowsTheOriginOfTheBox)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        SpinBox spinBox;
        const QRect field = editField(&spinBox, QRect(10, 5, 100, 23));
        EXPECT_EQ(field, QRect(12, 6, 78, 19)) << spinBoxRectText(field);
    }

    TEST_F(SpinBoxEditFieldTests, EditField_OfADoubleSpinBoxIsTheSame)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        DoubleSpinBox spinBox;
        for (const int height : { 23, 18 })
        {
            const QRect field = editField(&spinBox, QRect(0, 0, 100, height));
            EXPECT_EQ(field, QRect(2, 1, 78, height - 4)) << spinBoxRectText(field);
        }
    }

    TEST_F(SpinBoxEditFieldTests, EditField_InAVectorElementStartsAtTheEdgeOfTheBox)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // VectorInput.qss:25 `margin-left: 0` of the spin box of a VectorElement; the margin on the right stays
        VectorElement element;
        const QRect field = editField(element.getSpinBox(), QRect(0, 0, 100, 23));
        EXPECT_EQ(field, QRect(0, 1, 80, 19)) << spinBoxRectText(field);
    }

    // ----------------------------------------------------------------------------------
    // T-SB.2: the line edit inside a spin box paints the legacy colors and is sized by its contents
    // ----------------------------------------------------------------------------------

    TEST_F(SpinBoxEditFieldTests, LineEditPalette_TextIsBlackAndGreyWhenDisabled)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        SpinBox spinBox;
        spinBox.ensurePolished();
        const QLineEdit* lineEdit = spinBox.findChild<QLineEdit*>();
        ASSERT_TRUE(lineEdit != nullptr);

        // BaseStyleSheet.qss:77 `color: black` reaches the text and the window text (qstylesheetstyle.cpp:1455-1458);
        // SpinBox.qss:73 gives a disabled spin box #999999. The field itself is the spin box's (transparent here).
        const QPalette palette = lineEdit->palette();
        for (const auto group : { QPalette::Active, QPalette::Inactive })
        {
            EXPECT_TRUE(palette.color(group, QPalette::Text) == QColor(Qt::black));
            EXPECT_TRUE(palette.color(group, QPalette::WindowText) == QColor(Qt::black));
        }
        EXPECT_TRUE(palette.color(QPalette::Disabled, QPalette::Text) == QColor(0x99, 0x99, 0x99));
        EXPECT_TRUE(palette.color(QPalette::Disabled, QPalette::WindowText) == QColor(0x99, 0x99, 0x99));
        EXPECT_EQ(palette.color(QPalette::Active, QPalette::Base).alpha(), 0);
    }

    TEST_F(SpinBoxEditFieldTests, LineEditHint_IsTheSizeOfItsContents)
    {
        ASSERT_TRUE(StyleManager::stylesheetsDisabled());

        // The sheet returned the contents size of a spin box line edit as it was (qstylesheetstyle.cpp:5389-5393):
        // its box is the spin box's, not a field's of 20px
        SpinBox spinBox;
        const QLineEdit* lineEdit = spinBox.findChild<QLineEdit*>();
        ASSERT_TRUE(lineEdit != nullptr);

        QStyleOptionFrame option;
        option.initFrom(lineEdit);
        const QSize hint = m_style->sizeFromContents(QStyle::CT_LineEdit, &option, QSize(50, 15), lineEdit);
        EXPECT_EQ(hint.width(), 50);
        EXPECT_EQ(hint.height(), 15);
    }
} // namespace AzQtComponents
