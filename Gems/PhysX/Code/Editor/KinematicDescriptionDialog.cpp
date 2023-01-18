/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "KinematicDescriptionDialog.h"
#include "EditorDefs.h"

// Qt
#include <QPushButton>
#include <QStyle>


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <Editor/ui_KinematicDescriptionDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

constexpr AZStd::string_view dynamicDescription = "With <b>Dynamic</b> rigid bodies, you can physics forces "
                                                  "(gravity, collision, etc.) to control the movement and "
                                                  "position of the object.";

constexpr AZStd::string_view kinematicDescription = "With <b>Kinematic</b> rigid bodies, you can use Transform to "
                                                    "control the movement and position of the object. ";

constexpr AZStd::string_view moveWithCode = "Move with code";
constexpr AZStd::string_view moveManually = "Move manually";
constexpr AZStd::string_view collisions = "Collisions";
constexpr AZStd::string_view gravity = "Gravity";


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
    connect(m_ui->dynamicRadioButton, &QPushButton::clicked, this, &KinematicDescriptionDialog::OnButtonClicked);
    connect(m_ui->kinematicRadioButton, &QPushButton::clicked, this, &KinematicDescriptionDialog::OnButtonClicked);
    connect(m_ui->dynamicBox, &QGroupBox::clicked, this, &KinematicDescriptionDialog::OnButtonClicked);
    connect(m_ui->kinematicBox, &QGroupBox::clicked, this, &KinematicDescriptionDialog::OnButtonClicked);

    m_kinematicSetting ? m_ui->kinematicRadioButton->setChecked(true) : m_ui->dynamicRadioButton->setChecked(true);

    OnButtonClicked();
}

void KinematicDescriptionDialog::UpdateDialogText()
{
    if (m_kinematicSetting)
    {
        m_ui->kinematicRadioButton->setChecked(true);

        m_ui->selectedDescriptionLabel->setText(AZStd::string(kinematicDescription).c_str());

        m_ui->validLabel1->setText(AZStd::string(moveWithCode).c_str());
        m_ui->validLabel2->setText(AZStd::string(moveManually).c_str());
        m_ui->validIcon1->setText("<img src=\":/stylesheet/img/16x16/move_with_code.svg\"/>");
        m_ui->validIcon2->setText("<img src=\":/stylesheet/img/16x16/move_manually.svg\"/>");

        m_ui->invalidLabel1->setText(AZStd::string(collisions).c_str());
        m_ui->invalidLabel2->setText(AZStd::string(gravity).c_str());
        m_ui->invalidIcon1->setText("<img src=\":/stylesheet/img/16x16/impact.svg\"/>");
        m_ui->invalidIcon2->setText("<img src=\":/stylesheet/img/16x16/gravity.svg\"/>");
    }
    else
    {
        m_ui->dynamicRadioButton->setChecked(true);

        m_ui->selectedDescriptionLabel->setText(AZStd::string(dynamicDescription).c_str());

        m_ui->validLabel1->setText(AZStd::string(collisions).c_str());
        m_ui->validLabel2->setText(AZStd::string(gravity).c_str());
        m_ui->validIcon1->setText("<img src=\":/stylesheet/img/16x16/impact.svg\"/>");
        m_ui->validIcon2->setText("<img src=\":/stylesheet/img/16x16/gravity.svg\"/>");
        
        m_ui->invalidLabel1->setText(AZStd::string(moveWithCode).c_str());
        m_ui->invalidLabel2->setText(AZStd::string(moveManually).c_str());
        m_ui->invalidIcon1->setText("<img src=\":/stylesheet/img/16x16/move_with_code.svg\"/>");
        m_ui->invalidIcon2->setText("<img src=\":/stylesheet/img/16x16/move_manually.svg\"/>");


    }
}

void KinematicDescriptionDialog::OnButtonClicked()
{
    QString boxStyleSheet = QString("background-color: rgb(51, 51, 51);");
    QString boxBorderStyleSheet = QString(" border: 1px solid rgb(30, 112, 235);");

    if (m_ui->dynamicRadioButton->isChecked())
    {
        m_ui->dynamicBox->setStyleSheet(boxStyleSheet + boxBorderStyleSheet);
        m_ui->kinematicBox->setStyleSheet(boxStyleSheet + " border: none;");
        m_kinematicSetting = false;
    }
    else
    {
        m_ui->kinematicBox->setStyleSheet(boxStyleSheet + boxBorderStyleSheet);
        m_ui->dynamicBox->setStyleSheet(boxStyleSheet + " border: none;");
        m_kinematicSetting = true;
    }

    UpdateDialogText();
}

bool KinematicDescriptionDialog::GetResult() const
{
    return m_kinematicSetting;
}

#include <moc_KinematicDescriptionDialog.cpp>
