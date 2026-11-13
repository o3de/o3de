/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Tests/UI/UIFixture.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/GuiPerfFloor.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Tests/SheetTwins.h>
#include <AzQtComponents/Tests/SiteTwins.h>

#include <EMotionStudio/EMStudioSDK/Source/EMStudioFlatSheet.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioFlatValues.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/NotificationWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/ResetSettingsDialog.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphEditor.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphHierarchyWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigateWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/Inspector/ContentHeaderWidget.h>

#include <Editor/LineEditValidatable.h>
#include <Editor/NotificationWidget.h>
#include <Editor/Plugins/SimulatedObject/SimulatedJointWidget.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>

#include <MysticQt/Source/DialogStack.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QProgressBar>
#include <QSplitter>
#include <QStringList>
#include <QTextEdit>
#include <QToolButton>
#include <QTreeView>
#include <QWidget>

#include <memory>
#include <utility>
#include <vector>

namespace EMotionFX
{
    // ----------------------------------------------------------------------------------
    // EMStudioFlat (EMStudioFlatSheet.cpp): the inline sheets of the Animation Editor's widgets, twinned. The legacy twin is Qt's own
    // style sheet style over AzQtComponents::Style with the real application sheet, and the sheet the site sets (the real site's code
    // at the legacy rung, or the sheet quoted from the site where the widget is built here as the site builds it); the flat twin is the
    // flattened Style with the elements of EMStudioFlat, and no sheet. A state is driven through the site's own calls: its legacy sheet
    // (StyleManager::setLegacyStyleSheet) and its class tag (EMStudioFlat::SetStateTag).
    //
    // The sites that live in a plugin of the Animation Editor that the fixture makes only once (the command bar, the motion extraction
    // window, the blend space motions, the parameter editor) are built here as their code builds them: the same widget class, name
    // and sheet. The widgets whose constructors are self-contained are the real classes.
    // ----------------------------------------------------------------------------------

    namespace EMStudioFlatSheetTestsInternal
    {
        namespace Twins = AzQtComponents::SheetTwins;

        constexpr unsigned AllFacts = Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Palette | Twins::Compare::Pixels;
        constexpr unsigned BoxFacts = Twins::Compare::Geometry | Twins::Compare::Palette | Twins::Compare::Pixels;
        constexpr unsigned LookFacts = Twins::Compare::Palette | Twins::Compare::Pixels;

        // The roles a widget paints with; the others (a label's Base, Button and Window, a line edit's Button and Window) are never read
        // by what draws it, which is the element's fill and the text colour, so a row that compares them compares what nothing paints
        const std::vector<QPalette::ColorRole> LabelRoles{ QPalette::WindowText, QPalette::Text, QPalette::ButtonText };
        const std::vector<QPalette::ColorRole> FieldRoles{ QPalette::Text, QPalette::PlaceholderText, QPalette::Base };
        // A field the element fills itself: the fill is the element's, and no role of the palette is painted with
        const std::vector<QPalette::ColorRole> ElementFilledFieldRoles{ QPalette::Text, QPalette::PlaceholderText };
        const std::vector<QPalette::ColorRole> ComboRoles{ QPalette::Base, QPalette::Button, QPalette::Text, QPalette::ButtonText };
        const std::vector<QPalette::ColorRole> TextEditRoles{ QPalette::Base, QPalette::Text };
        const std::vector<QPalette::ColorRole> TreeRoles{ QPalette::Base, QPalette::Text, QPalette::WindowText };
        const std::vector<QPalette::ColorRole> FrameRoles{ QPalette::WindowText, QPalette::Text };

        //! A window shown without a screen, so that it is polished and laid out as the application has it
        QWidget* shown(QWidget* window)
        {
            window->setAttribute(Qt::WA_DontShowOnScreen);
            window->show();
            return window;
        }

        //! A site whose sheet or state the site's own code sets in the legacy twin: the row quotes none, so that the sheets decide
        //! nothing of the row's own (the twin without them has them too)
        Twins::Row siteRow(
            const char* site, Twins::WidgetFactory make, unsigned compare, std::vector<Twins::State> states = {}, QSize size = QSize(),
            std::vector<QPalette::ColorRole> roles = {})
        {
            return Twins::Row{
                .m_site = site,
                .m_make = std::move(make),
                .m_realApplicationSheet = true,
                .m_register = &EMStudioFlat::RegisterElements,
                .m_states = std::move(states),
                .m_compare = compare,
                .m_roles = std::move(roles),
                .m_size = size,
                .m_base = Twins::Base::AzQtComponentsStyle,
                .m_mayDecideNothing = true,
            };
        }

        //! A site built here as its code builds it, with the sheet it sets, quoted from the source
        Twins::Row quotedRow(
            const char* site, Twins::WidgetFactory make, const char* cite, const QString& sheet, unsigned compare,
            std::vector<Twins::State> states = {}, QSize size = QSize(), std::vector<QPalette::ColorRole> roles = {})
        {
            // The sheet text must outlive the row: the quoted sheets are plain pointers
            static std::vector<std::unique_ptr<QByteArray>> texts;
            texts.push_back(std::make_unique<QByteArray>(sheet.toUtf8()));
            return Twins::Row{
                .m_site = site,
                .m_make = std::move(make),
                .m_sheets = { { cite, Twins::SetOn::Widget, texts.back()->constData() } },
                .m_realApplicationSheet = true,
                .m_register = &EMStudioFlat::RegisterElements,
                .m_states = std::move(states),
                .m_compare = compare,
                .m_roles = std::move(roles),
                .m_size = size,
                .m_base = Twins::Base::AzQtComponentsStyle,
            };
        }

