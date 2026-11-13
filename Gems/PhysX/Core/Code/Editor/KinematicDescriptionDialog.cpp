/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/KinematicDescriptionDialog.h>

#include <Editor/ui_KinematicDescriptionDialog.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QBrush>
#include <QColor>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QMargins>
#include <QPushButton>
#include <QString>

static constexpr const char* const SimulatedDescription = "With <b>Simulated</b> rigid bodies, you can use physics forces "
                                                  "(gravity, collision, etc.) to control the movement and "
                                                  "position of the object.";

static constexpr const char* const KinematicDescription = "With <b>Kinematic</b> rigid bodies, you can use Transform to "
                                                    "control the movement and position of the object. ";

static constexpr const char* const MoveWithCodeText = "Move with code";
static constexpr const char* const MoveManuallyText = "Move manually";
static constexpr const char* const CollisionsText = "Collisions";
static constexpr const char* const GravityText = "Gravity";

static constexpr const char* const ImpactIcon = "<img src=\":/stylesheet/img/16x16/impact.svg\"/>";
static constexpr const char* const GravityIcon = "<img src=\":/stylesheet/img/16x16/gravity.svg\"/>";
static constexpr const char* const MoveManuallyIcon = "<img src=\":/stylesheet/img/16x16/move_manually.svg\"/>";
static constexpr const char* const MoveWithCodeIcon = "<img src=\":/stylesheet/img/16x16/move_with_code.svg\"/>";

namespace PhysX
{
    namespace Editor
    {
        //=========================================================================
        // KINEMATIC DESCRIPTION DIALOG VALUES (the looks of the form and of the boxes of the two modes)
        //
        // The citations of the values of the form point at KinematicDescriptionDialog.ui as it was before the flattening, which set them
        // as inline sheets on its widgets.
        //=========================================================================
        namespace KinematicDescriptionDialogFlat
        {
            constexpr QColor BoxFill(51, 51, 51);      // KinematicDescriptionDialog.cpp:71. TOKENIZE: KinematicDescriptionBoxFill
            constexpr QColor BoxBorder(30, 112, 235);  // KinematicDescriptionDialog.cpp:72. TOKENIZE: KinematicDescriptionBoxBorder

            constexpr QColor ValidText(0x58, 0xBC, 0x61);       // KinematicDescriptionDialog.ui:295. TOKENIZE: KinematicValidText
            constexpr QColor InvalidText(0xFA, 0x27, 0x27);     // KinematicDescriptionDialog.ui:311. TOKENIZE: KinematicInvalidText
            constexpr int NoteMarginLeft = 5;       // KinematicDescriptionDialog.ui:327, :349, :371, :393. TOKENIZE: KinematicNoteIndent
            constexpr QColor DividerLine(136, 136, 136);        // KinematicDescriptionDialog.ui:434-435. TOKENIZE: KinematicDividerLine
            constexpr int DividerLineWidth = 1;                 // KinematicDescriptionDialog.ui:435. TOKENIZE: KinematicDividerLineWidth
            constexpr QColor SaveFill(30, 112, 235);            // KinematicDescriptionDialog.ui:519. TOKENIZE: KinematicSaveFill
        } // namespace KinematicDescriptionDialogFlat

        //=========================================================================
        // FLATTENED GUI: THE LOOKS OF THE DIALOG
        //
        // The form and the dialog gave their widgets inline sheets: 19 on the form, set by setupUi, and the two of OnButtonClicked. The
        // legacy GUI sets them (SetLegacySheets, SetBoxLook); the flattened GUI sets none, and these elements draw what they drew. A
        // sheet had no selector, so it reached its widget and every widget below it; the elements carry what changes what the user
        // sees of that cascade:
        //   - `border: none` of a group box (:79, :264) takes the base style's frame off the group box (a group box draws that frame
        //     under every rule that leaves the border native), and, below groupBox_2 (:264), the border off the panel and off the two
        //     push buttons (a rule with a border of its own draws the button's face itself, with no bevel)
        //   - a fill of a box or of the panel repeats on the labels below it, which sit on the same fill: nothing to carry
        //   - `border: none` on a label or a radio button (:115, :131, :178, :194, :231, :247) changes nothing the user sees: a label has
        //     no frame, and the radio buttons are drawn by the indicator element either way
        // The rules of the application sheet that reach these widgets (the universal rule BaseStyleSheet.qss:15-21, the label rule
        // Text.qss:89-95, the push button rules PushButton.qss:25-58) are the Global elements'. An element is the whole definition of
        // its box, so those that carry a box restate what the label rule gave it (the margin above and below).
        //=========================================================================

