/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/std/string/string.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Tests/SheetTwins.h>
#include <EditorFlatSheet.h>
#include <EditorToolsApplicationAPI.h>
#include <NewLevelDialog.h>
#include <WelcomeScreen/WelcomeScreenDialog.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QHoverEvent>
#include <QIcon>
#include <QImage>
#include <QListWidget>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QSize>
#include <QString>
#include <QWidget>

#include <string>
#include <vector>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // Two sheets the Editor's dialogs set inline, converted to elements: the rule the New Level dialog sets on itself for its list of
    // templates (NewLevelDialog.cpp:84), and the sheet of the divider at the foot of the welcome dialog (WelcomeScreenDialog.ui:613,
    // set from WelcomeScreenDialog.cpp). The legacy twin is Qt's own style sheet style over Fusion with the real application sheet
    // (BaseStyleSheet.qss and the files it imports) and the sheet where the real GUI sets it; the flat twin is the flattened Style
    // with the elements of EditorFlatSheet and no sheet at all (AzQtComponents SheetTwins.h). The twins build the list and the divider
    // as the dialogs build them (the list with the dialog's own delegate) inside a stand-in that answers to the class name the
    // selectors ask for. The routing, that the flattened GUI sets no sheet at the two call sites, is read from the real dialogs.
    // ----------------------------------------------------------------------------------

    namespace NewLevelWelcomeFlatSheetTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;

        constexpr const char* NewLevelDialogClass = "CNewLevelDialog";
        constexpr const char* NewLevelListElementName = "Editor.ItemView.NewLevelTemplateList";
        constexpr const char* GlobalListElementName = "Global.ItemView.List";
        constexpr const char* WelcomeDialogClass = "WelcomeScreenDialog";
        constexpr const char* WelcomeDividerElementName = "Editor.Frame.WelcomeDivider";
        constexpr const char* GlobalFrameElementName = "Global.Frame.Panel";

        // NewLevelDialog.cpp:84, the rule the dialog sets on itself
        constexpr const char* NewLevelDialogRule =
            "QListWidget::item {height: 148px; padding-left: 0px; padding-right: 0px; background-color: transparent;}";

        // WelcomeScreenDialog.ui:613, the sheet the form gave the divider (set from WelcomeScreenDialog.cpp)
        constexpr const char* WelcomeDividerRule = "color: \"black\"";

        constexpr int TemplateItemHeight = 148;                         // height: NewLevelDialog.cpp:84
        constexpr int TemplateCount = 4;
        constexpr int CurrentTemplate = 1;                              // the default template of the dialog is selected
        constexpr QSize TemplateListSize(440, 330);                     // the list of the dialog (NewLevelDialog.ui:4, :15) shows two rows
        constexpr QSize NewLevelRootSize(471, 371);                     // the size of the dialog (NewLevelDialog.ui:9-10)
        constexpr QSize DividerSize(120, 1);                            // the divider is 1px high (WelcomeScreenDialog.ui:605-610)

        // ------------------------------------------------------------------------------
        // The sites, built as the code builds them
        // ------------------------------------------------------------------------------

        //! A template icon: a colour with a diagonal, so that a shifted or scaled icon is a different picture
        QIcon templateIcon(int index)
        {
            QPixmap pixmap(128, 128);
            pixmap.fill(QColor(40 + index * 50, 160 - index * 30, 200 - index * 40));
            QPainter painter(&pixmap);
            painter.setPen(QPen(Qt::white, 6));
            painter.drawLine(0, 0, 127, 127);
            painter.end();
            return QIcon(pixmap);
        }

        //! The list of templates as the dialog builds it: the form's settings (NewLevelDialog.ui:15-33) and CNewLevelDialog::
        //! InitTemplateListWidget (NewLevelDialog.cpp:132-198), with the dialog's own delegate
        QWidget* makeTemplateList(QWidget* parent)
        {
            auto* list = new QListWidget(parent);
            list->setObjectName(QStringLiteral("listTemplates"));
            list->setMovement(QListView::Static);
            list->setResizeMode(QListView::Adjust);
            list->setSpacing(4);
            list->setViewMode(QListView::IconMode);
            list->setUniformItemSizes(true);
            list->setSelectionRectVisible(false);

            const char* const names[TemplateCount] = { "Default_Level", "Terrain", "Empty", "Interior_Template" };
            for (int index = 0; index < TemplateCount; ++index)
            {
                auto* item = new QListWidgetItem(QString::fromLatin1(names[index]));
                item->setIcon(templateIcon(index));
                list->addItem(item);
            }

            list->setViewMode(QListWidget::IconMode);
            list->setIconSize(QSize(128, 128));
            list->setDragDropMode(QAbstractItemView::NoDragDrop);
            list->setItemDelegate(new DrawIconWithBackgroundDelegate(list));
            list->setCurrentRow(CurrentTemplate);

            // The scroll bar of a view is the Global scroll bar element's (ScrollBar.qss), which the dialog's rule does not decide: the
            // twins compare the items, so the view has none
            list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            return list;
        }

        //! The divider as the form builds it (WelcomeScreenDialog.ui:605-621): a Line, so a QFrame, Plain, line width 0, 1px high
        QWidget* makeDivider(QWidget* parent)
        {
            auto* line = new QFrame(parent);
            line->setObjectName(QStringLiteral("line_4"));
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Plain);
            line->setLineWidth(0);
            line->setMaximumSize(QSize(16777215, 1));
            return line;
        }

        // ------------------------------------------------------------------------------
        // The rows
        // ------------------------------------------------------------------------------

        //! The pointer over an item of the list (the `:hover` of the application's item rule), as Qt delivers it to the viewport
        Twins::State hoverItem(int row)
        {
            const auto send = [row](QWidget& widget, bool inside)
            {
                auto* list = qobject_cast<QListWidget*>(&widget);
                QWidget* viewport = list->viewport();
                const QPointF point = list->visualItemRect(list->item(row)).center();
                const QPointF outside(-1, -1);
                QHoverEvent hover(inside ? QEvent::HoverMove : QEvent::HoverLeave, inside ? point : outside,
                    viewport->mapToGlobal(inside ? point : outside), inside ? outside : point);
                QApplication::sendEvent(viewport, &hover);
                viewport->update();
            };
            return { "hover item " + std::to_string(row), [send](QWidget& widget) { send(widget, true); },
                [send](QWidget& widget) { send(widget, false); } };
        }

        //! The template list in a dialog of the class given, with the rule the dialog sets on itself
        Twins::Row templateListRow(const char* dialogClass)
        {
            return Twins::Row{
                .m_site = "C005 the template list of the New Level dialog",
                .m_make = &makeTemplateList,
                .m_chain = { { .m_name = "CNewLevelDialog", .m_className = dialogClass } },
                .m_sheets = { { "NewLevelDialog.cpp:84", Twins::SetOn::Ancestor, NewLevelDialogRule, 0 } },
                .m_realApplicationSheet = true,
                .m_register = &EditorFlatSheet::RegisterElements,
                .m_states = { hoverItem(CurrentTemplate), hoverItem(0), Twins::States::focus() },
                .m_compare = Twins::Compare::Geometry | Twins::Compare::Palette | Twins::Compare::Pixels,
                .m_roles = { QPalette::Highlight, QPalette::HighlightedText, QPalette::AlternateBase, QPalette::Text,
                    QPalette::WindowText },
                .m_size = TemplateListSize,
                .m_rootSize = NewLevelRootSize,
            };
        }

        //! The divider in a dialog of the class given, with the sheet the form gave it
        Twins::Row dividerRow(const char* dialogClass)
        {
            return Twins::Row{
                .m_site = "U007 the divider at the foot of the welcome dialog",
                .m_make = &makeDivider,
                .m_chain = { { .m_name = "WelcomeScreenDialog", .m_className = dialogClass } },
                .m_sheets = { { "WelcomeScreenDialog.ui:613", Twins::SetOn::Widget, WelcomeDividerRule } },
                .m_realApplicationSheet = true,
                .m_register = &EditorFlatSheet::RegisterElements,
                .m_compare = Twins::Compare::Geometry | Twins::Compare::Pixels,
                .m_size = DividerSize,
                .m_mayDrawNothing = true,           // a line of 1px is one colour at the ratio 1: the pictures at 1.5 and 2 hold two rows
            };
        }

        // ------------------------------------------------------------------------------
        // Reading a twin
        // ------------------------------------------------------------------------------

        //! One twin of a row, built, read idle and in each state, and gone
        std::vector<Twins::Reading> readTwin(const Twins::Row& row, Twins::Context& context, bool legacy)
        {
            Twins::Context::registerOnce(row.m_register);
            Twins::RungScope rung(legacy ? row.m_legacyRung : Twins::Rung::Flat);
            const Twins::Twin twin(row, context, legacy);
            return Twins::readStates(row, *twin.widget(), context, legacy, nullptr);
        }

        //! Where the items of the list sit, and what the list asks for each, as the layout of the view made them
        struct ItemFacts
        {
            std::vector<QRect> m_rects;
            std::vector<QSize> m_hints;
            std::string m_itemViewElement;
        };

        ItemFacts readItems(const Twins::Row& row, Twins::Context& context, bool legacy)
        {
            Twins::Context::registerOnce(row.m_register);
            Twins::RungScope rung(legacy ? row.m_legacyRung : Twins::Rung::Flat);
            const Twins::Twin twin(row, context, legacy);
            auto* list = qobject_cast<QListWidget*>(twin.widget());
            ItemFacts facts;
            for (int index = 0; index < list->count(); ++index)
            {
                facts.m_rects.push_back(list->visualItemRect(list->item(index)));
                facts.m_hints.push_back(list->sizeHintForIndex(list->model()->index(index, 0)));
            }
            const AzQtComponents::ItemViewElement* element = AzQtComponents::ItemViewElement::forWidget(list);
            facts.m_itemViewElement = element ? element->m_name : "";
            return facts;
        }

        //! The element of a widget in the flat twin of a row, by name: the Global one, or the Editor's
        std::string frameElementOf(const Twins::Row& row, Twins::Context& context)
        {
            Twins::Context::registerOnce(row.m_register);
            Twins::RungScope rung(Twins::Rung::Flat);
            const Twins::Twin twin(row, context, false);
            const AzQtComponents::FrameElement* element = AzQtComponents::FrameElement::forWidget(twin.widget());
            return element ? element->m_name : "";
        }

        // ------------------------------------------------------------------------------
        // The real dialogs
        // ------------------------------------------------------------------------------

        //! The Editor's application bus, as far as the welcome dialog asks it: the extension of a level file, at the end of its
        //! constructor. The dialog asserts when nobody answers.
        class LevelExtensionHandler
            : public EditorInternal::EditorToolsApplicationRequests::Bus::Handler
        {
        public:
            LevelExtensionHandler()
            {
                BusConnect();
            }

            ~LevelExtensionHandler() override
            {
                BusDisconnect();
            }

            bool OpenLevel(AZStd::string_view) override
            {
                return false;
            }

            bool OpenLevelNoPrompt(AZStd::string_view) override
            {
                return false;
            }

            int CreateLevel(AZStd::string_view, AZStd::string_view, bool) override
            {
                return 0;
            }

            int CreateLevelNoPrompt(AZStd::string_view, AZStd::string_view, int, bool) override
            {
                return 0;
            }

            AZStd::string GetGameFolder() const override
            {
                return {};
            }

            AZStd::string GetCurrentLevelName() const override
            {
                return {};
            }

            AZStd::string GetCurrentLevelPath() const override
            {
                return {};
            }

            const char* GetOldCryLevelExtension() const override
            {
                return ".ly";
            }

            const char* GetLevelExtension() const override
            {
                return ".prefab";
            }

            void Exit() override
            {
            }

            void ExitNoPrompt() override
            {
            }
        };

        //! The real welcome dialog. The window decoration it is made inside of deletes itself when the dialog is gone, later: the
        //! holder lets that happen before it ends.
        class WelcomeDialogHolder
        {
        public:
            WelcomeDialogHolder()
                : m_dialog(new WelcomeScreenDialog())
            {
            }

            ~WelcomeDialogHolder()
            {
                delete m_dialog;
                QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            }

            WelcomeDialogHolder(const WelcomeDialogHolder&) = delete;
            WelcomeDialogHolder& operator=(const WelcomeDialogHolder&) = delete;

            //! The divider at the foot of the body (WelcomeScreenDialog.ui:613)
            QFrame* divider() const
            {
                return m_dialog->findChild<QFrame*>(QStringLiteral("line_4"));
            }

        private:
            WelcomeScreenDialog* m_dialog;
        };

        class NewLevelWelcomeFlatSheetTests
            : public LeakDetectionFixture
        {
        protected:
            Twins::Context m_context;
        };
    } // namespace NewLevelWelcomeFlatSheetTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using NewLevelWelcomeFlatSheetTestsInternal::NewLevelWelcomeFlatSheetTests;
    namespace NewLevelWelcome = NewLevelWelcomeFlatSheetTestsInternal;

    // ----------------------------------------------------------------------------------
    // C005: the template list of the New Level dialog
    // ----------------------------------------------------------------------------------

    TEST_F(NewLevelWelcomeFlatSheetTests, TemplateList_TheElementAloneGivesTheItemsOfTheDialogsRule)
    {
        // The flat twin has no sheet (the harness flags one), and the list of the dialog is laid out, sized, coloured and painted as under
        // the rule in every state: 148px high items with no padding, no fill under the pointer or on the selected one
        EXPECT_TRUE(NewLevelWelcome::Twins::check(NewLevelWelcome::templateListRow(NewLevelWelcome::NewLevelDialogClass), m_context));
    }

    TEST_F(NewLevelWelcomeFlatSheetTests, TemplateList_TheItemsHaveTheSizeAndTheTopLeftsOfTheRule)
    {
        const NewLevelWelcome::Twins::Row row = NewLevelWelcome::templateListRow(NewLevelWelcome::NewLevelDialogClass);
        const NewLevelWelcome::ItemFacts legacy = NewLevelWelcome::readItems(row, m_context, true);
        const NewLevelWelcome::ItemFacts flat = NewLevelWelcome::readItems(row, m_context, false);

        ASSERT_EQ(legacy.m_rects.size(), size_t(NewLevelWelcome::TemplateCount));
        ASSERT_EQ(flat.m_rects.size(), legacy.m_rects.size());
        EXPECT_EQ(flat.m_itemViewElement, NewLevelWelcome::NewLevelListElementName);
        for (size_t index = 0; index < legacy.m_rects.size(); ++index)
        {
            EXPECT_EQ(legacy.m_hints[index].height(), NewLevelWelcome::TemplateItemHeight) << "legacy item " << index;
            EXPECT_EQ(flat.m_hints[index], legacy.m_hints[index]) << "item " << index;
            EXPECT_EQ(flat.m_rects[index], legacy.m_rects[index]) << "item " << index;
        }
    }

    TEST_F(NewLevelWelcomeFlatSheetTests, TemplateList_APointerOverAnItemIsTheStateTheRuleChangesNothingIn)
    {
        // The pointer drives the application's `::item:hover` rule: under the application's rules alone the hovered item has a fill the
        // idle one does not, so the pictures of the hover states of the row prove that the dialog's rule takes that fill away
        NewLevelWelcome::Twins::Row withoutRule = NewLevelWelcome::templateListRow(NewLevelWelcome::NewLevelDialogClass);
        withoutRule.m_sheets.clear();
        withoutRule.m_realApplicationSheet = true;
        const std::vector<NewLevelWelcome::Twins::Reading> legacy = NewLevelWelcome::readTwin(withoutRule, m_context, true);
        ASSERT_EQ(legacy.size(), size_t(4));        // at rest, the pointer over each of two items, the list focused in an active window
        EXPECT_TRUE(NewLevelWelcome::Twins::readingsDiffer(legacy[0], legacy[2])) << "the hover rule of the application changes item 0";

        const NewLevelWelcome::Twins::Row row = NewLevelWelcome::templateListRow(NewLevelWelcome::NewLevelDialogClass);
        const std::vector<NewLevelWelcome::Twins::Reading> ruled = NewLevelWelcome::readTwin(row, m_context, true);
        ASSERT_EQ(ruled.size(), size_t(4));
        EXPECT_FALSE(NewLevelWelcome::Twins::readingsDiffer(ruled[0], ruled[2]))
            << "under the dialog's rule the hovered item looks as at rest";
    }

    TEST_F(NewLevelWelcomeFlatSheetTests, TemplateList_AListOfAnotherDialogKeepsTheGlobalItemsAndDiffersFromTheRule)
    {
        // Without the dialog's class above it the list has the Global element's items (24px high, 4px of padding), which the rule's
        // 148px and no padding are not: the twin fails there, so the equality above is the element's doing
        NewLevelWelcome::Twins::Row other = NewLevelWelcome::templateListRow("CSomeOtherDialog");
        other.m_expectedDifference = "pixels at ratio 1";
        EXPECT_TRUE(NewLevelWelcome::Twins::check(other, m_context));

        const NewLevelWelcome::ItemFacts flat = NewLevelWelcome::readItems(other, m_context, false);
        EXPECT_EQ(flat.m_itemViewElement, NewLevelWelcome::GlobalListElementName);
    }

    TEST_F(NewLevelWelcomeFlatSheetTests, TemplateList_TheFlatGuiSetsNoSheetOnTheRealDialogAndTheLegacyGuiSetsTheRule)
    {
        // The routing of NewLevelDialog.cpp:83: the element carries the items, so the flattened GUI sets no sheet on the dialog
        // (a sheet makes Qt's style sheet style the dialog's style), and the legacy GUI sets the rule as it always did
        {
            NewLevelWelcome::Twins::RungScope rung(NewLevelWelcome::Twins::Rung::Flat);
            CNewLevelDialog dialog;
            EXPECT_TRUE(dialog.styleSheet().isEmpty()) << dialog.styleSheet().toStdString();
        }
        {
            NewLevelWelcome::Twins::RungScope rung(NewLevelWelcome::Twins::Rung::Qss);
            CNewLevelDialog dialog;
            EXPECT_EQ(dialog.styleSheet(), QString::fromLatin1(NewLevelWelcome::NewLevelDialogRule));
        }
    }

    // ----------------------------------------------------------------------------------
    // U007: the divider at the foot of the welcome dialog
    // ----------------------------------------------------------------------------------

    TEST_F(NewLevelWelcomeFlatSheetTests, Divider_TheFlatGuiSetsNoSheetOnTheRealDividerAndTheLegacyGuiSetsTheFormsSheet)
    {
        // The routing of WelcomeScreenDialog.cpp:79: the form no longer holds the sheet; the legacy GUI sets its text on the divider
        // after setupUi, and the flattened GUI sets none
        const NewLevelWelcome::LevelExtensionHandler extension;
        {
            NewLevelWelcome::Twins::RungScope rung(NewLevelWelcome::Twins::Rung::Flat);
            const NewLevelWelcome::WelcomeDialogHolder holder;
            ASSERT_NE(holder.divider(), nullptr);
            EXPECT_TRUE(holder.divider()->styleSheet().isEmpty()) << holder.divider()->styleSheet().toStdString();
        }
        {
            NewLevelWelcome::Twins::RungScope rung(NewLevelWelcome::Twins::Rung::Qss);
            const NewLevelWelcome::WelcomeDialogHolder holder;
            ASSERT_NE(holder.divider(), nullptr);
            EXPECT_EQ(holder.divider()->styleSheet(), QString::fromLatin1(NewLevelWelcome::WelcomeDividerRule));
        }
    }

    TEST_F(NewLevelWelcomeFlatSheetTests, Divider_TheElementAloneDrawsTheBlackLineOfTheFormsSheet)
    {
        EXPECT_TRUE(NewLevelWelcome::Twins::check(NewLevelWelcome::dividerRow(NewLevelWelcome::WelcomeDialogClass), m_context));
    }

    TEST_F(NewLevelWelcomeFlatSheetTests, Divider_ItIsTheDividerOfTheWelcomeDialogAndNoOtherFrame)
    {
        EXPECT_EQ(NewLevelWelcome::frameElementOf(NewLevelWelcome::dividerRow(NewLevelWelcome::WelcomeDialogClass), m_context),
            NewLevelWelcome::WelcomeDividerElementName);
        EXPECT_EQ(NewLevelWelcome::frameElementOf(NewLevelWelcome::dividerRow("CSomeOtherDialog"), m_context),
            NewLevelWelcome::GlobalFrameElementName);
    }

    TEST_F(NewLevelWelcomeFlatSheetTests, Divider_TheLineIsBlackInBothGuisAndWhiteWithoutTheElement)
    {
        const NewLevelWelcome::Twins::Row row = NewLevelWelcome::dividerRow(NewLevelWelcome::WelcomeDialogClass);
        const std::vector<NewLevelWelcome::Twins::Reading> legacy = NewLevelWelcome::readTwin(row, m_context, true);
        const std::vector<NewLevelWelcome::Twins::Reading> flat = NewLevelWelcome::readTwin(row, m_context, false);
        ASSERT_FALSE(legacy[0].m_pictures.empty());
        ASSERT_FALSE(flat[0].m_pictures.empty());
        EXPECT_EQ(legacy[0].m_pictures[0].m_image.pixel(0, 0) & 0xFFFFFF, 0x000000u) << "the legacy line at ratio 1";
        EXPECT_EQ(flat[0].m_pictures[0].m_image.pixel(0, 0) & 0xFFFFFF, 0x000000u) << "the flat line at ratio 1";

        // The Global frame draws the line in the universal rule's white: the sheet's black is what the element carries
        const std::vector<NewLevelWelcome::Twins::Reading> global =
            NewLevelWelcome::readTwin(NewLevelWelcome::dividerRow("CSomeOtherDialog"), m_context, false);
        EXPECT_EQ(global[0].m_pictures[0].m_image.pixel(0, 0) & 0xFFFFFF, 0xFFFFFFu) << "the Global frame's line at ratio 1";
    }

    TEST_F(NewLevelWelcomeFlatSheetTests, Divider_AFrameOfAnotherDialogHasTheWhiteLineAndDiffersFromTheSheet)
    {
        NewLevelWelcome::Twins::Row other = NewLevelWelcome::dividerRow("CSomeOtherDialog");
        other.m_expectedDifference = "pixels at ratio 1";
        EXPECT_TRUE(NewLevelWelcome::Twins::check(other, m_context));
    }
} // namespace UnitTest
