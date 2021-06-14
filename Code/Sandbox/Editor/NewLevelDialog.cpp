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

#include "NewLevelDialog.h"

// Qt
#include <QtWidgets/QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QToolButton>

// Editor
#include "NewTerrainDialog.h"  

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_NewLevelDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING


// Folder in which levels are stored
static const char kNewLevelDialog_LevelsFolder[] = "Levels";

// CNewLevelDialog dialog

CNewLevelDialog::CNewLevelDialog(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_bUpdate(false)
    , ui(new Ui::CNewLevelDialog)
    , m_initialized(false)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("New Level"));
    setMaximumSize(QSize(430, 180));
    adjustSize();

    m_bIsResize = false;

    
    ui->TITLE->setText(tr("Assign a name and location to the new level."));
    ui->STATIC1->setText(tr("Location:"));
    ui->STATIC2->setText(tr("Name:"));

    // Level name only supports ASCII characters
    QRegExp rx("[_a-zA-Z0-9-]+");
    QValidator* validator = new QRegExpValidator(rx, this);
    ui->LEVEL->setValidator(validator);

    ui->LEVEL_FOLDERS->setClearButtonEnabled(true);
    QToolButton* clearButton = AzQtComponents::LineEdit::getClearButton(ui->LEVEL_FOLDERS->lineEdit());
    assert(clearButton);
    connect(clearButton, &QToolButton::clicked, this, &CNewLevelDialog::OnClearButtonClicked);

    connect(ui->LEVEL_FOLDERS->lineEdit(), &QLineEdit::textEdited, this, &CNewLevelDialog::OnLevelNameChange);
    connect(ui->LEVEL_FOLDERS, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &CNewLevelDialog::PopupAssetPicker);

    connect(ui->LEVEL, &QLineEdit::textChanged, this, &CNewLevelDialog::OnLevelNameChange);

    disconnect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect( ui->buttonBox, SIGNAL(clicked(QAbstractButton*)),this, SLOT(buttonClicked(QAbstractButton*)));

    m_levelFolders = GetLevelsFolder();
    m_level = "";
    // First of all, keyboard focus is related to widget tab order, and the default tab order is based on the order in which 
    // widgets are constructed. Therefore, creating more widgets changes the keyboard focus. That is why setFocus() is called last.
    // Secondly, using singleShot() allows setFocus() slot of the QLineEdit instance to be invoked right after the event system
    // is ready to do so. Therefore, it is better to use singleShot() than directly call setFocus().
    QTimer::singleShot(0, ui->LEVEL, SLOT(onStartup()));

    ReloadLevelFolder();
}

CNewLevelDialog::~CNewLevelDialog()
{
}

void CNewLevelDialog::buttonClicked(QAbstractButton* button)
{
    QDialogButtonBox::StandardButton stdButton = ui->buttonBox->standardButton(button);
    switch (stdButton)
    {
    case QDialogButtonBox::Ok:
        if (!ValidateLevel())
        {
            QDir projectDir = QDir(Path::GetEditingGameDataFolder().c_str());
            QMessageBox::warning(this,
            QStringLiteral("Error creating level."),
                QString("The level must be created in a subdirectory of the %1 folder in the current project. (%2/%3)")
                    .arg(kNewLevelDialog_LevelsFolder)
                    .arg(projectDir.absolutePath())
                    .arg(kNewLevelDialog_LevelsFolder),
            QMessageBox::Ok);
        }
        else
        {
            done(Accepted);
        }
        break;
    default:
        break;
    }
}

void CNewLevelDialog::onStartup()
{
    UpdateData(false);
    setFocus();
}

void CNewLevelDialog::UpdateData(bool fromUi)
{
    if (fromUi)
    {
        m_level = ui->LEVEL->text();
        m_levelFolders = ui->LEVEL_FOLDERS->text();
    }
    else
    {
        ui->LEVEL->setText(m_level);
        ui->LEVEL_FOLDERS->lineEdit()->setText(m_levelFolders);
    }
}

// CNewLevelDialog message handlers

void CNewLevelDialog::OnInitDialog()
{
    ReloadLevelFolder();

    // Disable OK until some text is entered
    if (QPushButton* button = ui->buttonBox->button(QDialogButtonBox::Ok))
    {
        button->setEnabled(false);
    }

    // Save data.
    UpdateData(false);
}


//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::ReloadLevelFolder()
{
    m_itemFolders.clear();
    ui->LEVEL_FOLDERS->lineEdit()->clear();
    ui->LEVEL_FOLDERS->setText(QString(kNewLevelDialog_LevelsFolder) + '/');
}

QString CNewLevelDialog::GetLevelsFolder() const
{
    QDir projectDir = QDir(Path::GetEditingGameDataFolder().c_str());
    QDir projectLevelsDir = QDir(QStringLiteral("%1/%2").arg(projectDir.absolutePath()).arg(kNewLevelDialog_LevelsFolder));

    return projectLevelsDir.absolutePath();
}

//////////////////////////////////////////////////////////////////////////
QString CNewLevelDialog::GetLevel() const
{
    QString output = m_level;

    QDir projectLevelsDir = QDir(GetLevelsFolder());

    if (!m_levelFolders.isEmpty())
    {
        output = m_levelFolders + "/" + m_level;
    }

    QString relativePath = projectLevelsDir.relativeFilePath(output);

    return relativePath;
}

bool CNewLevelDialog::ValidateLevel()
{
    // Check that the selected folder is in or below the project/LEVELS folder.
    QDir projectLevelsDir = QDir(GetLevelsFolder());
    QString selectedFolder = ui->LEVEL_FOLDERS->text();
    QString absolutePath = QDir::cleanPath(projectLevelsDir.absoluteFilePath(selectedFolder));

    if (absolutePath == GetLevelsFolder())
    {
        return true;
    }

    QString relativePath = projectLevelsDir.relativeFilePath(selectedFolder);

    if (relativePath.startsWith(".."))
    {
        return false;
    }

    return true;
}

void CNewLevelDialog::OnLevelNameChange()
{
    UpdateData(true);

    // QRegExpValidator means the string will always be valid as long as it's not empty:
    const bool valid = !m_level.isEmpty();

    // Use the validity to dynamically change the Ok button's enabled state
    if (QPushButton* button = ui->buttonBox->button(QDialogButtonBox::Ok))
    {
        button->setEnabled(valid);
    }
}

void CNewLevelDialog::OnClearButtonClicked()
{
    ui->LEVEL_FOLDERS->lineEdit()->setText(GetLevelsFolder());
    UpdateData(true);

}

void CNewLevelDialog::PopupAssetPicker()
{
    QString newPath = QFileDialog::getExistingDirectory(nullptr, QObject::tr("Choose Destination Folder"), GetLevelsFolder());

    if (!newPath.isEmpty())
    {
        ui->LEVEL_FOLDERS->setText(newPath);
        OnLevelNameChange();
    }
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::IsResize(bool bIsResize)
{
    m_bIsResize = bIsResize;
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::showEvent(QShowEvent* event)
{
    if (!m_initialized)
    {
        OnInitDialog();
        m_initialized = true;
    }
    QDialog::showEvent(event);
}

#include <moc_NewLevelDialog.cpp>