        Twins::State state(
            const char* name, std::function<void(QWidget&)> enter, std::function<void(QWidget&)> leave)
        {
            return Twins::State{ name, std::move(enter), std::move(leave) };
        }

        // ------------------------------------------------------------------------------
        // The state calls of the sites: a legacy sheet and a class tag
        // ------------------------------------------------------------------------------

        void setSheetAndTag(QWidget& widget, const QString& sheet, const char* tag, bool on)
        {
            AzQtComponents::StyleManager::setLegacyStyleSheet(&widget, on ? sheet : QString());
            EMStudioFlat::SetStateTag(&widget, tag, on);
        }

        //! The motion set combo box's warning (AnimGraphEditor.cpp, SetMotionSetWarning), with the popup list polished again
        void setMotionSetWarning(QWidget& widget, bool on)
        {
            setSheetAndTag(
                widget, QString("border: 1px solid %1;").arg(EMStudioFlatValues::WarningBorder.name()), EMStudioFlat::MotionSetWarningTag,
                on);
            if (AzQtComponents::StyleManager::flatStylingActive())
            {
                for (QAbstractItemView* popup : widget.findChildren<QAbstractItemView*>())
                {
                    popup->style()->unpolish(popup);
                    popup->style()->polish(popup);
                }
            }
        }

        QString conflictSheet()
        {
            return QString("color: %1;").arg(EMStudioFlatValues::ConflictText.name());
        }

        // ------------------------------------------------------------------------------
        // The input fields: C103, C148, C140
        // ------------------------------------------------------------------------------

        QWidget* makeRenameField(QWidget* parent)
        {
            auto* dialog = new EMStudio::ParameterCreateRenameWindow(
                "Rename", nullptr, "name", "name", AZStd::vector<AZStd::string>(), parent);
            shown(dialog);
            return dialog->findChild<QLineEdit*>();
        }

        std::vector<Twins::State> invalidTextStates()
        {
            return { state(
                "invalid text", [](QWidget& widget) { static_cast<QLineEdit&>(widget).setText(QString()); },
                [](QWidget& widget) { static_cast<QLineEdit&>(widget).setText(QStringLiteral("name")); }) };
        }

        QWidget* makeValidatableField(QWidget* parent)
        {
            auto* edit = new EMStudio::LineEditValidatable(parent);
            edit->setProperty("flatTwinValid", true);
            edit->SetValidatorFunc([edit]() { return edit->property("flatTwinValid").toBool(); });
            return edit;
        }

        std::vector<Twins::State> invalidValidatableStates()
        {
            return { state(
                "invalid name",
                [](QWidget& widget)
                {
                    widget.setProperty("flatTwinValid", false);
                    static_cast<QLineEdit&>(widget).setText(QStringLiteral("bad"));
                },
                [](QWidget& widget)
                {
                    widget.setProperty("flatTwinValid", true);
                    static_cast<QLineEdit&>(widget).setText(QStringLiteral("good"));
                }) };
        }

        //! The name field of a disabled time track, as HeaderTrackWidget builds it (TrackHeaderWidget.cpp)
        QWidget* makeTrackNameField(QWidget* parent)
        {
            auto* edit = new QLineEdit(QStringLiteral("Track"), parent);
            edit->setObjectName(EMStudioFlat::InputFieldName);
            edit->setFrame(false);
            AzQtComponents::StyleManager::setLegacyStyleSheet(
                edit, QStringLiteral("background-color: %1;").arg(EMStudioFlatValues::DisabledNameEdit.name()));
            EMStudioFlat::SetStateTag(edit, EMStudioFlat::DisabledTrackTag, true);
            return edit;
        }

        std::vector<Twins::State> invalidTrackNameStates()
        {
            return { state(
                "invalid name", [](QWidget& widget) { EMStudio::GetManager()->SetWidgetAsInvalidInput(&widget); },
                [](QWidget& widget) { EMStudio::GetManager()->SetWidgetAsValidInput(&widget); }) };
        }

        // ------------------------------------------------------------------------------
        // The labels: the shortcut dialog's two, the bold ones, the bar's progress text, the blend space's axes and motion name
        // ------------------------------------------------------------------------------

        //! A label of ShortcutReceiverDialog (KeyboardShortcutsWindow.cpp): named, centered, with a font of its own
        QWidget* makeShortcutLabel(QWidget* parent)
        {
            auto* label = new QLabel(QStringLiteral("Ctrl+S"), parent);
            label->setObjectName(EMStudioFlat::ShortcutLabelName);
            label->setAlignment(Qt::AlignHCenter);
            QFont font = label->font();
            font.setPointSize(14);
            font.setBold(true);
            label->setFont(font);
            return label;
        }