        //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins files into one translation unit,
        //! where anonymous namespaces merge.
        namespace KinematicDescriptionDialogFlatInternal
        {
            //! The class tag of the box of the mode that is chosen (Style::addClass, the flattened GUI only), which its element reads
            constexpr const char* SelectedTag = "KinematicSelected";

            // ------------------------------------------------------------------------------
            // Elements: the group boxes
            // ------------------------------------------------------------------------------

            //! `border: none` on groupBox (KinematicDescriptionDialog.ui:79) and groupBox_2 (:264), which reaches groupBox_5 (a group box
            //! with no sheet of its own, below groupBox) too: no frame at all. The fill is the universal rule's, a clear one
            //! (BaseStyleSheet.qss:17), and the title is empty.
            const AzQtComponents::BoxElement Frameless{
                .m_name = "PhysX.Box.KinematicFrameless",
                .m_nativeBorder = false,
                .m_idle = { .m_fill = QBrush(AzQtComponents::Foundations::Surface::Fill) },
            };

            //! The box of the mode that is not chosen: `background-color: rgb(51, 51, 51); border: none;` (OnButtonClicked, the sheet
            //! set at KinematicDescriptionDialog.cpp:77 and :83 before the flattening; the fill of the form's own sheet, .ui:100 and :162,
            //! is the same and is replaced by it at construction)
            const AzQtComponents::BoxElement Option{
                .m_name = "PhysX.Box.KinematicOption",
                .m_nativeBorder = false,
                .m_idle = { .m_fill = QBrush(KinematicDescriptionDialogFlat::BoxFill) },
            };

            //! The box of the mode that is chosen: `background-color: rgb(51, 51, 51); border: 1px solid rgb(30, 112, 235);`
            //! (OnButtonClicked, the sheet set at KinematicDescriptionDialog.cpp:76 and :82 before the flattening)
            const AzQtComponents::BoxElement OptionSelected{
                .m_name = "PhysX.Box.KinematicOptionSelected",
                .m_nativeBorder = false,
                .m_idle = {
                    .m_fill = QBrush(KinematicDescriptionDialogFlat::BoxFill),
                    .m_edges = AzQtComponents::BoxElement::border(1, KinematicDescriptionDialogFlat::BoxBorder),
                },
            };

            // ------------------------------------------------------------------------------
            // Elements: the labels
            // ------------------------------------------------------------------------------

            //! `color: #58BC61` on the "Valid" header (KinematicDescriptionDialog.ui:295): the label rule's font (Text.qss:91-92) in
            //! green. The text is rich text, which reads the palette's text colour
            const AzQtComponents::TextElement ValidHeader{
                .m_name = "PhysX.Text.Valid",
                .m_family = AzQtComponents::Foundations::Label::Family,
                .m_pixelSize = AzQtComponents::Foundations::Label::Size,
                .m_color = KinematicDescriptionDialogFlat::ValidText,
            };

            //! `color: #FA2727` on the "Invalid" header (KinematicDescriptionDialog.ui:311)
            const AzQtComponents::TextElement InvalidHeader{
                .m_name = "PhysX.Text.Invalid",
                .m_family = AzQtComponents::Foundations::Label::Family,
                .m_pixelSize = AzQtComponents::Foundations::Label::Size,
                .m_color = KinematicDescriptionDialogFlat::InvalidText,
            };

