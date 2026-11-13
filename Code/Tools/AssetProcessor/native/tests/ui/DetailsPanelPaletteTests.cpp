/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <native/ui/AssetDetailsPanelPalettes.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QApplication>
#include <QBrush>
#include <QFrame>
#include <QPalette>

#include <algorithm>
#include <iterator>

// ----------------------------------------------------------------------------------
// The palettes the .ui forms of the two asset details panels gave their "AssetDetailsSeparator" line (<palette>,
// ProductAssetDetailsPanel.ui and SourceAssetDetailsPanel.ui, lines 58-175 of each). The forms no longer hold them: the panels set the
// palettes of native/ui/AssetDetailsPanelPalettes.h in code right after setupUi. The panels are classes of the application target, so
// the line is built here the way the forms built it (a plain horizontal QFrame) and given the palette the panels give it. Every value
// below is quoted from the upstream form, with the line of its <color> element (the line numbers are those of the .ui at git HEAD,
// before the flatten work).
// ----------------------------------------------------------------------------------

namespace AssetProcessor
{
    namespace DetailsPanelPaletteTestsInternal
    {
        // ------------------------------------------------------------------------------
        // What the forms held: one <colorrole> of the <palette>
        // ------------------------------------------------------------------------------

        struct FormBrush
        {
            QPalette::ColorGroup m_group;
            QPalette::ColorRole m_role;
            int m_uiLine;       // the line of its <color alpha="..."> element in the .ui
            int m_alpha;
            int m_red;
            int m_green;
            int m_blue;
        };

        // ProductAssetDetailsPanel.ui:58-175. Every <brush> was brushstyle="SolidPattern"
        constexpr FormBrush ProductForm[] = {
            // <active> (:60-97)
            { QPalette::Active, QPalette::WindowText, 63, 255, 255, 255, 255 },
            { QPalette::Active, QPalette::Button, 72, 255, 87, 87, 87 },
            { QPalette::Active, QPalette::Base, 81, 255, 87, 87, 87 },
            { QPalette::Active, QPalette::Window, 90, 255, 87, 87, 87 },
            // <inactive> (:98-135)
            { QPalette::Inactive, QPalette::WindowText, 101, 255, 255, 255, 255 },
            { QPalette::Inactive, QPalette::Button, 110, 255, 87, 87, 87 },
            { QPalette::Inactive, QPalette::Base, 119, 255, 87, 87, 87 },
            { QPalette::Inactive, QPalette::Window, 128, 255, 87, 87, 87 },
            // <disabled> (:136-173)
            { QPalette::Disabled, QPalette::WindowText, 139, 255, 120, 120, 120 },
            { QPalette::Disabled, QPalette::Button, 148, 255, 87, 87, 87 },
            { QPalette::Disabled, QPalette::Base, 157, 255, 87, 87, 87 },
            { QPalette::Disabled, QPalette::Window, 166, 255, 87, 87, 87 },
        };

        // SourceAssetDetailsPanel.ui:58-175. Every <brush> was brushstyle="SolidPattern"; the Active WindowText has alpha 200
        constexpr FormBrush SourceForm[] = {
            // <active> (:60-97)
            { QPalette::Active, QPalette::WindowText, 63, 200, 200, 200, 200 },
            { QPalette::Active, QPalette::Button, 72, 255, 87, 87, 87 },
            { QPalette::Active, QPalette::Base, 81, 255, 87, 87, 87 },
            { QPalette::Active, QPalette::Window, 90, 255, 87, 87, 87 },
            // <inactive> (:98-135)
            { QPalette::Inactive, QPalette::WindowText, 101, 255, 255, 255, 255 },
            { QPalette::Inactive, QPalette::Button, 110, 255, 87, 87, 87 },
            { QPalette::Inactive, QPalette::Base, 119, 255, 87, 87, 87 },
            { QPalette::Inactive, QPalette::Window, 128, 255, 87, 87, 87 },
            // <disabled> (:136-173)
            { QPalette::Disabled, QPalette::WindowText, 139, 255, 120, 120, 120 },
            { QPalette::Disabled, QPalette::Button, 148, 255, 87, 87, 87 },
            { QPalette::Disabled, QPalette::Base, 157, 255, 87, 87, 87 },
            { QPalette::Disabled, QPalette::Window, 166, 255, 87, 87, 87 },
        };

        // ------------------------------------------------------------------------------
        // Fixture: an application, as the other tests of the folder make it
        // ------------------------------------------------------------------------------

        class DetailsPanelPaletteTests
            : public ::UnitTest::LeakDetectionFixture
        {
        public:
            DetailsPanelPaletteTests()
            {
                // Before the application: its construction is where Qt warns
                s_previousHandler = qInstallMessageHandler(&DetailsPanelPaletteTests::messageHandler);
                if (QCoreApplication::instance() == nullptr)
                {
                    m_application = AZStd::make_unique<QApplication>(m_argc, m_argv);
                }
            }

            ~DetailsPanelPaletteTests() override
            {
                m_application.reset();
                qInstallMessageHandler(s_previousHandler);
            }

            //! The test environment fails a test on any Qt warning (BaseAssetProcessorTest.h:20-36). From the second application of
            //! the process on, Qt's Windows platform warns that the window class of its theme observer exists already: Qt's own,
            //! nothing the palettes did. That one warning is let through; every other message goes to the environment's handler.
            static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
            {
                if (type == QtWarningMsg && message.contains(QLatin1String("ThemeChangeObserverWindow")))
                {
                    return;
                }
                if (s_previousHandler)
                {
                    s_previousHandler(type, context, message);
                }
            }

            static inline QtMessageHandler s_previousHandler = nullptr;

        protected:
            int m_argc = 1;                                     // QApplication keeps a reference to it
            char m_programName[5] = "test";
            char* m_argv[2] = { m_programName, nullptr };
            AZStd::unique_ptr<QApplication> m_application;
        };

