/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/SheetTwins.h>

#include <AzQtComponents/Components/StyleManager.h>
#include <FlatElements.h>
#include <FlatValues.h>
#include <GemCatalog/GemInfo.h>
#include <GemCatalog/GemModel.h>
#include <GemCatalog/GemRequirementListView.h>

#include <QAbstractScrollArea>
#include <QColor>
#include <QDialog>
#include <QPalette>
#include <QScrollBar>
#include <QWidget>

#include <utility>
#include <vector>

// ----------------------------------------------------------------------------------
// C086: the list of the 'Manual setup is required' dialog (GemRequirementListView.cpp:19, GemRequirementDialog.cpp:56). Its sheet,
// `background-color: #444444;`, has no selector, so it reached the list, its viewport and its scroll bars; the lanes of the bars had
// the fill in every state, the pointer on a bar included, because a widget's sheet outranks the application's `QScrollBar:hover`
// (ScrollBar.qss:76-81). The legacy GUI sets the sheet; the flattened GUI sets none and hands the colour over
// (ScrollBar::setBackgroundColor). The legacy twin is Qt's own style sheet style with the application's real sheet; the flat twin is
// the flattened Style with the module's elements (SheetTwins.h), both on the real class.
// ----------------------------------------------------------------------------------

namespace O3DE::ProjectManager
{
    namespace GemRequirementListFlatSheetTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;
        using AzQtComponents::StyleManager;

        class GemRequirementListFlatSheetTests
            : public ::UnitTest::LeakDetectionFixture
        {
        protected:
            Twins::Context m_context;
        };

        //! The sheet of the site as the file has it (GemRequirementListView.cpp:19 before the flattening)
        constexpr const char* ListSheet = "background-color: #444444;";

        //! The gems of the list: more rows than the list shows, so that it has a scroll bar
        void addGems(GemModel& model)
        {
            for (const char* name : { "GemOne", "GemTwo", "GemThree", "GemFour", "GemFive", "GemSix" })
            {
                GemInfo gem;
                gem.m_name = QString::fromLatin1(name);
                gem.m_displayName = QString::fromLatin1(name);
                gem.m_requirement = QStringLiteral("Needs a manual setup, see the <a href=\"https://o3de.org\">documentation</a>");
                model.AddGem(gem);
            }
        }

        //! The pointer on the vertical bar: the attribute the option's State_MouseOver comes from (a hover event would reach the
        //! watcher of ScrollBar.cpp, which needs the StyleManager)
        void setBarHovered(QWidget& widget, bool entered)
        {
            QScrollBar* bar = static_cast<QAbstractScrollArea&>(widget).verticalScrollBar();
            bar->setAttribute(Qt::WA_UnderMouse, entered);
            bar->update();
        }

        Twins::State barHoveredState()
        {
            return { "scroll bar under the pointer",
                [](QWidget& widget) { setBarHovered(widget, true); },
                [](QWidget& widget) { setBarHovered(widget, false); } };
        }

        Twins::Row requirementListRow(const char* site, std::vector<Twins::State> states, unsigned compare)
        {
            return Twins::Row{
                .m_site = site,
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    // GemRequirementDialog.cpp:56: the list over the model of the gems that need setup, below the dialog
                    auto* model = new GemModel(parent);
                    addGems(*model);
                    auto* list = new GemRequirementListView(model, model->GetSelectionModel(), parent);
                    list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
                    return list;
                },
                // The dialog the list sits in (a QDialog rule fills it, BaseStyleSheet.qss:60-66); a child widget of its class here
                .m_chain = { { .m_name = "gemRequirementDialog",
                    .m_make = [](QWidget* parent) -> QWidget*
                    {
                        auto* dialog = new QDialog(parent);
                        dialog->setWindowFlags(Qt::Widget);
                        return dialog;
                    } } },
                .m_sheets = { { "GemRequirementListView.cpp:19", Twins::SetOn::Widget, ListSheet } },
                .m_realApplicationSheet = true,
                .m_register = &RegisterFlatElements,
                .m_states = std::move(states),
                .m_compare = compare,
                .m_roles = { QPalette::Base, QPalette::Window },
                .m_size = QSize(320, 200),
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        // ------------------------------------------------------------------------------
        // The site: the legacy GUI sets the sheet, the flattened GUI hands the colour over
        // ------------------------------------------------------------------------------

        TEST_F(GemRequirementListFlatSheetTests, Site_TheLegacyRungSetsTheSheetAndTheFlatRungsSetNone)
        {
            GemModel model;
            QWidget host;

            {
                Twins::RungScope rung(Twins::Rung::Qss);
                GemRequirementListView list(&model, model.GetSelectionModel(), &host);
                EXPECT_STREQ(list.styleSheet().toUtf8().constData(), ListSheet);
            }

            for (const Twins::Rung flatRung : { Twins::Rung::Flat, Twins::Rung::Noop, Twins::Rung::Passthrough })
            {
                Twins::RungScope rung(flatRung);
                GemRequirementListView list(&model, model.GetSelectionModel(), &host);
                EXPECT_TRUE(list.styleSheet().isEmpty()) << AzQtComponents::GuiPerfFloor::rungName(flatRung);
            }
        }

        TEST_F(GemRequirementListFlatSheetTests, Site_TheFlatRungHandsTheColourToThePalette)
        {
            GemModel model;
            QWidget host;
            Twins::RungScope rung(Twins::Rung::Flat);
            GemRequirementListView list(&model, model.GetSelectionModel(), &host);
            ASSERT_TRUE(StyleManager::flatStylingActive());

            // The colour the sheet gave the list and the lanes of its bars
            for (const QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled })
            {
                EXPECT_EQ(list.palette().color(group, QPalette::Base), FlatValues::GemRequirementListFill);
                EXPECT_EQ(list.palette().color(group, QPalette::Window), FlatValues::GemRequirementListFill);
            }
        }

        // ------------------------------------------------------------------------------
        // The twins: the list, its rows and its scroll bar, idle and with the pointer on the bar
        // ------------------------------------------------------------------------------

        TEST_F(GemRequirementListFlatSheetTests, List_PaintsAsItDidUnderTheSheet)
        {
            EXPECT_TRUE(check(requirementListRow("C086 the list of the 'Manual setup is required' dialog", {},
                                  Twins::Compare::Geometry | Twins::Compare::Palette | Twins::Compare::Pixels),
                m_context));
        }

        TEST_F(GemRequirementListFlatSheetTests, List_TheLaneOfItsScrollBarDoesNotTintUnderThePointer)
        {
            EXPECT_TRUE(check(requirementListRow("C086 the scroll bar of the list, the pointer on it", { barHoveredState() },
                                  Twins::Compare::Pixels),
                m_context));
        }
    } // namespace GemRequirementListFlatSheetTestsInternal
} // namespace O3DE::ProjectManager
