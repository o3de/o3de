/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/MockComponentApplication.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzQtComponents/Tests/SiteTwins.h>

#include <Window/ParticleEditorFlat.h>
#include <Window/ParticleGraphicsView.h>
#include <Window/ParticleItemWidget.h>
#include <Window/ParticleLineWidget.h>

#include <QApplication>
#include <QCheckBox>
#include <QFrame>
#include <QLineEdit>
#include <QMenu>
#include <QStringList>
#include <QToolButton>
#include <QWidget>

#include <string>
#include <vector>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // The flattened Particle Editor (ParticleEditorFlat): the sheets that its widgets gave themselves, in their forms and in their code,
    // carried by elements. The legacy twin of each site is Qt's own style sheet style over the base style, with the real application
    // sheet (BaseStyleSheet.qss and the files it imports) and the sheets the real classes set; the flat twin is the flattened Style with
    // the elements of ParticleEditorFlat and no sheet at all (AzQtComponents SheetTwins.h). The classes under test are the real ones:
    // the emitter window (and with it the twelve rows, the title row, the check boxes, the tool buttons and the four lines of its form),
    // the graph view and the menu below it. The effector inspector needs the Editor's entity system, so its form is a stand-in: the
    // name edit and the icon beside it are built as its form builds them, below a widget that answers to the inspector's class name,
    // and the name edit's states are the helper the inspector's event filter calls (SetNameEditState).
    // ----------------------------------------------------------------------------------

    namespace ParticleEditorFlatTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;
        namespace Flat = OpenParticleSystemEditor::ParticleEditorFlat;
        using Compare = Twins::Compare;
        using OpenParticleSystemEditor::ParticleGraphicsView;
        using OpenParticleSystemEditor::ParticleItemWidget;
        using OpenParticleSystemEditor::ParticleLineWidget;

        //! The Qt application of the process, made once and never destroyed (a QApplication made and destroyed for each test would
        //! rebuild the platform integration each time); the offscreen platform is the caller's (QT_QPA_PLATFORM)
        struct ApplicationOnce
        {
            ApplicationOnce()
            {
                if (!QApplication::instance())
                {
                    static int argc = 1;
                    static char name[] = "OpenParticleSystemEditor.Tests";
                    static char* argv[] = { name, nullptr };
                    new QApplication(argc, argv);
                }
            }
        };

        //! The test application carries StyleManager's legacy sheets, which the twins name their own of: none while a test runs
        class ApplicationSheetGuard
        {
        public:
            ApplicationSheetGuard()
                : m_sheet(qApp->styleSheet())
            {
                qApp->setStyleSheet(QString());
            }

            ~ApplicationSheetGuard()
            {
                qApp->setStyleSheet(m_sheet);
            }

            ApplicationSheetGuard(const ApplicationSheetGuard&) = delete;
            ApplicationSheetGuard& operator=(const ApplicationSheetGuard&) = delete;

        private:
            QString m_sheet;
        };

        //! What the emitter window is given: no emitter, so that the window's calls into its source data change nothing
        struct EmitterData
        {
            OpenParticle::ParticleSourceData m_source;
            OpenParticle::ParticleSourceData::DetailInfo m_detail;
        };

        class ParticleEditorFlatTests
            : public LeakDetectionFixture
        {
        protected:
            ParticleEditorFlatTests()
            {
                // The source data reads the serialize context of the application when it is made
                m_componentApplication = AZStd::make_unique<::testing::NiceMock<MockComponentApplication>>();
                m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
                ON_CALL(*m_componentApplication, GetSerializeContext()).WillByDefault(::testing::Return(m_serializeContext.get()));
                m_data = AZStd::make_unique<EmitterData>();
            }

            //! The flattened way: the flat Style on the widget and everything below it, and each polished (the element of a widget is
            //! resolved at its polish)
            void polishFlat(QWidget& root) const
            {
                QList<QWidget*> widgets = root.findChildren<QWidget*>();
                widgets.prepend(&root);
                for (QWidget* each : widgets)
                {
                    each->setStyle(m_context.flatStyle());
                }
                for (QWidget* each : widgets)
                {
                    each->ensurePolished();
                }
            }

            ApplicationOnce m_qtApplication;
            ApplicationSheetGuard m_applicationSheet;
            Twins::Context m_context;
            AZStd::unique_ptr<::testing::NiceMock<MockComponentApplication>> m_componentApplication;
            AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
            AZStd::unique_ptr<EmitterData> m_data;
        };

        // ------------------------------------------------------------------------------
        // What the sites are called
        // ------------------------------------------------------------------------------

        constexpr const char* InspectorClass = "OpenParticleSystemEditor::EffectorInspector";   // the class of the form of the name edit
        constexpr const char* NameEditName = "particleName";                                    // EffectorInspector.ui:51
        constexpr const char* NameIconName = "particNameIco";                                   // EffectorInspector.ui:32

        //! The twelve rows of the emitter window (ParticleItemWidget.ui): each has a tool button and a label, and all but the first a
        //! check box
        constexpr const char* RowNames[] = { "emitterWidget", "spawnWidget", "particlesWidget", "locationWidget", "velocityWidget",
            "colorWidget", "sizeWidget", "forceWidget", "lightWidget", "subUVWidget", "eventWidget", "rendererWidget" };

        //! The four lines that frame a selected window, with the size the form gives each (ParticleItemWidget.ui:771-840)
        struct LineSite
        {
            const char* m_name;
            QSize m_size;
        };

        constexpr LineSite LineSites[] = {
            { "lineLeft", QSize(2, 474) },
            { "lineRight", QSize(2, 474) },
            { "lineBotton", QSize(207, 2) },
            { "lineTop", QSize(207, 2) },
        };

        constexpr QSize ItemWindowSize(213, 486);               // ParticleItemWidget.ui:13-14
        constexpr QSize RowSize(207, 30);                       // a row: the form's width, 30 high (ParticleItemWidget.ui)

        constexpr const char* BoxNameOfLine = "OpenParticle.Box.Line";
        constexpr const char* BoxNameOfLinePressed = "OpenParticle.Box.LinePressed";
        constexpr const char* BoxNameOfTitle = "OpenParticle.Box.Title";

        // ------------------------------------------------------------------------------
        // The sites, as the code builds them
        // ------------------------------------------------------------------------------

        //! The emitter window below `parent`, as ParticleGraphicsView::NewItem builds it (with a parent, as the twins need one),
        //! selected so that its four lines show
        ParticleItemWidget* makeItemWindow(EmitterData& data, QWidget* parent)
        {
            auto* item = new ParticleItemWidget(nullptr, &data.m_detail, &data.m_source, AZStd::string("particles"), parent);
            item->resize(ItemWindowSize);
            item->OnSelected();
            return item;
        }

        //! A part of the emitter window, by the object name its form gives it; none (null): the window itself, as a window of its own
        //! (in the graph view it is a translucent top-level window)
        Twins::WidgetFactory itemPart(EmitterData& data, const char* name)
        {
            return [&data, name](QWidget* parent) -> QWidget*
            {
                ParticleItemWidget* item = makeItemWindow(data, parent);
                if (!name)
                {
                    item->setWindowFlags(item->windowFlags() | Qt::Window);
                    return item;
                }
                return item->findChild<QWidget*>(QString::fromLatin1(name));
            };
        }

        //! The check box of a row (by its name where the row has two)
        QCheckBox* checkOf(QWidget& row, const char* name = nullptr)
        {
            return name ? row.findChild<QCheckBox*>(QString::fromLatin1(name)) : row.findChild<QCheckBox*>();
        }

        //! The widget the inspector's form is below: a stand-in that answers to the inspector's class name
        Twins::Link inspectorLink()
        {
            return Twins::Link{ .m_name = "effectorInspector", .m_className = InspectorClass };
        }

        //! The name edit, as EffectorInspector builds it: its form's widget and the look it gives it first
        QLineEdit* makeNameEdit(QWidget* parent)
        {
            auto* edit = new QLineEdit(parent);
            edit->setObjectName(QString::fromLatin1(NameEditName));
            edit->setText(QStringLiteral("Particle_name"));
            Flat::SetNameEditState(edit, Flat::NameEditState::Initial);
            return edit;
        }

        //! The icon beside the name edit, as the form and EffectorInspector build it
        QWidget* makeNameIcon(QWidget* parent)
        {
            auto* icon = new QWidget(parent);
            icon->setObjectName(QString::fromLatin1(NameIconName));
            icon->setFixedSize(23, 22);
            AzQtComponents::StyleManager::setLegacyStyleSheet(icon, Flat::NameIconSheet());
            return icon;
        }

        // ------------------------------------------------------------------------------
        // The states a site goes through
        // ------------------------------------------------------------------------------

        //! A row pressed and let go (ParticleLineWidget::mousePressEvent and mouseReleaseEvent call the helper)
        Twins::State pressedRow()
        {
            return { "row pressed", [](QWidget& row) { Flat::SetLineRowPressed(&row, true); },
                [](QWidget& row) { Flat::SetLineRowPressed(&row, false); } };
        }

        //! A row of a window pressed, read from the window
        Twins::State pressedRowInWindow(const char* rowName)
        {
            const auto rowOf = [rowName](QWidget& window) { return window.findChild<ParticleLineWidget*>(QString::fromLatin1(rowName)); };
            return { std::string("row pressed: ") + rowName, [rowOf](QWidget& window) { Flat::SetLineRowPressed(rowOf(window), true); },
                [rowOf](QWidget& window) { Flat::SetLineRowPressed(rowOf(window), false); } };
        }

        //! The row of a tool button pressed, read from the tool button
        Twins::State pressedRowOfButton()
        {
            return { "row pressed", [](QWidget& button) { Flat::SetLineRowPressed(button.parentWidget(), true); },
                [](QWidget& button) { Flat::SetLineRowPressed(button.parentWidget(), false); } };
        }

        //! The check box of a row disabled, as ParticleItemWidget::OnClickParticleName does while the emitter's own box is off
        Twins::State checkDisabled(const char* name = nullptr)
        {
            return { "check box disabled",
                [name](QWidget& row)
                {
                    if (QCheckBox* check = checkOf(row, name))
                    {
                        check->setEnabled(false);
                    }
                },
                [name](QWidget& row)
                {
                    if (QCheckBox* check = checkOf(row, name))
                    {
                        check->setEnabled(true);
                    }
                } };
        }

        //! The check box of a row focused (the focus rule of the box, CheckBox.qss:50-73, is outranked by the box's own sheet)
        Twins::State checkFocused(const char* name = nullptr)
        {
            return { "check box focused",
                [name](QWidget& row)
                {
                    if (QCheckBox* check = checkOf(row, name))
                    {
                        Twins::States::setActiveWindowNow(row.window());
                        check->setFocus(Qt::OtherFocusReason);
                    }
                },
                [name](QWidget& row)
                {
                    if (QCheckBox* check = checkOf(row, name))
                    {
                        check->clearFocus();
                    }
                    Twins::States::setActiveWindowNow(nullptr);
                } };
        }

        //! The check box of a row toggled
        Twins::State checkToggled(const char* name = nullptr)
        {
            const auto toggle = [name](QWidget& row)
            {
                if (QCheckBox* check = checkOf(row, name))
                {
                    check->setChecked(!check->isChecked());
                }
            };
            return { "check box toggled", toggle, toggle };
        }

        //! The name edit goes through the states the inspector's event filter puts it in, in this order, and back to its first look
        Twins::State nameEditState(const char* name, std::vector<Flat::NameEditState> sequence, bool disabled = false)
        {
            const auto edit = [](QWidget& widget) { return qobject_cast<QLineEdit*>(&widget); };
            return { name,
                [sequence, disabled, edit](QWidget& widget)
                {
                    for (const Flat::NameEditState state : sequence)
                    {
                        Flat::SetNameEditState(edit(widget), state);
                    }
                    widget.setEnabled(!disabled);
                },
                [edit](QWidget& widget)
                {
                    widget.setEnabled(true);
                    Flat::SetNameEditState(edit(widget), Flat::NameEditState::Initial);
                } };
        }

        // ------------------------------------------------------------------------------
        // The rows of the twins
        // ------------------------------------------------------------------------------

        //! C172: the graph view and its fill (ParticleGraphicsView.cpp:52)
        Twins::Row graphViewRow()
        {
            return Twins::Row{
                .m_site = "C172 ParticleGraphicsView.cpp:52 the graph view",
                .m_make = [](QWidget* parent) -> QWidget* { return new ParticleGraphicsView(parent); },
                .m_realApplicationSheet = true,
                .m_register = &Flat::RegisterFlatElements,
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = QSize(240, 160),
                // The view is one colour end to end, which the harness cannot tell from nothing drawn: the test reads the colour too
                .m_mayDrawNothing = true,
                .m_mayDecideNothing = true,
            };
        }

        //! X-ops-1: the menu the graph view pops up (ParticleGraphicsView::ShowMenu), under the view's sheet
        Twins::Row graphMenuRow()
        {
            return Twins::Row{
                .m_site = "X-ops-1 ParticleGraphicsView.cpp ShowMenu the graph view's menu",
                .m_make = [](QWidget* parent) -> QWidget*
                {
                    auto* view = new ParticleGraphicsView(parent);
                    auto* menu = new QMenu(view);
                    menu->addAction(QStringLiteral("Add Emitter"));
                    menu->addAction(QStringLiteral("Copy Emitter"))->setEnabled(false);
                    menu->addAction(QStringLiteral("Paste Emitter"));
                    return menu;
                },
                .m_realApplicationSheet = true,
                .m_register = &Flat::RegisterFlatElements,
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = QSize(120, 80),
            };
        }

        //! C173: the emitter window, whole: its fill, its rows, its title and its lines, as they lie in the window
        Twins::Row itemWindowRow(EmitterData& data)
        {
            return Twins::Row{
                .m_site = "C173 ParticleItemWidget.cpp:54 the emitter window",
                .m_make = itemPart(data, nullptr),
                .m_realApplicationSheet = true,
                .m_register = &Flat::RegisterFlatElements,
                .m_states = { pressedRowInWindow("emitterWidget"), pressedRowInWindow("rendererWidget") },
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = ItemWindowSize,
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        //! C174-C176, U066-U088: a row of the emitter window with what sits in it, idle and pressed, and its check box in each state
        Twins::Row lineRow(EmitterData& data, const char* name)
        {
            return Twins::Row{
                .m_site = "C174-C176 U066-U088 ParticleLineWidget.cpp:18 a row of the emitter window",
                .m_make = itemPart(data, name),
                .m_realApplicationSheet = true,
                .m_register = &Flat::RegisterFlatElements,
                .m_states = { pressedRow(), checkDisabled(), checkFocused(), checkToggled() },
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = RowSize,
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        //! U063, U064, U065: the title row with its tool buttons and check boxes (a tile of the row's gradient on each)
        Twins::Row titleRow(EmitterData& data)
        {
            return Twins::Row{
                .m_site = "U063-U065 ParticleItemWidget.ui:59 the title row",
                .m_make = itemPart(data, "titleWidget"),
                .m_realApplicationSheet = true,
                .m_register = &Flat::RegisterFlatElements,
                .m_states = { checkToggled("checkSolo"), checkToggled("checkParticleName"), checkDisabled("checkSolo"),
                    checkDisabled("checkParticleName"), checkFocused("checkSolo"), checkFocused("checkParticleName") },
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = RowSize,
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        //! The tool button of a row: the row's sheet reached it (a fill with a radius, no look of its own in any state)
        Twins::Row toolButtonRow(EmitterData& data, const char* site, const char* name, std::vector<Twins::State> states)
        {
            return Twins::Row{
                .m_site = site,
                .m_make = itemPart(data, name),
                .m_realApplicationSheet = true,
                .m_register = &Flat::RegisterFlatElements,
                .m_states = states,
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Pixels,
                .m_base = Twins::Base::AzQtComponentsStyle,
                .m_mayDecideNothing = true,
            };
        }

        //! The check box of a row on its own: its box and size (its picture is the row's, above)
        Twins::Row checkBoxRow(EmitterData& data, const char* site, const char* name)
        {
            return Twins::Row{
                .m_site = site,
                .m_make = itemPart(data, name),
                .m_realApplicationSheet = true,
                .m_register = &Flat::RegisterFlatElements,
                .m_states = { Twins::States::disabled(), Twins::States::focus(), Twins::States::checked() },
                .m_compare = Compare::Geometry,
                .m_base = Twins::Base::AzQtComponentsStyle,
                .m_mayDecideNothing = true,
            };
        }

        //! U089-U092: one of the four lines of a selected window
        Twins::Row selectionLineRow(EmitterData& data, const LineSite& line)
        {
            return Twins::Row{
                .m_site = "U089-U092 ParticleItemWidget.ui:781 a selection line",
                .m_make = itemPart(data, line.m_name),
                .m_realApplicationSheet = true,
                .m_register = &Flat::RegisterFlatElements,
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = line.m_size,
                .m_base = Twins::Base::AzQtComponentsStyle,
                // A line is one colour end to end, which the harness cannot tell from nothing drawn: the test reads the colour too
                .m_mayDrawNothing = true,
                .m_mayDecideNothing = true,
            };
        }

        //! C169-C171: the name edit in each look
        Twins::Row nameEditRow()
        {
            return Twins::Row{
                .m_site = "C169-C171 EffectorInspector.cpp:31 the name edit",
                .m_make = &makeNameEdit,
                .m_chain = { inspectorLink() },
                .m_realApplicationSheet = true,
                .m_register = &Flat::RegisterFlatElements,
                .m_states = {
                    nameEditState("focused", { Flat::NameEditState::Focused }),
                    nameEditState("blurred", { Flat::NameEditState::Blurred }),
                    nameEditState("focused, blurred, focused", { Flat::NameEditState::Focused, Flat::NameEditState::Blurred,
                        Flat::NameEditState::Focused }),
                    nameEditState("blurred, focused, blurred", { Flat::NameEditState::Blurred, Flat::NameEditState::Focused,
                        Flat::NameEditState::Blurred }),
                    Twins::States::disabled(),
                    nameEditState("blurred and disabled", { Flat::NameEditState::Blurred }, true),
                },
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Palette | Compare::Pixels,
                // Not Base: the legacy sheet sets it to the fill, which the field never paints with (the panel is drawn from the sheet's
                // fill, and the flat element draws its own), so no pixel depends on it
                .m_roles = { QPalette::Text, QPalette::PlaceholderText },
                .m_size = QSize(160, 20),
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        //! U062: the icon beside the name edit
        Twins::Row nameIconRow()
        {
            return Twins::Row{
                .m_site = "U062 EffectorInspector.ui:46 the icon beside the name edit",
                .m_make = &makeNameIcon,
                .m_chain = { inspectorLink() },
                .m_realApplicationSheet = true,
                .m_register = &Flat::RegisterFlatElements,
                .m_compare = Compare::Geometry | Compare::Pixels,
                .m_size = QSize(23, 22),
                .m_mayDecideNothing = true,
            };
        }

        //! The colour of the whole flat twin of a row that is one colour end to end (the graph view, a line), at ratio 1; invalid if any
        //! pixel differs. The twin check passes such a row when the two twins agree, and a picture of one colour does not say that
        //! anything was drawn (m_mayDrawNothing), so the tests say which colour it is
        QColor flatFillOf(const Twins::Row& row, Twins::Context& context)
        {
            Twins::Context::registerOnce(row.m_register);
            Twins::RungScope flat(Twins::Rung::Flat);
            const Twins::Twin twin(row, context, false);
            Twins::Reading reading;
            Twins::readPixels(reading, *twin.widget(), row);
            const QImage& image = reading.m_pictures.front().m_image;
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    if (image.pixel(x, y) != image.pixel(0, 0))
                    {
                        return QColor();
                    }
                }
            }
            return image.pixelColor(0, 0);
        }

        // ------------------------------------------------------------------------------
        // The element of a widget, by name
        // ------------------------------------------------------------------------------

        const char* boxOf(const QWidget* widget)
        {
            const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(widget);
            return element ? element->m_name : "";
        }

        const char* indicatorOf(const QWidget* widget)
        {
            const AzQtComponents::IndicatorElement* element = AzQtComponents::IndicatorElement::forWidget(widget);
            return element ? element->m_name : "";
        }

        const char* frameOf(const QWidget* widget)
        {
            const AzQtComponents::FrameElement* element = AzQtComponents::FrameElement::forWidget(widget);
            return element ? element->m_name : "";
        }

        QColor textOf(const QWidget& widget)
        {
            return widget.palette().color(QPalette::Active, QPalette::Text);
        }

        //! The census of the sheets the sites set: the emitter window and the graph view as the editor builds them, a row pressed, and the
        //! name edit and its icon as the inspector's form builds them
        QStringList censusOfSites(EmitterData& data, AzQtComponents::GuiPerfFloor::Rung rung)
        {
            return AzQtComponents::SiteTwins::sheetLinesAt(rung,
                [&data]
                {
                    QWidget host;
                    host.resize(600, 600);
                    ParticleItemWidget* item = makeItemWindow(data, &host);
                    new ParticleGraphicsView(&host);
                    QWidget* icon = makeNameIcon(&host);
                    QLineEdit* edit = makeNameEdit(&host);
                    host.setAttribute(Qt::WA_DontShowOnScreen);
                    host.show();
                    for (QWidget* each : host.findChildren<QWidget*>())
                    {
                        each->ensurePolished();
                    }
                    Flat::SetLineRowPressed(item->m_ui->emitterWidget, true);
                    Flat::SetNameEditState(edit, Flat::NameEditState::Focused);
                    Flat::SetNameEditState(edit, Flat::NameEditState::Blurred);
                    Q_UNUSED(icon);
                });
        }

        bool anyLineHas(const QStringList& lines, const QString& text)
        {
            for (const QString& line : lines)
            {
                if (line.contains(text, Qt::CaseInsensitive))
                {
                    return true;
                }
            }
            return false;
        }
    } // namespace ParticleEditorFlatTestsInternal

    // A unity build joins this file with others of namespace UnitTest: the fixture is the only name brought in, the rest is named
    // through the alias
    using ParticleEditorFlatTestsInternal::ParticleEditorFlatTests;
    namespace ParticleFlat = ParticleEditorFlatTestsInternal;

    // ----------------------------------------------------------------------------------
    // Registry: which widgets have which element
    // ----------------------------------------------------------------------------------

    TEST_F(ParticleEditorFlatTests, Registry_TheEmitterWindowAndTheWidgetsOfItsFormHaveTheirElements)
    {
        ParticleFlat::Twins::RungScope flat(ParticleFlat::Twins::Rung::Flat);
        ParticleFlat::Twins::Context::registerOnce(&ParticleFlat::Flat::RegisterFlatElements);

        QWidget root;
        ParticleFlat::ParticleItemWidget* item = ParticleFlat::makeItemWindow(*m_data, &root);
        polishFlat(root);
        auto& form = *item->m_ui;

        // The window paints nothing itself (a translucent window's background is never painted); its form's container is the fill
        EXPECT_STREQ(ParticleFlat::boxOf(item), "");
        EXPECT_STREQ(ParticleFlat::boxOf(form.verticalLayoutWidget), "OpenParticle.Box.ItemContainer");

        // The title row and its parts, a row and its parts
        EXPECT_STREQ(ParticleFlat::boxOf(form.titleWidget), ParticleFlat::BoxNameOfTitle);
        EXPECT_STREQ(ParticleFlat::boxOf(form.btnParticleName), ParticleFlat::BoxNameOfTitle);
        EXPECT_STREQ(ParticleFlat::boxOf(form.toolButton), ParticleFlat::BoxNameOfTitle);
        EXPECT_STREQ(ParticleFlat::boxOf(form.checkSolo), ParticleFlat::BoxNameOfTitle);
        EXPECT_STREQ(ParticleFlat::boxOf(form.checkParticleName), ParticleFlat::BoxNameOfTitle);
        EXPECT_STREQ(ParticleFlat::boxOf(form.emitterWidget), ParticleFlat::BoxNameOfLine);
        EXPECT_STREQ(ParticleFlat::boxOf(form.toolButton_21), ParticleFlat::BoxNameOfLine);
        EXPECT_STREQ(ParticleFlat::boxOf(form.spawnWidget), ParticleFlat::BoxNameOfLine);
        EXPECT_STREQ(ParticleFlat::boxOf(form.toolButton_20), ParticleFlat::BoxNameOfLine);

        // The label and the check box of a row have the row's colour already: no element of their own
        EXPECT_STREQ(ParticleFlat::boxOf(form.label_20), "");
        EXPECT_STREQ(ParticleFlat::boxOf(form.checkSpawn), "");

        // The check boxes: the solo box has its images, every other box of a row the fixed ones
        EXPECT_STREQ(ParticleFlat::indicatorOf(form.checkSolo), "OpenParticle.Indicator.SoloCheck");
        EXPECT_STREQ(ParticleFlat::indicatorOf(form.checkParticleName), "OpenParticle.Indicator.FixedCheck");
        EXPECT_STREQ(ParticleFlat::indicatorOf(form.checkSpawn), "OpenParticle.Indicator.FixedCheck");
        EXPECT_STREQ(ParticleFlat::indicatorOf(form.checkRenderer), "OpenParticle.Indicator.FixedCheck");

        // The four lines
        for (const ParticleFlat::LineSite& line : ParticleFlat::LineSites)
        {
            EXPECT_STREQ(ParticleFlat::boxOf(item->findChild<QWidget*>(QString::fromLatin1(line.m_name))),
                "OpenParticle.Box.SelectionLine") << line.m_name;
        }
    }

    TEST_F(ParticleEditorFlatTests, Registry_NoWidgetOutsideTheEditorHasTheElementsOfTheEditor)
    {
        // The same names and classes in another place: the sheets were set on these widgets and no other
        ParticleFlat::Twins::RungScope flat(ParticleFlat::Twins::Rung::Flat);
        ParticleFlat::Twins::Context::registerOnce(&ParticleFlat::Flat::RegisterFlatElements);

        QWidget root;
        auto* tool = new QToolButton(&root);
        auto* check = new QCheckBox(&root);
        auto* line = new QFrame(&root);
        line->setObjectName(QStringLiteral("lineLeft"));
        auto* edit = new QLineEdit(&root);
        edit->setObjectName(QString::fromLatin1(ParticleFlat::NameEditName));
        auto* icon = new QWidget(&root);
        icon->setObjectName(QString::fromLatin1(ParticleFlat::NameIconName));
        auto* plainRow = new QWidget(&root);
        auto* toolInPlainRow = new QToolButton(plainRow);
        auto* container = new QWidget(&root);
        container->setObjectName(QStringLiteral("verticalLayoutWidget"));
        QMenu menu(&root);
        polishFlat(root);

        EXPECT_STREQ(ParticleFlat::boxOf(container), "");
        EXPECT_STREQ(ParticleFlat::boxOf(line), "");
        EXPECT_STREQ(ParticleFlat::boxOf(tool), "");
        EXPECT_STREQ(ParticleFlat::boxOf(toolInPlainRow), "");
        EXPECT_STREQ(ParticleFlat::boxOf(check), "");
        EXPECT_STREQ(ParticleFlat::indicatorOf(check), "Global.Indicator.CheckBox");
        EXPECT_STREQ(ParticleFlat::frameOf(line), "Global.Frame.Panel");
        EXPECT_STREQ(ParticleFlat::boxOf(edit), "");
        EXPECT_STREQ(ParticleFlat::boxOf(icon), "");
        EXPECT_EQ(&AzQtComponents::MenuElement::forWidget(&menu), &AzQtComponents::MenuElement::forWidget(nullptr));
    }

    TEST_F(ParticleEditorFlatTests, Registry_APressedRowAndItsToolButtonsTakeThePressedFillAndOnlyThatRow)
    {
        ParticleFlat::Twins::RungScope flat(ParticleFlat::Twins::Rung::Flat);
        ParticleFlat::Twins::Context::registerOnce(&ParticleFlat::Flat::RegisterFlatElements);

        QWidget root;
        ParticleFlat::ParticleItemWidget* item = ParticleFlat::makeItemWindow(*m_data, &root);
        polishFlat(root);
        auto& form = *item->m_ui;

        ParticleFlat::Flat::SetLineRowPressed(form.emitterWidget, true);
        EXPECT_STREQ(ParticleFlat::boxOf(form.emitterWidget), ParticleFlat::BoxNameOfLinePressed);
        EXPECT_STREQ(ParticleFlat::boxOf(form.toolButton_21), ParticleFlat::BoxNameOfLinePressed);
        EXPECT_STREQ(ParticleFlat::boxOf(form.spawnWidget), ParticleFlat::BoxNameOfLine);
        EXPECT_STREQ(ParticleFlat::boxOf(form.toolButton_20), ParticleFlat::BoxNameOfLine);
        EXPECT_STREQ(ParticleFlat::boxOf(form.titleWidget), ParticleFlat::BoxNameOfTitle);

        ParticleFlat::Flat::SetLineRowPressed(form.emitterWidget, false);
        EXPECT_STREQ(ParticleFlat::boxOf(form.emitterWidget), ParticleFlat::BoxNameOfLine);
        EXPECT_STREQ(ParticleFlat::boxOf(form.toolButton_21), ParticleFlat::BoxNameOfLine);
    }

    TEST_F(ParticleEditorFlatTests, Registry_TheNameEditHasAnElementForEachOfItsLooksAndTheTextFollowsIt)
    {
        ParticleFlat::Twins::RungScope flat(ParticleFlat::Twins::Rung::Flat);
        ParticleFlat::Twins::Context::registerOnce(&ParticleFlat::Flat::RegisterFlatElements);
        using State = ParticleFlat::Flat::NameEditState;

        QWidget root;
        auto* inspector = new ParticleFlat::Twins::StandIn(ParticleFlat::InspectorClass, &root);
        QLineEdit* edit = ParticleFlat::makeNameEdit(inspector);
        auto* stray = new QLineEdit(&root);
        stray->setObjectName(QString::fromLatin1(ParticleFlat::NameEditName));
        polishFlat(root);

        // Before it had the focus: no fill over the field, white text
        EXPECT_STREQ(ParticleFlat::boxOf(edit), "OpenParticle.Box.NameEditInitial");
        EXPECT_EQ(ParticleFlat::textOf(*edit), QColor(255, 255, 255));

        // With the focus: the field of every line edit, dark text
        ParticleFlat::Flat::SetNameEditState(edit, State::Focused);
        EXPECT_STREQ(ParticleFlat::boxOf(edit), "");
        EXPECT_EQ(ParticleFlat::textOf(*edit), QColor(68, 68, 68));

        // After it lost the focus: a flat fill, white text
        ParticleFlat::Flat::SetNameEditState(edit, State::Blurred);
        EXPECT_STREQ(ParticleFlat::boxOf(edit), "OpenParticle.Box.NameEditBlurred");
        EXPECT_EQ(ParticleFlat::textOf(*edit), QColor(255, 255, 255));

        // And again: the look comes back each time
        ParticleFlat::Flat::SetNameEditState(edit, State::Focused);
        EXPECT_EQ(ParticleFlat::textOf(*edit), QColor(68, 68, 68));
        ParticleFlat::Flat::SetNameEditState(edit, State::Blurred);
        EXPECT_STREQ(ParticleFlat::boxOf(edit), "OpenParticle.Box.NameEditBlurred");
        EXPECT_EQ(ParticleFlat::textOf(*edit), QColor(255, 255, 255));

        // A line edit of the same name outside the inspector's form keeps the field of every line edit
        ParticleFlat::Flat::SetNameEditState(stray, State::Blurred);
        EXPECT_STREQ(ParticleFlat::boxOf(stray), "");
        EXPECT_NE(ParticleFlat::textOf(*stray), QColor(255, 255, 255));
    }

    TEST_F(ParticleEditorFlatTests, Registry_TheGraphViewItsViewportAndTheMenuBelowItHaveTheFill)
    {
        ParticleFlat::Twins::RungScope flat(ParticleFlat::Twins::Rung::Flat);
        ParticleFlat::Twins::Context::registerOnce(&ParticleFlat::Flat::RegisterFlatElements);

        QWidget root;
        auto* view = new ParticleFlat::ParticleGraphicsView(&root);
        QMenu menu(view);
        QMenu elsewhere(&root);
        polishFlat(root);

        EXPECT_STREQ(ParticleFlat::boxOf(view), "OpenParticle.Box.GraphView");
        EXPECT_EQ(AzQtComponents::BoxElement::containerOf(view->viewport()), view);

        // The sheet had no selector and reached the menu below the view: its surface is the view's fill, the rest the popup's
        const AzQtComponents::MenuElement& global = AzQtComponents::MenuElement::forWidget(nullptr);
        const AzQtComponents::MenuElement& graph = AzQtComponents::MenuElement::forWidget(&menu);
        EXPECT_STREQ(graph.m_name, "OpenParticle.Menu.GraphView");
        EXPECT_TRUE(graph.m_surface == QColor(0x2D, 0x2D, 0x2D));
        EXPECT_FALSE(global.m_surface == graph.m_surface);
        EXPECT_EQ(graph.m_paddingH, global.m_paddingH);
        EXPECT_EQ(graph.m_paddingV, global.m_paddingV);
        EXPECT_TRUE(graph.m_text == global.m_text);
        EXPECT_TRUE(graph.m_selectedFill == global.m_selectedFill);
        EXPECT_TRUE(graph.m_disabledText == global.m_disabledText);
        EXPECT_EQ(&AzQtComponents::MenuElement::forWidget(&elsewhere), &global);
    }

    TEST_F(ParticleEditorFlatTests, Registry_TheClassOfARowIsItsOwnSoItsRowsDoNotSitOnQWidget)
    {
        // ParticleLineWidget has Q_OBJECT, so a meta object of its own, and the attribute that gave its sheets a styled background
        // before (QStyleSheetStyle::polish sets it for a QWidget itself only)
        QWidget root;
        auto* row = new ParticleFlat::ParticleLineWidget(&root);
        EXPECT_TRUE(row->metaObject() != &QWidget::staticMetaObject);
        EXPECT_STREQ(row->metaObject()->className(), "OpenParticleSystemEditor::ParticleLineWidget");
        EXPECT_TRUE(row->testAttribute(Qt::WA_StyledBackground));
    }

    // ----------------------------------------------------------------------------------
    // Rungs: the legacy GUI sets the sheets, the flattened GUI none
    // ----------------------------------------------------------------------------------

    TEST_F(ParticleEditorFlatTests, Sheets_TheLegacyGuiSetsThemWithTheColoursOfTheElementsAndTheFlattenedGuiNone)
    {
        using AzQtComponents::GuiPerfFloor;
        const QStringList legacy = ParticleFlat::censusOfSites(*m_data, GuiPerfFloor::Rung::Qss);
        EXPECT_FALSE(legacy.isEmpty());
        for (const char* colour : { "#474747", "#666666", "#2d2d2d", "#333333", "#444444", "#00000000", "#ffff00" })
        {
            EXPECT_TRUE(ParticleFlat::anyLineHas(legacy, QString::fromLatin1(colour))) << colour << "\n" << legacy.join("\n").toStdString();
        }
        EXPECT_TRUE(ParticleFlat::anyLineHas(legacy, QStringLiteral("rgba(223, 169, 112, 255)")));
        EXPECT_TRUE(ParticleFlat::anyLineHas(legacy, QStringLiteral("checkbox/off.svg")));
        EXPECT_TRUE(ParticleFlat::anyLineHas(legacy, QStringLiteral("Asset_File.svg")));

        // The flattened GUI sets none: the census reads no widget of the editor under a sheet
        const QStringList flat = ParticleFlat::censusOfSites(*m_data, GuiPerfFloor::Rung::Flat);
        EXPECT_TRUE(flat.isEmpty()) << flat.join("\n").toStdString();
    }

    TEST_F(ParticleEditorFlatTests, Sheets_TheLegacyRowSheetsAreTheTextTheRowsHadAndThePressedOneIsToldApart)
    {
        using AzQtComponents::GuiPerfFloor;
        GuiPerfFloor::setRungForTesting(GuiPerfFloor::Rung::Qss);

        QWidget root;
        ParticleFlat::ParticleItemWidget* item = ParticleFlat::makeItemWindow(*m_data, &root);
        ParticleFlat::ParticleLineWidget* row = item->m_ui->emitterWidget;
        EXPECT_EQ(row->styleSheet(), QStringLiteral("background-color: #474747; border-radius: 1px;"));
        ParticleFlat::Flat::SetLineRowPressed(row, true);
        EXPECT_EQ(row->styleSheet(), QStringLiteral("background-color: #666666; border-radius: 1px;"));
        ParticleFlat::Flat::SetLineRowPressed(row, false);
        EXPECT_EQ(row->styleSheet(), QStringLiteral("background-color: #474747; border-radius: 1px;"));

        // The title row has the gradient the form gave it, in place of the row's fill
        EXPECT_TRUE(item->m_ui->titleWidget->styleSheet().contains(QStringLiteral("qlineargradient")));
        GuiPerfFloor::resetForTesting();
    }

    // ----------------------------------------------------------------------------------
    // Twins: the elements against the sheets, at the device ratios of the GUI
    // ----------------------------------------------------------------------------------

    TEST_F(ParticleEditorFlatTests, Twin_TheGraphViewIsFilledAsItsSheetFilledIt)
    {
        EXPECT_TRUE(ParticleFlat::Twins::check(ParticleFlat::graphViewRow(), m_context));
        EXPECT_EQ(ParticleFlat::flatFillOf(ParticleFlat::graphViewRow(), m_context), QColor(0x2D, 0x2D, 0x2D));
    }

    TEST_F(ParticleEditorFlatTests, Twin_TheGraphMenuIsFilledAsTheViewsSheetFilledIt)
    {
        EXPECT_TRUE(ParticleFlat::Twins::check(ParticleFlat::graphMenuRow(), m_context));
    }

    TEST_F(ParticleEditorFlatTests, Twin_TheEmitterWindowIsPaintedAsItsSheetsPaintedIt)
    {
        // The whole window as it lies in the graph view: the fill (a square, if the twin says the radius is not drawn), the rows, the
        // title, the lines, and two rows pressed
        EXPECT_TRUE(ParticleFlat::Twins::check(ParticleFlat::itemWindowRow(*m_data), m_context));
    }

    TEST_F(ParticleEditorFlatTests, Twin_EachRowOfTheEmitterWindowIsPaintedAsItsSheetPaintedItInEveryState)
    {
        // The twelve rows with their tool button, label and check box: the fill of the sheet reached them all (the label and the check
        // box lie on the row's colour), the press, and the check box in the states of its own
        for (const char* name : ParticleFlat::RowNames)
        {
            EXPECT_TRUE(ParticleFlat::Twins::check(ParticleFlat::lineRow(*m_data, name), m_context)) << name;
        }
    }

    TEST_F(ParticleEditorFlatTests, Twin_TheTitleRowAndTheTilesOfItsGradientAreThoseOfItsSheet)
    {
        EXPECT_TRUE(ParticleFlat::Twins::check(ParticleFlat::titleRow(*m_data), m_context));
    }

    TEST_F(ParticleEditorFlatTests, Twin_TheToolButtonsOfARowAreTheTilesOfTheRowsSheet)
    {
        using ParticleFlat::Twins::check;
        EXPECT_TRUE(check(ParticleFlat::toolButtonRow(*m_data, "C174 a tool button of a row", "toolButton_21", {}), m_context));
        EXPECT_TRUE(check(ParticleFlat::toolButtonRow(*m_data, "C175 a tool button of a row pressed", "toolButton_20",
            { ParticleFlat::pressedRowOfButton() }), m_context));
        EXPECT_TRUE(check(ParticleFlat::toolButtonRow(*m_data, "U063 the name button of the title row", "btnParticleName", {}), m_context));
        EXPECT_TRUE(check(ParticleFlat::toolButtonRow(*m_data, "U063 the first tool button of the title row", "toolButton", {}),
            m_context));
    }

    TEST_F(ParticleEditorFlatTests, Twin_TheCheckBoxesOfTheFormHaveTheBoxOfTheirSheets)
    {
        using ParticleFlat::Twins::check;
        EXPECT_TRUE(check(ParticleFlat::checkBoxRow(*m_data, "U064 the solo check box", "checkSolo"), m_context));
        EXPECT_TRUE(check(ParticleFlat::checkBoxRow(*m_data, "U065 the name check box", "checkParticleName"), m_context));
        EXPECT_TRUE(check(ParticleFlat::checkBoxRow(*m_data, "U068 a check box of a row", "checkSpawn"), m_context));
        EXPECT_TRUE(check(ParticleFlat::checkBoxRow(*m_data, "U088 a check box of a row", "checkRenderer"), m_context));
    }

    TEST_F(ParticleEditorFlatTests, Twin_TheFourSelectionLinesAreYellowAsTheirSheetsMadeThem)
    {
        for (const ParticleFlat::LineSite& line : ParticleFlat::LineSites)
        {
            const ParticleFlat::Twins::Row row = ParticleFlat::selectionLineRow(*m_data, line);
            EXPECT_TRUE(ParticleFlat::Twins::check(row, m_context)) << line.m_name;
            EXPECT_EQ(ParticleFlat::flatFillOf(row, m_context), QColor(255, 255, 0)) << line.m_name;
        }
    }

    TEST_F(ParticleEditorFlatTests, Twin_TheNameEditHasTheLooksOfItsSheetsInEveryState)
    {
        // Before the focus, with it, after it, again, and disabled (the widget's sheet outranks the application's disabled colours)
        EXPECT_TRUE(ParticleFlat::Twins::check(ParticleFlat::nameEditRow(), m_context));
    }

    TEST_F(ParticleEditorFlatTests, Twin_TheIconBesideTheNameEditIsTheGradientOfItsSheet)
    {
        EXPECT_TRUE(ParticleFlat::Twins::check(ParticleFlat::nameIconRow(), m_context));
    }
} // namespace UnitTest