        std::vector<Twins::State> conflictStates()
        {
            return { state(
                "conflict", [](QWidget& widget) { setSheetAndTag(widget, conflictSheet(), EMStudioFlat::ConflictTag, true); },
                [](QWidget& widget) { setSheetAndTag(widget, conflictSheet(), EMStudioFlat::ConflictTag, false); }) };
        }

        QWidget* makeContentHeaderTitle(QWidget* parent)
        {
            auto* header = new EMStudio::ContentHeaderWidget(parent);
            auto* title = header->findChild<QLabel*>(EMStudioFlat::BoldLabelName);
            if (title)
            {
                title->setText(QStringLiteral("Anim graph"));
            }
            return title;
        }

        QWidget* makeResetTopLabel(QWidget* parent)
        {
            auto* dialog = new EMStudio::ResetSettingsDialog(parent);
            shown(dialog);
            return dialog->findChild<QLabel*>(EMStudioFlat::DialogTopLabelName);
        }

        QWidget* makeProgressText(QWidget* parent)
        {
            auto* label = new QLabel(QStringLiteral("Saving the anim graph"), parent);
            label->setObjectName(EMStudioFlat::ProgressTextName);
            label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            label->setAlignment(Qt::AlignRight);
            return label;
        }

        QWidget* makeAxisLabel(QWidget* parent, const char* name, const QString& text)
        {
            auto* label = new QLabel(text, parent);
            label->setObjectName(QString::fromLatin1(name));
            label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            return label;
        }

        QWidget* makeMotionLabel(QWidget* parent)
        {
            auto* label = new QLabel(QStringLiteral("Walk"), parent);
            label->setObjectName(QStringLiteral("m_labelMotion"));
            label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            return label;
        }

        constexpr const char* InvalidMotionSheet = "#m_labelMotion { border: 1px solid red; }";
        constexpr const char* ValidMotionSheet = "#m_labelMotion { border: none; }";

        std::vector<Twins::State> invalidMotionStates()
        {
            const auto invalid = [](QWidget& widget)
            {
                AzQtComponents::StyleManager::setLegacyStyleSheet(&widget, InvalidMotionSheet);
                EMStudioFlat::SetStateTag(&widget, EMStudioFlat::InvalidMotionTag, true);
            };
            const auto valid = [](QWidget& widget)
            {
                AzQtComponents::StyleManager::setLegacyStyleSheet(&widget, ValidMotionSheet);
                EMStudioFlat::SetStateTag(&widget, EMStudioFlat::InvalidMotionTag, false);
            };
            // C154, and C155 which takes it away again (a label with no frame draws and sizes as it did before)
            return { state("invalid motion", invalid, valid),
                state("invalid motion cleared", [invalid, valid](QWidget& widget) { invalid(widget); valid(widget); }, [](QWidget&) {}) };
        }

        // ------------------------------------------------------------------------------
        // The command bar's result field: its text, its tip, its menu
        // ------------------------------------------------------------------------------

        QWidget* makeResultField(QWidget* parent)
        {
            auto* edit = new QLineEdit(QStringLiteral("The command failed"), parent);
            edit->setObjectName(EMStudioFlat::ResultEditName);
            edit->setReadOnly(true);
            edit->setToolTip(QStringLiteral("The command failed"));
            return edit;
        }

        void setResult(QWidget& widget, bool error, bool on)
        {
            const QColor color = error ? EMStudioFlatValues::ResultErrorText : EMStudioFlatValues::ResultSuccessText;
            const char* tag = error ? EMStudioFlat::ResultErrorTag : EMStudioFlat::ResultSuccessTag;
            const char* other = error ? EMStudioFlat::ResultSuccessTag : EMStudioFlat::ResultErrorTag;
            AzQtComponents::StyleManager::setLegacyStyleSheet(&widget, on ? QString("color: %1;").arg(color.name()) : QString());
            if (on)
            {
                EMStudioFlat::SetStateTag(&widget, other, false);
            }
            EMStudioFlat::SetStateTag(&widget, tag, on);
        }

        std::vector<Twins::State> resultStates()
        {
            return { state(
                         "error", [](QWidget& widget) { setResult(widget, true, true); },
                         [](QWidget& widget) { setResult(widget, true, false); }),
                state(
                    "success", [](QWidget& widget) { setResult(widget, false, true); },
                    [](QWidget& widget) { setResult(widget, false, false); }) };
        }

        // ------------------------------------------------------------------------------
        // The anim graph editor's motion set combo box and its popup, the trees
        // ------------------------------------------------------------------------------

        EMotionFX::AnimGraphEditor* makeAnimGraphEditor(QWidget* parent)
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            return new EMotionFX::AnimGraphEditor(nullptr, serializeContext, parent);
        }

        QWidget* makeMotionSetCombo(QWidget* parent)
        {
            return makeAnimGraphEditor(parent)->GetMotionSetComboBox();
        }

        //! The popup list of the combo box: made, so that it is polished with the rest of the twin
        QWidget* makeMotionSetPopup(QWidget* parent)
        {
            QComboBox* combo = makeAnimGraphEditor(parent)->GetMotionSetComboBox();
            combo->addItem(QStringLiteral("Motion set"));
            QAbstractItemView* view = combo->view();
            // The popup container is shown, so that the list is laid out and draws its items as the popup of the real combo box does
            shown(view->window());
            return view;
        }

