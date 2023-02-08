/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/KinematicDescriptionDialog.h>

#include <Editor/ui_KinematicDescriptionDialog.h>

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
        KinematicDescriptionDialog::KinematicDescriptionDialog(bool kinematicSetting, QWidget* parent)
            : QDialog(parent)
            , m_ui(new Ui::KinematicDescriptionDialog)
            , m_kinematicSetting(kinematicSetting)
        {
            m_ui->setupUi(this);

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
            const QString boxStyleSheet = QString("background-color: rgb(51, 51, 51);");
            const QString boxBorderStyleSheet = QString(" border: 1px solid rgb(30, 112, 235);");

            if (m_ui->simulatedRadioButton->isChecked())
            {
                m_ui->simulatedBox->setStyleSheet(boxStyleSheet + boxBorderStyleSheet);
                m_ui->kinematicBox->setStyleSheet(boxStyleSheet + " border: none;");
                m_kinematicSetting = false;
            }
            else
            {
                m_ui->kinematicBox->setStyleSheet(boxStyleSheet + boxBorderStyleSheet);
                m_ui->simulatedBox->setStyleSheet(boxStyleSheet + " border: none;");
                m_kinematicSetting = true;
            }

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

#include <moc_KinematicDescriptionDialog.cpp>
