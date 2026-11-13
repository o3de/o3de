/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Tests/SheetTwins.h>

#include <QApplication>
#include <QFrame>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // A capability of BoxElement, tested on its own (the site that uses it is twinned against the legacy sheet in
    // ProgressShieldFlatSheetTests): the fill as the palette of the widget (BoxElement::m_fillsPalette): QStyleSheetStyle::setPalette
    // gave the widget the fill of its rule as the Base, Button and Window of its palette.
    // ----------------------------------------------------------------------------------

    namespace BoxElementPaletteTestsInternal
    {
        using namespace SheetTwins;
        using Rung = GuiPerfFloor::Rung;

        class BoxElementPaletteTests
            : public ::UnitTest::LeakDetectionFixture
        {
        protected:
            Context m_context;
        };

        //! The fill of the palette: a gradient, which only the palette can give a top-level window
        const BoxElement PaletteFill{
            .m_name = "Test.Box.PaletteFill",
            .m_idle = { .m_fill = BoxElement::vertical(QColor(0, 0, 0, 50), QColor(0, 0, 0, 90)) },
            .m_fillsPalette = true,
        };

        const BoxElement PaletteNoFlag{
            .m_name = "Test.Box.PaletteNoFlag",
            .m_idle = { .m_fill = QBrush(QColor(0x12, 0x34, 0x56)) },
        };

        bool isPaletteFill(const QWidget* widget)
        {
            return Selectors::named(widget, "runtimePaletteFill");
        }

        bool isPaletteNoFlag(const QWidget* widget)
        {
            return Selectors::named(widget, "runtimePaletteNoFlag");
        }

        //! Registered once for the process, as a module registers its own
        void registerElements()
        {
            static bool registered = false;
            if (!registered)
            {
                registered = true;
                BoxElement::registerElement(&PaletteFill, &QFrame::staticMetaObject, &isPaletteFill);
                BoxElement::registerElement(&PaletteNoFlag, &QFrame::staticMetaObject, &isPaletteNoFlag);
            }
        }

        //! A frame the style and the elements reach, polished
        QFrame* makeFrame(QWidget& parent, const char* name, const Context& context)
        {
            auto* frame = new QFrame(&parent);
            frame->setObjectName(QString::fromLatin1(name));
            frame->setStyle(context.flatStyle());
            frame->ensurePolished();
            return frame;
        }
    } // namespace BoxElementPaletteTestsInternal

    using BoxElementPaletteTestsInternal::BoxElementPaletteTests;
    namespace PaletteTest = BoxElementPaletteTestsInternal;

    // ----------------------------------------------------------------------------------
    // The fill as the palette
    // ----------------------------------------------------------------------------------

    TEST_F(BoxElementPaletteTests, FillsPalette_TheIdleFillIsTheBaseButtonAndWindowOfTheWidgetInEveryGroup)
    {
        PaletteTest::RungScope flat(PaletteTest::Rung::Flat);
        PaletteTest::registerElements();
        QWidget root;
        QFrame* frame = PaletteTest::makeFrame(root, "runtimePaletteFill", m_context);

        ASSERT_EQ(BoxElement::forWidget(frame), &PaletteTest::PaletteFill);
        const QBrush fill = PaletteTest::PaletteFill.m_idle.m_fill;
        for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            for (const QPalette::ColorRole role : { QPalette::Base, QPalette::Button, QPalette::Window })
            {
                EXPECT_EQ(frame->palette().brush(group, role), fill) << group << " " << role;
            }
            // The text roles are no part of the fill
            EXPECT_NE(frame->palette().brush(group, QPalette::WindowText), fill) << group;
        }
    }

    TEST_F(BoxElementPaletteTests, FillsPalette_TheRoleTheWidgetPaintsItsBackgroundWithIsFilledToo)
    {
        // QRenderRule::configurePalette: the rule's background is also the palette role of QWidget::backgroundRole, whichever it is
        PaletteTest::RungScope flat(PaletteTest::Rung::Flat);
        PaletteTest::registerElements();
        QWidget root;
        auto* frame = new QFrame(&root);
        frame->setObjectName(QStringLiteral("runtimePaletteFill"));
        frame->setBackgroundRole(QPalette::AlternateBase);
        frame->setStyle(m_context.flatStyle());
        frame->ensurePolished();

        ASSERT_EQ(BoxElement::forWidget(frame), &PaletteTest::PaletteFill);
        const QBrush fill = PaletteTest::PaletteFill.m_idle.m_fill;
        for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
        {
            EXPECT_EQ(frame->palette().brush(group, QPalette::AlternateBase), fill) << group;
            EXPECT_EQ(frame->palette().brush(group, QPalette::Window), fill) << group;
        }

        // A widget with the role of an ordinary background has no role beyond the three
        QFrame* ordinary = PaletteTest::makeFrame(root, "runtimePaletteFill", m_context);
        EXPECT_NE(ordinary->palette().brush(QPalette::Active, QPalette::AlternateBase), fill);
    }

    TEST_F(BoxElementPaletteTests, FillsPalette_AnotherPolishSetsTheSamePaletteAndAnElementWithoutTheFlagSetsNone)
    {
        PaletteTest::RungScope flat(PaletteTest::Rung::Flat);
        PaletteTest::registerElements();
        QWidget root;
        QFrame* filled = PaletteTest::makeFrame(root, "runtimePaletteFill", m_context);
        QFrame* plain = PaletteTest::makeFrame(root, "runtimePaletteNoFlag", m_context);

        const QPalette before = filled->palette();
        filled->style()->unpolish(filled);
        filled->style()->polish(filled);
        EXPECT_EQ(filled->palette(), before);

        ASSERT_EQ(BoxElement::forWidget(plain), &PaletteTest::PaletteNoFlag);
        EXPECT_EQ(
            plain->palette().brush(QPalette::Active, QPalette::Window), QApplication::palette().brush(QPalette::Active, QPalette::Window));
        EXPECT_NE(plain->palette().brush(QPalette::Active, QPalette::Window), PaletteTest::PaletteNoFlag.m_idle.m_fill);
    }
} // namespace AzQtComponents