        std::vector<Twins::State> motionSetWarningStates()
        {
            return { state(
                         "warning", [](QWidget& widget) { setMotionSetWarning(widget, true); },
                         [](QWidget& widget) { setMotionSetWarning(widget, false); }),
                state(
                    "warning, hovered",
                    [](QWidget& widget)
                    {
                        setMotionSetWarning(widget, true);
                        Twins::States::hover().m_enter(widget);
                    },
                    [](QWidget& widget)
                    {
                        Twins::States::hover().m_leave(widget);
                        setMotionSetWarning(widget, false);
                    }) };
        }

        //! The popup's own state: the warning is the combo box's, which the list reads through its popup container
        std::vector<Twins::State> motionSetPopupStates()
        {
            const auto comboOf = [](QWidget& popup) -> QWidget*
            {
                QWidget* container = popup.parentWidget();
                return container ? container->parentWidget() : nullptr;
            };
            return { state(
                "warning", [comboOf](QWidget& popup) { setMotionSetWarning(*comboOf(popup), true); },
                [comboOf](QWidget& popup) { setMotionSetWarning(*comboOf(popup), false); }) };
        }

        // ------------------------------------------------------------------------------
        // The rest: boxes of the dialogs, the stack, the caret, the preview frame, the notification widget's header
        // ------------------------------------------------------------------------------

        QWidget* makeErrorWindow(QWidget* parent)
        {
            auto* window = new EMStudio::ErrorWindow(parent);
            window->Init(AZStd::vector<AZStd::string>{ "The first command failed", "The second command failed" });
            return shown(window);
        }

        QWidget* makeErrorWindowText(QWidget* parent)
        {
            return makeErrorWindow(parent)->findChild<QTextEdit*>();
        }

        QWidget* makeNotificationIcon(QWidget* parent)
        {
            auto* window = new EMStudio::NotificationWindow(parent, EMStudio::NotificationWindow::TYPE_SUCCESS, QStringLiteral("Saved"));
            shown(window);
            return window->findChild<QToolButton*>(QStringLiteral("NotificationIcon"));
        }

        QWidget* makeRecoverCheck(QWidget* parent)
        {
            auto* check = new QCheckBox(QString(), parent);
            check->setObjectName(EMStudioFlat::RecoverCheckName);
            check->setChecked(true);
            return check;
        }

        QWidget* makeProgressBar(QWidget* parent)
        {
            auto* bar = new QProgressBar(parent);
            bar->setObjectName(EMStudioFlat::ProgressBarName);
            bar->setRange(0, 100);
            bar->setValue(50);
            bar->setMaximumWidth(300);
            // The maximum height that Style::polish gives a flat bar (the height of its hint) and no legacy bar has: the legacy twin
            // gets the same number from the row, as the twins of the Global bar do (ControlFoundationTwinTests, progressRow)
            bar->setMaximumHeight(5);
            return bar;
        }

        QWidget* makeCaretCheck(QWidget* parent)
        {
            auto* check = new QCheckBox(QStringLiteral("Motion Extraction"), parent);
            check->setObjectName(EMStudioFlat::CaretCheckName);
            check->setChecked(true);
            return check;
        }

        QWidget* makePreviewFrame(QWidget* parent)
        {
            auto* frame = new QFrame(parent);
            frame->setFrameShape(QFrame::Box);
            frame->setObjectName(QStringLiteral("previewFrame"));
            return frame;
        }

        QWidget* makeNotificationHeader(QWidget* parent)
        {
            auto* widget = new EMotionFX::NotificationWidget(parent, QStringLiteral("This collider collides with nothing"));
            return widget->findChild<QFrame*>(QStringLiteral("HeaderFrame"));
        }

        //! A stack of three dialogs, and the headers that open and close them
        //! The handle between the second and the third card of a stack of three: the handle of the splitter of the second card, which
        //! opens and closes with it. The stack stands behind it in the twin (its parent chain), so the states find it again.
        QWidget* makeDialogStackHandle(QWidget* parent)
        {
            auto* stack = new MysticQt::DialogStack(parent);
            stack->Add(new QLabel(QStringLiteral("First")), QStringLiteral("First"));
            stack->Add(new QLabel(QStringLiteral("Second")), QStringLiteral("Second"));
            stack->Add(new QLabel(QStringLiteral("Third")), QStringLiteral("Third"));
            const QList<QSplitter*> splitters = stack->findChildren<QSplitter*>();
            return splitters.size() > 1 ? splitters[1]->handle(1) : nullptr;
        }

        std::vector<Twins::State> dialogStackStates()
        {
            const auto setMiddle = [](QWidget& handle, bool expanded)
            {
                for (QWidget* each = &handle; each; each = each->parentWidget())
                {
                    if (each->inherits("MysticQt::DialogStack"))
                    {
                        const QList<AzQtComponents::CardHeader*> headers = each->findChildren<AzQtComponents::CardHeader*>();
                        if (headers.size() > 1)
                        {
                            headers[1]->setExpanded(expanded);
                        }
                        return;
                    }
                }
            };
            return { state(
                "middle closed", [setMiddle](QWidget& handle) { setMiddle(handle, false); },
                [setMiddle](QWidget& handle) { setMiddle(handle, true); }) };
        }