            //! `margin-left: 5px;` on the four notes under the headers (KinematicDescriptionDialog.ui:327, :349, :371, :393), over the
            //! margin above and below of every label (`QLabel { margin-top: 1px; margin-bottom: 1px }`, Text.qss:93-94). A label has
            //! no frame, so its border is the default one.
            const AzQtComponents::BoxElement Note{
                .m_name = "PhysX.Box.KinematicNote",
                .m_margin = QMargins(KinematicDescriptionDialogFlat::NoteMarginLeft, AzQtComponents::Foundations::Label::MarginV, 0,
                    AzQtComponents::Foundations::Label::MarginV),
                .m_idle = { .m_fill = QBrush(AzQtComponents::Foundations::Surface::Fill) },
            };

            // ------------------------------------------------------------------------------
            // Elements: the panel of icons and its divider
            // ------------------------------------------------------------------------------

            //! `background-color: rgb(51, 51, 51);` on the panel (KinematicDescriptionDialog.ui:415) over the application sheet's fill of
            //! a frame (BaseStyleSheet.qss:40-48), which a sheet of the widget or of an ancestor outranks. The panel has no border of its
            //! own declaration, so the `border: none;` of groupBox_2 (:264) reaches it: no StyledPanel frame.
            const AzQtComponents::BoxElement Panel{
                .m_name = "PhysX.Box.KinematicPanel",
                .m_nativeBorder = false,
                .m_idle = { .m_fill = QBrush(KinematicDescriptionDialogFlat::BoxFill) },
            };

            //! `color: rgb(136, 136, 136); border-right: 1px solid rgb(136, 136, 136);` on the vertical line of the panel
            //! (KinematicDescriptionDialog.ui:434-435): the border replaces the line's own frame, and is drawn at the right edge of the
            //! widget; the colour is moot. The fill is the panel's, which reaches the line as a descendant (:415), and the other edges
            //! have the `border: none;` of groupBox_2.
            const AzQtComponents::BoxElement Divider{
                .m_name = "PhysX.Box.KinematicDivider",
                .m_nativeBorder = false,
                .m_idle = {
                    .m_fill = QBrush(KinematicDescriptionDialogFlat::BoxFill),
                    .m_edges = {
                        .m_right = { KinematicDescriptionDialogFlat::DividerLineWidth, KinematicDescriptionDialogFlat::DividerLine },
                    },
                },
            };

            // ------------------------------------------------------------------------------
            // Elements: the buttons
            // ------------------------------------------------------------------------------

            //! `background-color: rgb(30, 112, 235);` on Save (KinematicDescriptionDialog.ui:519): the widget's sheet outranks the
            //! application sheet's rules of the button in every state (they set the text colour and the font of a button that is not
            //! flat, PushButton.qss:30-46, :54-64), so the face is the same blue idle, hovered and pressed. `border: none;` of
            //! groupBox_2 (:264) gives the rule a border of its own: the sheet style draws the face itself, with no bevel.
            const AzQtComponents::BoxElement Save{
                .m_name = "PhysX.Box.KinematicSave",
                .m_nativeBorder = false,
                .m_idle = { .m_fill = QBrush(KinematicDescriptionDialogFlat::SaveFill) },
            };

            //! Cancel has no sheet of its own: it takes the `border: none;` of groupBox_2 (KinematicDescriptionDialog.ui:264) and the
            //! universal rule's clear fill (BaseStyleSheet.qss:17), which the sheet style draws as the label alone, with no bevel
            const AzQtComponents::BoxElement Cancel{
                .m_name = "PhysX.Box.KinematicCancel",
                .m_nativeBorder = false,
                .m_idle = { .m_fill = QBrush(AzQtComponents::Foundations::Surface::Fill) },
            };

