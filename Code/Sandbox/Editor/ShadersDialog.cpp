/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorDefs.h"

#include "ShadersDialog.h"

// Qt
#include <QStringListModel>

// Editor
#include "ShaderEnum.h"


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_ShadersDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

/////////////////////////////////////////////////////////////////////////////
// CShadersDialog dialog


CShadersDialog::CShadersDialog(const QString& selection, QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_shadersModel(new QStringListModel(this))
    , ui(new Ui::CShadersDialog)
    , m_selection(selection)
{
    ui->setupUi(this);
    ui->m_shaders->setModel(m_shadersModel);
    OnInitDialog();

    connect(ui->m_shaders->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CShadersDialog::OnSelchangeShaders);
    connect(ui->m_shaders, &QListView::doubleClicked, this, &CShadersDialog::OnDblclkShaders);
    connect(ui->m_shaderText, &QTextEdit::textChanged, this, &CShadersDialog::OnEnChangeText);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->m_saveButton, &QPushButton::clicked, this, &CShadersDialog::OnBnClickedSave);
    connect(ui->m_editButton, &QPushButton::clicked, this, &CShadersDialog::OnBnClickedEdit);
}


CShadersDialog::~CShadersDialog()
{
}


void CShadersDialog::OnSelchangeShaders()
{
    // When shader changes.
    // Edit shader file.
    auto index = ui->m_shaders->currentIndex();
    if (index.isValid())
    {
        QString file = GetIEditor()->GetShaderEnum()->GetShaderFile(index.row());
        file.replace('/', '\\');
        ui->m_shaderText->LoadFile(file);
        // Just loaded file.. Not savable.
        ui->m_saveButton->setEnabled(false);

        QString shaderName = QStringLiteral("'%1'").arg(index.data().toString());

        if (ui->m_shaderText->find(shaderName))
        {
            auto cursor = ui->m_shaderText->textCursor();
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, shaderName.size());
        }

        m_selection = index.data().toString();
    }
}

void CShadersDialog::OnInitDialog()
{
    QWaitCursor wait;

    // Fill with shaders.
    CShaderEnum* shaderEnum = GetIEditor()->GetShaderEnum();
    int numShaders = shaderEnum->EnumShaders();

    QStringList shaders;
    for (int i = 0; i < numShaders; i++)
    {
        shaders.append(shaderEnum->GetShader(i));
    }
    m_shadersModel->setStringList(shaders);

    /*
    if (numShaders > 0)
    {
        int i = m_shaders.FindString(m_sel);
        if (i != LB_ERR)
            m_shaders.SetCurSel( i );
    }
    */
}

void CShadersDialog::OnDblclkShaders()
{
    // Same as IDOK.
    accept();
}

void CShadersDialog::OnBnClickedEdit()
{
    // Edit shader file.
    auto index = ui->m_shaders->currentIndex();
    if (index.isValid())
    {
        CShaderEnum* shaderEnum = GetIEditor()->GetShaderEnum();
        QString file = shaderEnum->GetShaderFile(index.row());
        CFileUtil::EditTextFile(file.toUtf8().data(), IFileUtil::FILE_TYPE_SHADER);
    }
}

//////////////////////////////////////////////////////////////////////////
void CShadersDialog::OnBnClickedSave()
{
    if (ui->m_shaderText->IsModified())
    {
        ui->m_shaderText->SaveFile(ui->m_shaderText->GetFilename());
        if (ui->m_shaderText->IsModified())
        {
            ui->m_saveButton->setEnabled(true);
        }
        else
        {
            ui->m_saveButton->setEnabled(false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CShadersDialog::OnEnChangeText()
{
    // File can be saved.
    ui->m_saveButton->setEnabled(true);
}

#include <moc_ShadersDialog.cpp>