        QString previewSheet()
        {
            return QString("QFrame#previewFrame { border: 2px dashed %1; background-color: %2; }")
                .arg(EMStudioFlatValues::PreviewBorder.name())
                .arg(EMStudioFlatValues::PreviewBackground.name(QColor::HexArgb));
        }

        constexpr const char* CaretCheckSheet =
            "QCheckBox::indicator { width: 16px; height: 16px; border: none; margin: 0px; } "
            "QCheckBox::indicator:checked, QCheckBox::indicator:checked:disabled, QCheckBox::indicator:checked:focus "
            "{ image: url(:/Cards/img/UI20/Cards/caret-down.svg); } "
            "QCheckBox::indicator:unchecked, QCheckBox::indicator:unchecked:disabled, QCheckBox::indicator:unchecked:focus "
            "{ image: url(:/Cards/img/UI20/Cards/caret-right.svg); }";

        //! A QMenu the widget parents, as QLineEdit::contextMenuEvent and QTextEdit::contextMenuEvent make theirs: polished in the
        //! flattened GUI, so that its element is the one it keeps
        const AzQtComponents::MenuElement& menuElementOf(QWidget& owner, QStyle* flatStyle)
        {
            auto* menu = new QMenu(&owner);
            menu->setStyle(flatStyle);
            menu->ensurePolished();
            return AzQtComponents::MenuElement::forWidget(menu);
        }

        class EMStudioFlatSheetTests
            : public UIFixture
        {
        protected:
            void SetUp() override
            {
                UIFixture::SetUp();
                // The twins live in an application of their own (the legacy twin puts the real application sheet on its root), so the
                // sheet the fixture's StyleManager gave the application is taken away; the context makes its own application only when
                // the process has none
                qApp->setStyleSheet(QString());
                m_context = std::make_unique<EMStudioFlatSheetTestsInternal::Twins::Context>();
            }

            void TearDown() override
            {
                m_context.reset();
                UIFixture::TearDown();
            }

            std::unique_ptr<EMStudioFlatSheetTestsInternal::Twins::Context> m_context;
        };

        // ----------------------------------------------------------------------------------
        // The input fields
        // ----------------------------------------------------------------------------------