            // ------------------------------------------------------------------------------
            // Matchers: the widget's own test first, then the dialog it is below. The names the form gives (Designer's own) are not
            // the dialog's alone, so the dialog is asked after the name; the names of one .ui file need no second test.
            // ------------------------------------------------------------------------------

            //! The widgets below a dialog of this class
            bool isInDialog(const QWidget* widget)
            {
                return AzQtComponents::Selectors::withinClass(widget, KinematicDescriptionDialog::staticMetaObject.className());
            }

            bool isFrameless(const QWidget* widget)
            {
                using AzQtComponents::Selectors::named;
                return (named(widget, "groupBox") || named(widget, "groupBox_2") || named(widget, "groupBox_5")) && isInDialog(widget);
            }

            bool isOption(const QWidget* widget)
            {
                using AzQtComponents::Selectors::named;
                return named(widget, "simulatedBox") || named(widget, "kinematicBox");
            }

            bool isOptionSelected(const QWidget* widget)
            {
                return isOption(widget) && AzQtComponents::Selectors::tagged(widget, SelectedTag);
            }

            bool isValidHeader(const QWidget* widget)
            {
                return AzQtComponents::Selectors::named(widget, "valid");
            }

            bool isInvalidHeader(const QWidget* widget)
            {
                return AzQtComponents::Selectors::named(widget, "label_8") && isInDialog(widget);
            }

            bool isNote(const QWidget* widget)
            {
                using AzQtComponents::Selectors::named;
                return named(widget, "validLabel1") || named(widget, "validLabel2") || named(widget, "invalidLabel1")
                    || named(widget, "invalidLabel2");
            }

            bool isPanel(const QWidget* widget)
            {
                return AzQtComponents::Selectors::named(widget, "frame") && isInDialog(widget);
            }

            bool isDivider(const QWidget* widget)
            {
                return AzQtComponents::Selectors::named(widget, "line") && isInDialog(widget);
            }

            bool isSave(const QWidget* widget)
            {
                return AzQtComponents::Selectors::named(widget, "saveButton") && isInDialog(widget);
            }

            bool isCancel(const QWidget* widget)
            {
                return AzQtComponents::Selectors::named(widget, "cancelButton") && isInDialog(widget);
            }

            // ------------------------------------------------------------------------------
            // The sheets of the legacy GUI
            // ------------------------------------------------------------------------------

            //! The sheets the form gave its widgets (KinematicDescriptionDialog.ui before the flattening), as it wrote them
            namespace LegacySheets
            {
                //! groupBox :79 (written without the closing semicolon, which the parser takes the same), label_2 :115, label_3 :131,
                //! label_4 :178, label_5 :194, simulatedRadioButton :231, kinematicRadioButton :247, groupBox_2 :264
                constexpr const char* NoBorder = "border: none;";
                constexpr const char* ValidColor = "color: #58BC61";                                 // valid :295
                constexpr const char* InvalidColor = "color: #FA2727";                               // label_8 :311
                constexpr const char* NoteIndent = "margin-left: 5px;";                     // validLabel1 :327 to invalidLabel2 :393
                constexpr const char* PanelFill = "background-color: rgb(51, 51, 51);\n";            // frame :415
                constexpr const char* DividerLine =                                                  // line :434-435
                    "color: rgb(136, 136, 136);\nborder-right: 1px solid  rgb(136, 136, 136); ";
                constexpr const char* SaveFill = "background-color: rgb(30, 112, 235);";             // saveButton :519
            } // namespace LegacySheets