        // ------------------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------------------

        //! The brush of the palette at the form's group and role is what the form's <color> held: solid, with its alpha and channels
        void expectFormBrush(const QPalette& palette, const FormBrush& form, const char* panel)
        {
            const QBrush brush = palette.brush(form.m_group, form.m_role);
            EXPECT_EQ(brush.style(), Qt::SolidPattern) << panel << ".ui:" << form.m_uiLine;
            EXPECT_EQ(brush.color().rgba(), qRgba(form.m_red, form.m_green, form.m_blue, form.m_alpha)) << panel << ".ui:" << form.m_uiLine;
        }

        //! Every brush of the form, in every group and role it held
        template<size_t Count>
        void expectFormBrushes(const QPalette& palette, const FormBrush (&form)[Count], const char* panel)
        {
            for (const FormBrush& brush : form)
            {
                expectFormBrush(palette, brush, panel);
            }
        }

        //! The form set those brushes and no other: the palette sets exactly those, and the rest of every group is the application's
        template<size_t Count>
        void expectNoOtherBrush(const QPalette& palette, const FormBrush (&form)[Count], const char* panel)
        {
            const QPalette untouched;
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                for (int roleIndex = 0; roleIndex < QPalette::NColorRoles; ++roleIndex)
                {
                    const QPalette::ColorRole role = static_cast<QPalette::ColorRole>(roleIndex);
                    const bool inForm = std::any_of(std::begin(form), std::end(form), [group, role](const FormBrush& brush)
                        {
                            return brush.m_group == group && brush.m_role == role;
                        });
                    EXPECT_EQ(palette.isBrushSet(group, role), inForm) << panel << " group " << group << " role " << roleIndex;
                    if (!inForm)
                    {
                        EXPECT_TRUE(palette.brush(group, role) == untouched.brush(group, role))
                            << panel << " group " << group << " role " << roleIndex;
                    }
                }
            }
        }

        //! The line the forms built, a horizontal plain QFrame under the panel, with the palette the panel gives it
        QFrame* makeSeparator(QFrame& panel, const QPalette& palette)
        {
            auto* separator = new QFrame(&panel);
            separator->setObjectName(QStringLiteral("AssetDetailsSeparator"));
            separator->setFrameShape(QFrame::HLine);
            separator->setFrameShadow(QFrame::Plain);
            separator->setPalette(palette);
            return separator;
        }

        // ------------------------------------------------------------------------------
        // ProductAssetDetailsPanel
        // ------------------------------------------------------------------------------

        TEST_F(DetailsPanelPaletteTests, ProductPanel_TheFormHeldTwelveBrushesAndThePaletteHoldsThem)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);
            EXPECT_EQ(std::size(ProductForm), 12u);

            const QPalette palette = ProductAssetDetailsPanelFlat::SeparatorPalette();
            expectFormBrushes(palette, ProductForm, "ProductAssetDetailsPanel");
            expectNoOtherBrush(palette, ProductForm, "ProductAssetDetailsPanel");
        }

        TEST_F(DetailsPanelPaletteTests, ProductPanel_TheSeparatorLineHasThePaletteOfTheForm)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QFrame panel;
            const QFrame* separator = makeSeparator(panel, ProductAssetDetailsPanelFlat::SeparatorPalette());
            expectFormBrushes(separator->palette(), ProductForm, "ProductAssetDetailsPanel");
        }

        // ------------------------------------------------------------------------------
        // SourceAssetDetailsPanel (the source and the intermediate assets panels)
        // ------------------------------------------------------------------------------

        TEST_F(DetailsPanelPaletteTests, SourcePanel_TheFormHeldTwelveBrushesAndThePaletteHoldsThem)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);
            EXPECT_EQ(std::size(SourceForm), 12u);

            const QPalette palette = SourceAssetDetailsPanelFlat::SeparatorPalette();
            expectFormBrushes(palette, SourceForm, "SourceAssetDetailsPanel");
            expectNoOtherBrush(palette, SourceForm, "SourceAssetDetailsPanel");
        }

        TEST_F(DetailsPanelPaletteTests, SourcePanel_TheSeparatorLineHasThePaletteOfTheForm)
        {
            ASSERT_TRUE(qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr);

            QFrame panel;
            const QFrame* separator = makeSeparator(panel, SourceAssetDetailsPanelFlat::SeparatorPalette());
            expectFormBrushes(separator->palette(), SourceForm, "SourceAssetDetailsPanel");
        }
    } // namespace DetailsPanelPaletteTestsInternal
} // namespace AssetProcessor