        TEST_F(EMStudioFlatSheetTests, InvalidInput_RenameDialogField_HasTheRedLineOfItsSheet)
        {
            // C103 and the clears C126, C127, C135, C136, C137, C141: SetWidgetAsInvalidInput and SetWidgetAsValidInput
            EXPECT_TRUE(Twins::check(
                siteRow(
                    "C103 the name field of the parameter rename dialog", &makeRenameField, BoxFacts, invalidTextStates(), QSize(),
                    FieldRoles),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, InvalidInput_ValidatableField_HasTheRedLineOfItsSheet)
        {
            // C148: the row is on the class, and covers what derives from it
            EXPECT_TRUE(Twins::check(
                siteRow(
                    "C148 the line edit of an input dialog that validates its text", &makeValidatableField, BoxFacts,
                    invalidValidatableStates(), QSize(), FieldRoles),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, DisabledTrackName_HasTheFillOfItsSheetAndTheRedLineOfAnInvalidName)
        {
            // C140: idle (the fill), then an invalid name (the red line replaces the sheet)
            EXPECT_TRUE(Twins::check(
                siteRow(
                    "C140 the name field of a disabled time track", &makeTrackNameField, BoxFacts, invalidTrackNameStates(), QSize(),
                    ElementFilledFieldRoles),
                *m_context));
        }

        // ----------------------------------------------------------------------------------
        // The labels
        // ----------------------------------------------------------------------------------

        TEST_F(EMStudioFlatSheetTests, ShortcutLabels_TakeTheAmberOfTheConflictAndGiveItBack)
        {
            // C107 and C108: the two labels, in the conflict state
            EXPECT_TRUE(Twins::check(
                siteRow("C107 the key label of the shortcut dialog", &makeShortcutLabel, AllFacts, conflictStates(), QSize(), LabelRoles),
                *m_context));
            EXPECT_TRUE(Twins::check(
                siteRow(
                    "C108 the conflict label of the shortcut dialog", &makeShortcutLabel, AllFacts, conflictStates(), QSize(), LabelRoles),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, BoldLabels_AreBold)
        {
            // C133: the title of the inspector's content header (the real class). C115 is the file name of the anim graph editor below
            EXPECT_TRUE(Twins::check(
                siteRow(
                    "C133 the title of the content header of the inspector", &makeContentHeaderTitle, AllFacts, {}, QSize(), LabelRoles),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, BoldLabels_AnimGraphEditorFileNameIsBold)
        {
            // C115 and C116: the editor also routes the sheet of its property editor
            EXPECT_TRUE(Twins::check(
                siteRow(
                    "C115 the file name label of the anim graph editor",
                    [](QWidget* parent) -> QWidget*
                    {
                        auto* label = makeAnimGraphEditor(parent)->findChild<QLabel*>(EMStudioFlat::BoldLabelName);
                        // The editor of no anim graph has no file name, and a label with no text draws nothing
                        label->setText(QStringLiteral("Walk.animgraph"));
                        return label;
                    },
                    AllFacts, {}, QSize(), LabelRoles),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, BoldLabels_JointWidgetNameIsBold)
        {
            // C152: the real widget on the simulated object plugin of the fixture (C150 and C151 are the same element on two labels of the
            // collider widget, whose content builder needs a skeleton)
            EMotionFX::SimulatedObjectWidget* plugin = m_simulatedObjectPlugin;
            EXPECT_TRUE(Twins::check(
                siteRow(
                    "C152 the name label of the simulated joint widget",
                    [plugin](QWidget* parent) -> QWidget*
                    {
                        auto* widget = new EMotionFX::SimulatedJointWidget(plugin, parent);
                        auto* label = widget->findChild<QLabel*>(EMStudioFlat::BoldLabelName);
                        // The widget names no joint until one is selected, and a label with no text draws nothing; its contents are not
                        // shown before that, so the label is lifted out to be drawn (the element finds it by its name alone)
                        label->setText(QStringLiteral("Joint name"));
                        label->setParent(parent);
                        label->show();
                        return label;
                    },
                    AllFacts, {}, QSize(), LabelRoles),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, ResetSettingsTitle_HasItsStripAndPadding)
        {
            // C113
            EXPECT_TRUE(Twins::check(
                siteRow("C113 the title strip of the reset workspace dialog", &makeResetTopLabel, AllFacts, {}, QSize(), LabelRoles),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, CommandBarProgressText_HasItsColourAndPadding)
        {
            // C129: two elements on one label
            EXPECT_TRUE(Twins::check(
                quotedRow(
                    "C129 the progress text of the command bar", &makeProgressText, "CommandBarPlugin.cpp:104",
                    QString("padding-right: 1px; color: %1;").arg(EMStudioFlatValues::ProgressText.name()), AllFacts, {}, QSize(200, 18),
                    LabelRoles),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, BlendSpaceAxisLabels_AreBoldRedAndGreen)
        {
            // C153: the two looks of the run-time colour string
            EXPECT_TRUE(Twins::check(
                quotedRow(
                    "C153 the X label of a blend space motion",
                    [](QWidget* parent) { return makeAxisLabel(parent, EMStudioFlat::AxisXName, "X"); },
                    "BlendSpaceMotionContainerHandler.cpp:51", QStringLiteral("QLabel { font-weight: bold; color : red; }"), AllFacts, {},
                    QSize(), LabelRoles),
                *m_context));
            EXPECT_TRUE(Twins::check(
                quotedRow(
                    "C153 the Y label of a blend space motion",
                    [](QWidget* parent) { return makeAxisLabel(parent, EMStudioFlat::AxisYName, "Y"); },
                    "BlendSpaceMotionContainerHandler.cpp:51", QStringLiteral("QLabel { font-weight: bold; color : green; }"), AllFacts, {},
                    QSize(), LabelRoles),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, BlendSpaceMotionLabel_HasTheRedLineWhileInvalid)
        {
            // C154 and C155
            EXPECT_TRUE(Twins::check(
                siteRow(
                    "C154 the motion label of a blend space motion", &makeMotionLabel, BoxFacts, invalidMotionStates(), QSize(),
                    LabelRoles),
                *m_context));
        }

        // ----------------------------------------------------------------------------------
        // The command bar's result field and progress bar
        // ----------------------------------------------------------------------------------

        TEST_F(EMStudioFlatSheetTests, CommandResult_HasTheRedAndTheGreenOfItsSheet)
        {
            // C131 and C132
            EXPECT_TRUE(Twins::check(
                siteRow("C131 the result field of the command bar", &makeResultField, AllFacts, resultStates(), QSize(), FieldRoles),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, CommandResult_TipTakesTheColourOfTheField)
        {
            // The tooltip of the field (CommandBarPlugin.cpp:179 sets it on an error) took the sheet's colour
            EXPECT_TRUE(Twins::check(
                siteRow("C131 the tip of the result field of the command bar", &makeResultField, Twins::Compare::ToolTip, resultStates()),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, CommandBarProgressBar_StopsShortAtTheRight)
        {
            // C130
            EXPECT_TRUE(Twins::check(
                quotedRow(
                    "C130 the progress bar of the command bar", &makeProgressBar, "CommandBarPlugin.cpp:112",
                    QStringLiteral("padding-right: 2px;"), Twins::Compare::Geometry | Twins::Compare::Fonts | Twins::Compare::Pixels,
                    {}, QSize(200, 5)),
                *m_context));
        }

        // ----------------------------------------------------------------------------------
        // The menus and the popups that a bare sheet reached through their owner
        // ----------------------------------------------------------------------------------

        TEST_F(EMStudioFlatSheetTests, ErrorWindowMenu_TakesTheDialogsFill)
        {
            // EM1: the text edit's standard context menu is a QMenu the text edit parents
            AzQtComponents::SheetTwins::RungScope flat(AzQtComponents::GuiPerfFloor::Rung::Flat);
            Twins::Context::registerOnce(&EMStudioFlat::RegisterElements);
            EMStudio::ErrorWindow window(nullptr);
            auto* edit = window.findChild<QTextEdit*>();
            ASSERT_NE(edit, nullptr);
            const AzQtComponents::MenuElement& element = menuElementOf(*edit, m_context->flatStyle());
            EXPECT_STREQ(element.m_name, "EMotionFX.Menu.ErrorWindow");
            EXPECT_EQ(element.m_surface, EMStudioFlatValues::ErrorWindowBackground);

            QMenu other;
            other.setStyle(m_context->flatStyle());
            other.ensurePolished();
            EXPECT_STRNE(AzQtComponents::MenuElement::forWidget(&other).m_name, "EMotionFX.Menu.ErrorWindow")
                << "a menu of no dialog takes it";
        }

        TEST_F(EMStudioFlatSheetTests, CommandResultMenu_TakesTheColourOfTheFieldWhileTheStateHolds)
        {
            // EM3: the result field's standard context menu has the text colour of the field's sheet
            AzQtComponents::SheetTwins::RungScope flat(AzQtComponents::GuiPerfFloor::Rung::Flat);
            Twins::Context::registerOnce(&EMStudioFlat::RegisterElements);
            QLineEdit edit;
            edit.setObjectName(EMStudioFlat::ResultEditName);
            edit.setStyle(m_context->flatStyle());

            setResult(edit, true, true);
            EXPECT_EQ(menuElementOf(edit, m_context->flatStyle()).m_text, EMStudioFlatValues::ResultErrorText);
            setResult(edit, false, true);
            EXPECT_EQ(menuElementOf(edit, m_context->flatStyle()).m_text, EMStudioFlatValues::ResultSuccessText);
            setResult(edit, false, false);
            EXPECT_NE(menuElementOf(edit, m_context->flatStyle()).m_text, EMStudioFlatValues::ResultSuccessText);
        }

        TEST_F(EMStudioFlatSheetTests, MotionSetComboBox_HasTheOrangeLineWhileItWarns)
        {
            // C118 and C119
            EXPECT_TRUE(Twins::check(
                siteRow(
                    "C118 the motion set combo box of the anim graph editor", &makeMotionSetCombo, BoxFacts, motionSetWarningStates(),
                    QSize(), ComboRoles),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, MotionSetComboBox_PopupListHasTheOrangeLineWhileItWarns)
        {
            // EM2: the list a bare sheet reached. Its pictures are the list's own, and the items are the Global popup's
            EXPECT_TRUE(Twins::check(
                // What the list draws: its size hint also counts the hints of its scroll bars, which the combo box's bare sheet gave the
                // line as well and which show only when the list scrolls (a cascade this home does not carry)
                siteRow(
                    "C118 the popup list of the motion set combo box", &makeMotionSetPopup, Twins::Compare::Pixels,
                    motionSetPopupStates(), QSize(140, 60)),
                *m_context));
        }

        // ----------------------------------------------------------------------------------
        // The dialogs, the stack, the caret, the preview frame, the notification widget
        // ----------------------------------------------------------------------------------

        TEST_F(EMStudioFlatSheetTests, ErrorWindow_IsFilledAsItsSheetFilledIt)
        {
            // C110: the dialog, and the text edit with its margin and the lanes of its scroll bars
            EXPECT_TRUE(Twins::check(
                // The dialog's fill is the element's: the legacy palette's Window is what a sheet's background-color writes, which the
                // dialog does not paint with, and its size limits are the code's (setMinimumWidth, setMinimumHeight), which no sheet or
                // element decides, so the dialog is compared by what it shows
                siteRow("C110 the error report dialog", &makeErrorWindow, Twins::Compare::Pixels, {}, QSize(600, 400)),
                *m_context));
            EXPECT_TRUE(Twins::check(
                siteRow(
                    "C110 the text edit of the error report dialog", &makeErrorWindowText, LookFacts, {}, QSize(560, 360), TextEditRoles),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, NotificationIcon_HasNoBevelInAnyState)
        {
            // C111
            EXPECT_TRUE(Twins::check(
                siteRow(
                    "C111 the icon of a notification window", &makeNotificationIcon, Twins::Compare::Geometry | Twins::Compare::Pixels,
                    { Twins::States::hover(), Twins::States::pressed() }),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, RecoverCheckBox_IsAtMostSixteenWide)
        {
            // C112
            EXPECT_TRUE(Twins::check(
                quotedRow(
                    "C112 the check box of a file of the recover files dialog", &makeRecoverCheck, "RecoverFilesWindow.cpp:118",
                    QStringLiteral("background: transparent; padding-left: 3px; max-width: 13px;"),
                    Twins::Compare::Geometry | Twins::Compare::Pixels),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, CaretCheckBox_DrawsACaretInEveryState)
        {
            // C139
            EXPECT_TRUE(Twins::check(
                quotedRow(
                    "C139 the 'Motion Extraction' check box", &makeCaretCheck, "MotionExtractionWindow.cpp:167",
                    QString::fromLatin1(CaretCheckSheet),
                    Twins::Compare::Geometry | Twins::Compare::Pixels,
                    { Twins::States::checked(), Twins::States::disabled(), Twins::States::focus() }),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, PreviewFrame_HasItsDashedLineAndFill)
        {
            // C123
            EXPECT_TRUE(Twins::check(
                quotedRow(
                    "C123 the preview frame of the parameter editor", &makePreviewFrame, "ParameterCreateEditWidget.cpp:83", previewSheet(),
                    Twins::Compare::Geometry | Twins::Compare::Pixels, {}, QSize(160, 60)),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, NotificationHeader_HasItsFill)
        {
            // C149
            EXPECT_TRUE(Twins::check(
                siteRow(
                    "C149 the header of a notification widget", &makeNotificationHeader, LookFacts, {}, QSize(200, 32), FrameRoles),
                *m_context));
        }

        TEST_F(EMStudioFlatSheetTests, DialogStack_HasTheGapOfItsSheetOpenAndClosed)
        {
            // C142 to C146: the gap between two cards is a transparent handle, 4px while the card above it is open and 1px while it is
            // closed. The twin is the handle itself: the stack fixes the height of each card when it adds it, from the hints its widgets
            // have then, and the legacy twin has the application's sheet only after the factory (a header 20px high, 27px with the
            // sheet), so the cards of a whole stack differ between the twins by what no site decides. The handle draws nothing of its
            // own in the legacy GUI, so the picture proves that the flattened Style does not draw the Global handle in its place.
            Twins::Row row = siteRow(
                "C142 the handle between two cards of a dialog stack", &makeDialogStackHandle,
                Twins::Compare::Geometry | Twins::Compare::Pixels, dialogStackStates(), QSize(240, 4));
            row.m_mayDrawNothing = true;
            EXPECT_TRUE(Twins::check(row, *m_context));
        }

        // ----------------------------------------------------------------------------------
        // The trees
        // ----------------------------------------------------------------------------------

        TEST_F(EMStudioFlatSheetTests, GraphTrees_HaveTheFontAndTheTextOfTheirSheet)
        {
            // C120 and C121: the font and the palette (the trees show the plugin's model, which has no rows here)
            EMStudio::AnimGraphPlugin* plugin = m_animGraphPlugin;
            Twins::Row hierarchy = siteRow(
                "C120 the tree of the anim graph hierarchy widget",
                [](QWidget* parent) -> QWidget*
                {
                    return (new EMStudio::AnimGraphHierarchyWidget(parent))->findChild<QTreeView*>(EMStudioFlat::GraphTreeName);
                },
                Twins::Compare::Fonts | Twins::Compare::Palette, {}, QSize(), TreeRoles);
            EXPECT_TRUE(Twins::check(hierarchy, *m_context));

            Twins::Row navigate = siteRow(
                "C121 the tree of the navigate widget",
                [plugin](QWidget* parent) -> QWidget*
                {
                    return (new EMStudio::NavigateWidget(plugin, parent))->findChild<QTreeView*>(EMStudioFlat::GraphTreeName);
                },
                Twins::Compare::Fonts | Twins::Compare::Palette, {}, QSize(), TreeRoles);
            EXPECT_TRUE(Twins::check(navigate, *m_context));
        }




        // ----------------------------------------------------------------------------------
        // The sites set a sheet in the legacy GUI only
        // ----------------------------------------------------------------------------------

        TEST_F(EMStudioFlatSheetTests, Sites_SetTheirSheetsOnlyInTheLegacyGui)
        {
            // The routed sites (C114, C116, C124, C125, C128, C134, C138) are the same call as the carried ones: a sheet in the legacy GUI,
            // none in the flattened one. The sites built here are the ones with self-contained constructors; the editors' sheets that only
            // route (the transparent property editors) are among them through the anim graph editor
            EMStudio::AnimGraphPlugin* plugin = m_animGraphPlugin;
            const auto build = [plugin]
            {
                QWidget host;
                host.resize(600, 400);
                makeAnimGraphEditor(&host);
                new EMStudio::ContentHeaderWidget(&host);
                new EMotionFX::NotificationWidget(&host, QStringLiteral("Notification"));
                new EMStudio::NavigateWidget(plugin, &host);
                new EMStudio::AnimGraphHierarchyWidget(&host);
                auto* stack = new MysticQt::DialogStack(&host);
                stack->Add(new QLabel(QStringLiteral("First")), QStringLiteral("First"));
                stack->Add(new QLabel(QStringLiteral("Second")), QStringLiteral("Second"), true);
                auto* validatable = new EMStudio::LineEditValidatable(&host);
                validatable->SetValidatorFunc([]() { return false; });
                validatable->setText(QStringLiteral("bad"));
                makeErrorWindow(&host);
                makeResetTopLabel(&host);
                makeNotificationIcon(&host);
                host.setAttribute(Qt::WA_DontShowOnScreen);
                host.show();
                for (QWidget* each : host.findChildren<QWidget*>())
                {
                    each->ensurePolished();
                }
            };
            const QStringList legacy = AzQtComponents::SiteTwins::sheetLinesAt(AzQtComponents::GuiPerfFloor::Rung::Qss, build);
            EXPECT_GE(legacy.size(), 8) << legacy.join("\n").toStdString();
            const QStringList flat = AzQtComponents::SiteTwins::sheetLinesAt(AzQtComponents::GuiPerfFloor::Rung::Flat, build);
            EXPECT_TRUE(flat.isEmpty()) << flat.join("\n").toStdString();
        }
    } // namespace EMStudioFlatSheetTestsInternal
} // namespace EMotionFX
