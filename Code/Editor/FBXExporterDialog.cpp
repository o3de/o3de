/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "FBXExporterDialog.h"

#include <QMessageBox>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_FBXExporterDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

CFBXExporterDialog::CFBXExporterDialog(bool bDisplayOnlyFPSSetting, QWidget* pParent)
    : QDialog(pParent)
    , m_ui(new Ui::FBXExporterDialog)
{
    m_ui->setupUi(this);
    setFixedSize(size());
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_ui->m_exportLocalCoordsCheckbox->setChecked(false);
    m_bDisplayOnlyFPSSetting = bDisplayOnlyFPSSetting;

    connect(m_ui->m_fpsCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &CFBXExporterDialog::OnFPSChange);
}

CFBXExporterDialog::~CFBXExporterDialog()
{
}

float CFBXExporterDialog::GetFPS() const
{
    return m_ui->m_fpsCombo->currentText().toFloat();
}

bool CFBXExporterDialog::GetExportCoordsLocalToTheSelectedObject() const
{
    return m_ui->m_exportLocalCoordsCheckbox->isChecked();
}

bool CFBXExporterDialog::GetExportOnlyPrimaryCamera() const
{
    return m_ui->m_exportOnlyPrimaryCameraCheckBox->isChecked();
}

void CFBXExporterDialog::SetExportLocalCoordsCheckBoxEnable(bool checked)
{
    if (!m_bDisplayOnlyFPSSetting)
    {
        m_ui->m_exportLocalCoordsCheckbox->setEnabled(checked);
    }
}

void CFBXExporterDialog::accept()
{
    const QString fpsStr = m_ui->m_fpsCombo->currentText();

    bool ok;
    const double fps = fpsStr.toDouble(&ok);

    if (fps <= 0 && !fpsStr.isEmpty() || fpsStr.isEmpty() || !ok)
    {
        QMessageBox::information(this, QString(), tr("Please enter a correct FPS value"));
        m_ui->m_fpsCombo->setCurrentIndex(2);
        return;
    }

    QDialog::accept();
}

void CFBXExporterDialog::OnFPSChange()
{
    const QString fpsStr = m_ui->m_fpsCombo->currentText();

    if (QString::compare(fpsStr, tr("Custom"), Qt::CaseInsensitive) == 0)
    {
        m_ui->m_fpsCombo->setCurrentIndex(-1);
    }
}

int CFBXExporterDialog::exec()
{
    if (m_bDisplayOnlyFPSSetting)
    {
        m_ui->m_exportLocalCoordsCheckbox->setEnabled(false);
        m_ui->m_exportOnlyPrimaryCameraCheckBox->setEnabled(false);
    }

    m_ui->m_fpsCombo->addItem("24");
    m_ui->m_fpsCombo->addItem("25");
    m_ui->m_fpsCombo->addItem("30");
    m_ui->m_fpsCombo->addItem("48");
    m_ui->m_fpsCombo->addItem("60");
    m_ui->m_fpsCombo->addItem(tr("Custom"));
    m_ui->m_fpsCombo->setCurrentIndex(2);

    return QDialog::exec();
}

#include <moc_FBXExporterDialog.cpp>