            //! The legacy GUI sets the sheets the form gave its widgets (setupUi does not any more); the flattened GUI sets none. The
            //! boxes of the two modes had one too (.ui:100, :162), which OnButtonClicked replaced at construction: SetBoxLook has them.
            void SetLegacySheets(Ui::KinematicDescriptionDialog& ui)
            {
                using AzQtComponents::StyleManager;
                if (StyleManager::stylesheetsDisabled())
                {
                    return;
                }

                for (QWidget* widget : std::initializer_list<QWidget*>{ ui.groupBox, ui.label_2, ui.label_3, ui.label_4, ui.label_5,
                         ui.simulatedRadioButton, ui.kinematicRadioButton, ui.groupBox_2 })
                {
                    StyleManager::setLegacyStyleSheet(widget, QString::fromLatin1(LegacySheets::NoBorder));
                }
                StyleManager::setLegacyStyleSheet(ui.valid, QString::fromLatin1(LegacySheets::ValidColor));
                StyleManager::setLegacyStyleSheet(ui.label_8, QString::fromLatin1(LegacySheets::InvalidColor));
                for (QWidget* widget :
                     std::initializer_list<QWidget*>{ ui.validLabel1, ui.validLabel2, ui.invalidLabel1, ui.invalidLabel2 })
                {
                    StyleManager::setLegacyStyleSheet(widget, QString::fromLatin1(LegacySheets::NoteIndent));
                }
                StyleManager::setLegacyStyleSheet(ui.frame, QString::fromLatin1(LegacySheets::PanelFill));
                StyleManager::setLegacyStyleSheet(ui.line, QString::fromLatin1(LegacySheets::DividerLine));
                StyleManager::setLegacyStyleSheet(ui.saveButton, QString::fromLatin1(LegacySheets::SaveFill));
            }

            //! The look of the box of a mode: the legacy GUI sets the sheet it always set, the flattened GUI gives the box of the chosen
            //! mode the class tag its element reads (Style::addClass and removeClass polish the box again)
            void SetBoxLook(QGroupBox* box, bool selected)
            {
                const QString boxStyleSheet = QString("background-color: %1;").arg(KinematicDescriptionDialogFlat::BoxFill.name());
                const QString boxBorderStyleSheet = QString(" border: 1px solid %1;").arg(KinematicDescriptionDialogFlat::BoxBorder.name());
                AzQtComponents::StyleManager::setLegacyStyleSheet(
                    box, selected ? boxStyleSheet + boxBorderStyleSheet : boxStyleSheet + " border: none;");

                if (AzQtComponents::StyleManager::flatStylingActive())
                {
                    if (selected)
                    {
                        AzQtComponents::Style::addClass(box, QString::fromLatin1(SelectedTag));
                    }
                    else
                    {
                        AzQtComponents::Style::removeClass(box, QString::fromLatin1(SelectedTag));
                    }
                }
            }
        } // namespace KinematicDescriptionDialogFlatInternal

        //=========================================================================
        // REGISTRATION
        //=========================================================================

        void KinematicDescriptionDialog::RegisterFlatElements()
        {
            static bool registered = false;
            if (registered)
            {
                return;
            }
            registered = true;

            namespace Flat = KinematicDescriptionDialogFlatInternal;
            using AzQtComponents::BoxElement;
            using AzQtComponents::TextElement;

            // The first row that matches wins: the chosen box before the other
            BoxElement::registerElement(&Flat::OptionSelected, &QGroupBox::staticMetaObject, &Flat::isOptionSelected);
            BoxElement::registerElement(&Flat::Option, &QGroupBox::staticMetaObject, &Flat::isOption);
            BoxElement::registerElement(&Flat::Frameless, &QGroupBox::staticMetaObject, &Flat::isFrameless);
            BoxElement::registerElement(&Flat::Note, &QLabel::staticMetaObject, &Flat::isNote);
            BoxElement::registerElement(&Flat::Panel, &QFrame::staticMetaObject, &Flat::isPanel);
            BoxElement::registerElement(&Flat::Divider, &QFrame::staticMetaObject, &Flat::isDivider);
            BoxElement::registerElement(&Flat::Save, &QPushButton::staticMetaObject, &Flat::isSave);
            BoxElement::registerElement(&Flat::Cancel, &QPushButton::staticMetaObject, &Flat::isCancel);
            TextElement::registerElement(&Flat::ValidHeader, &QLabel::staticMetaObject, &Flat::isValidHeader);
            TextElement::registerElement(&Flat::InvalidHeader, &QLabel::staticMetaObject, &Flat::isInvalidHeader);
        }

