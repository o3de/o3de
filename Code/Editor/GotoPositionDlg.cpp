/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "GotoPositionDlg.h"

// Editor
#include "ViewManager.h"
#include "GameEngine.h"


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_GotoPositionDlg.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

/////////////////////////////////////////////////////////////////////////////
// CGotoPositionDlg dialog


CGotoPositionDlg::CGotoPositionDlg(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_ui(new Ui::GotoPositionDlg)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(size());
    OnInitDialog();

    auto doubleValueChanged = static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
    auto valueChanged = static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged);

    connect(m_ui->m_posEdit, &QLineEdit::editingFinished, this, &CGotoPositionDlg::OnChangeEdit);
    connect(m_ui->m_dymX, doubleValueChanged, this, &CGotoPositionDlg::OnUpdateNumbers);
    connect(m_ui->m_dymY, doubleValueChanged, this, &CGotoPositionDlg::OnUpdateNumbers);
    connect(m_ui->m_dymZ, doubleValueChanged, this, &CGotoPositionDlg::OnUpdateNumbers);
    connect(m_ui->m_dymAngleX, doubleValueChanged, this, &CGotoPositionDlg::OnUpdateNumbers);
    connect(m_ui->m_dymAngleY, doubleValueChanged, this, &CGotoPositionDlg::OnUpdateNumbers);
    connect(m_ui->m_dymAngleZ, doubleValueChanged, this, &CGotoPositionDlg::OnUpdateNumbers);
    connect(m_ui->m_dymSegX, valueChanged, this, &CGotoPositionDlg::OnUpdateNumbers);
    connect(m_ui->m_dymSegY, valueChanged, this, &CGotoPositionDlg::OnUpdateNumbers);
}

CGotoPositionDlg::~CGotoPositionDlg()
{
}

/////////////////////////////////////////////////////////////////////////////
// CGotoPositionDlg message handlers


void CGotoPositionDlg::OnInitDialog()
{
    Vec3 pos;
    Ang3 angle;

    CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
    if (pRenderViewport)
    {
        Matrix34 tm = pRenderViewport->GetViewTM();
        pos   = pRenderViewport->GetViewTM().GetTranslation();
        angle = RAD2DEG(Ang3::GetAnglesXYZ(tm));
    }

    // CORDS --------------------------------------
    m_ui->m_dymX->setRange(-64000, 64000);
    m_ui->m_dymX->setValue(pos.x);

    m_ui->m_dymY->setRange(-64000, 64000);
    m_ui->m_dymY->setValue(pos.y);

    m_ui->m_dymZ->setRange(-64000, 64000);
    m_ui->m_dymZ->setValue(pos.z);

    // ANGLES -------------------------------------
    m_ui->m_dymAngleX->setRange(-180, 180);
    m_ui->m_dymAngleX->setValue(angle.x);

    m_ui->m_dymAngleY->setRange(-180, 180);
    m_ui->m_dymAngleY->setValue(angle.y);

    m_ui->m_dymAngleZ->setRange(-180, 180);
    m_ui->m_dymAngleZ->setValue(angle.z);


    m_ui->m_labelSeg->setVisible(false);
    m_ui->m_labelSegX->setVisible(false);
    m_ui->m_labelSegY->setVisible(false);
    m_ui->m_dymSegX->setVisible(false);
    m_ui->m_dymSegY->setVisible(false);

    // Ensure the goto button is highlighted correctly.
    m_ui->pushButton->setDefault(true);

    OnUpdateNumbers();
}


void CGotoPositionDlg::OnChangeEdit()
{
    const int lengthInSw = 8;
    const int strNum = 6;
    AZStd::vector<float> pos(strNum);

    m_sPos = m_ui->m_posEdit->text();
    const QStringList parts = m_sPos.split(QRegularExpression("[\\s,;\\t]"), Qt::SkipEmptyParts);
    for (int k = 0; k < strNum && k < parts.count(); ++k)
    {
        pos[k] = parts[k].toDouble();
    }

    m_ui->m_dymX->setValue(pos[0]);
    m_ui->m_dymY->setValue(pos[1]);
    m_ui->m_dymZ->setValue(pos[2]);
    m_ui->m_dymAngleX->setValue(pos[3]);
    m_ui->m_dymAngleY->setValue(pos[4]);
    m_ui->m_dymAngleZ->setValue(pos[5]);

    if constexpr (strNum == lengthInSw)
    {
        if (parts.count() == lengthInSw)
        {
            m_ui->m_dymSegX->setValue(static_cast<int>(parts[6].toDouble()));
            m_ui->m_dymSegY->setValue(static_cast<int>(parts[7].toDouble()));
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CGotoPositionDlg::OnUpdateNumbers()
{
    m_ui->m_posEdit->setText(QString::fromLatin1("%1, %2, %3, %4, %5, %6")
            .arg(m_ui->m_dymX->value(), 2, 'f', 2).arg(m_ui->m_dymY->value(), 2, 'f', 2).arg(m_ui->m_dymZ->value(), 2, 'f', 2)
            .arg(m_ui->m_dymAngleX->value(), 2, 'f', 2).arg(m_ui->m_dymAngleY->value(), 2, 'f', 2).arg(m_ui->m_dymAngleZ->value(), 2, 'f', 2));
}


void CGotoPositionDlg::accept()
{
    Vec3 vPos(m_ui->m_dymX->value(), m_ui->m_dymY->value(), m_ui->m_dymZ->value());

    m_ui->m_dymX->setValue(vPos.x);
    m_ui->m_dymY->setValue(vPos.y);
    m_ui->m_dymZ->setValue(vPos.z);

    AzToolsFramework::IEditorCameraController* editorCameraController = AZ::Interface<AzToolsFramework::IEditorCameraController>::Get();
    AZ_Error("editor", editorCameraController, "IEditorCameraCommands is not registered.");
    if (editorCameraController)
    {
        editorCameraController->SetCurrentViewPosition(AZ::Vector3{
            aznumeric_cast<float>(m_ui->m_dymX->value()),
            aznumeric_cast<float>(m_ui->m_dymY->value()),
            aznumeric_cast<float>(m_ui->m_dymZ->value())
            });
        editorCameraController->SetCurrentViewRotation(AZ::Vector3{
            aznumeric_cast<float>(m_ui->m_dymAngleX->value()),
            aznumeric_cast<float>(m_ui->m_dymAngleY->value()),
            aznumeric_cast<float>(m_ui->m_dymAngleZ->value())
            });
    }

    QDialog::accept();
}

#include <moc_GotoPositionDlg.cpp>