        //=========================================================================
        // DIALOG
        //=========================================================================

        KinematicDescriptionDialog::KinematicDescriptionDialog(bool kinematicSetting, QWidget* parent)
            : QDialog(parent)
            , m_ui(new Ui::KinematicDescriptionDialog)
            , m_kinematicSetting(kinematicSetting)
        {
            // The elements first: the widgets the form makes are polished as the dialog is built, and take the element they find then
            if (AzQtComponents::StyleManager::stylesheetsDisabled())
            {
                RegisterFlatElements();
            }

            m_ui->setupUi(this);
            KinematicDescriptionDialogFlatInternal::SetLegacySheets(*m_ui);

            InitializeButtons();
            UpdateDialogText();
        }

        KinematicDescriptionDialog::~KinematicDescriptionDialog()
        {
        }

        void KinematicDescriptionDialog::InitializeButtons()
        {
            connect(m_ui->simulatedRadioButton, &QPushButton::clicked, this, &KinematicDescriptionDialog::OnButtonClicked);
            connect(m_ui->kinematicRadioButton, &QPushButton::clicked, this, &KinematicDescriptionDialog::OnButtonClicked);
            connect(m_ui->simulatedBox, &QGroupBox::clicked, this, &KinematicDescriptionDialog::OnButtonClicked);
            connect(m_ui->kinematicBox, &QGroupBox::clicked, this, &KinematicDescriptionDialog::OnButtonClicked);

            if (m_kinematicSetting)
            {
                m_ui->kinematicRadioButton->setChecked(true);
            }
            else
            {
                m_ui->simulatedRadioButton->setChecked(true);
            }

            // running OnButtonClicked manually to get initial border highlight
            OnButtonClicked();
        }

        void KinematicDescriptionDialog::OnButtonClicked()
        {
            const bool simulated = m_ui->simulatedRadioButton->isChecked();
            KinematicDescriptionDialogFlatInternal::SetBoxLook(m_ui->simulatedBox, simulated);
            KinematicDescriptionDialogFlatInternal::SetBoxLook(m_ui->kinematicBox, !simulated);
            m_kinematicSetting = !simulated;

            UpdateDialogText();
        }

        void KinematicDescriptionDialog::UpdateDialogText()
        {
            if (m_kinematicSetting)
            {
                m_ui->kinematicRadioButton->setChecked(true);

                m_ui->selectedDescriptionLabel->setText(KinematicDescription);

                m_ui->validLabel1->setText(MoveWithCodeText);
                m_ui->validLabel2->setText(MoveManuallyText);
                m_ui->validIcon1->setText(MoveWithCodeIcon);
                m_ui->validIcon2->setText(MoveManuallyIcon);

                m_ui->invalidLabel1->setText(CollisionsText);
                m_ui->invalidLabel2->setText(GravityText);
                m_ui->invalidIcon1->setText(ImpactIcon);
                m_ui->invalidIcon2->setText(GravityIcon);
            }
            else
            {
                m_ui->simulatedRadioButton->setChecked(true);

                m_ui->selectedDescriptionLabel->setText(SimulatedDescription);

                m_ui->validLabel1->setText(CollisionsText);
                m_ui->validLabel2->setText(GravityText);
                m_ui->validIcon1->setText(ImpactIcon);
                m_ui->validIcon2->setText(GravityIcon);

                m_ui->invalidLabel1->setText(MoveWithCodeText);
                m_ui->invalidLabel2->setText(MoveManuallyText);
                m_ui->invalidIcon1->setText(MoveWithCodeIcon);
                m_ui->invalidIcon2->setText(MoveManuallyIcon);
            }
        }

        bool KinematicDescriptionDialog::GetResult() const
        {
            return m_kinematicSetting;
        }
    } // namespace Editor
} // namespace PhysX
