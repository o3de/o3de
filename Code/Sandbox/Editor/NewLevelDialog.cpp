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
#include <QTimer>

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
    , m_ilevelFolders(0)
    , m_bUpdate(false)
    , ui(new Ui::CNewLevelDialog)
    , m_initialized(false)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("New Level"));
    setMaximumSize(QSize(320, 280));
    adjustSize();

    // Default level folder is root (Levels/)
    m_ilevelFolders = 0;

    m_bIsResize = false;

    // Level name only supports ASCII characters
    QRegExp rx("[_a-zA-Z0-9-]+");
    QValidator* validator = new QRegExpValidator(rx, this);
    ui->LEVEL->setValidator(validator);

    connect(ui->LEVEL_FOLDERS, SIGNAL(activated(int)), this, SLOT(OnCbnSelendokLevelFolders()));
    connect(ui->LEVEL, &QLineEdit::textChanged, this, &CNewLevelDialog::OnLevelNameChange);

    // First of all, keyboard focus is related to widget tab order, and the default tab order is based on the order in which 
    // widgets are constructed. Therefore, creating more widgets changes the keyboard focus. That is why setFocus() is called last.
    // Secondly, using singleShot() allows setFocus() slot of the QLineEdit instance to be invoked right after the event system
    // is ready to do so. Therefore, it is better to use singleShot() than directly call setFocus().
    QTimer::singleShot(0, ui->LEVEL, SLOT(setFocus()));
}

CNewLevelDialog::~CNewLevelDialog()
{
}

void CNewLevelDialog::UpdateData(bool fromUi)
{
    if (fromUi)
    {
        m_level = ui->LEVEL->text();
        m_levelFolders = ui->LEVEL_FOLDERS->currentText();
        m_ilevelFolders = ui->LEVEL_FOLDERS->currentIndex();
    }
    else
    {
        ui->LEVEL->setText(m_level);
        ui->LEVEL_FOLDERS->setCurrentText(m_levelFolders);
        ui->LEVEL_FOLDERS->setCurrentIndex(m_ilevelFolders);
    }
}

// CNewLevelDialog message handlers

void CNewLevelDialog::OnInitDialog()
{
    ReloadLevelFolders();

    // Disable OK until some text is entered
    if (QPushButton* button = ui->buttonBox->button(QDialogButtonBox::Ok))
    {
        button->setEnabled(false);
    }

    // Save data.
    UpdateData(false);
}


//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::ReloadLevelFolders()
{
    QString levelsFolder = QString(Path::GetEditingGameDataFolder().c_str()) + "/" + kNewLevelDialog_LevelsFolder;

    m_itemFolders.clear();
    ui->LEVEL_FOLDERS->clear();
    ui->LEVEL_FOLDERS->addItem(QString(kNewLevelDialog_LevelsFolder) + '/');
    ReloadLevelFoldersRec(levelsFolder);
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::ReloadLevelFoldersRec(const QString& currentFolder)
{
    QDir dir(currentFolder);

    QFileInfoList infoList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

    foreach(const QFileInfo &fi, infoList)
    {
        m_itemFolders.push_back(fi.baseName());
        ui->LEVEL_FOLDERS->addItem(QString(kNewLevelDialog_LevelsFolder) + '/' + fi.baseName());
    }
}

//////////////////////////////////////////////////////////////////////////
QString CNewLevelDialog::GetLevel() const
{
    QString output = m_level;

    if (m_itemFolders.size() > 0 && m_ilevelFolders > 0)
    {
        output = m_itemFolders[m_ilevelFolders - 1] + "/" + m_level;
    }

    return output;
}

//////////////////////////////////////////////////////////////////////////
void CNewLevelDialog::OnCbnSelendokLevelFolders()
{
    UpdateData();
}

void CNewLevelDialog::OnLevelNameChange()
{
    m_level = ui->LEVEL->text();

    // QRegExpValidator means the string will always be valid as long as it's not empty:
    const bool valid = !m_level.isEmpty();

    // Use the validity to dynamically change the Ok button's enabled state
    if (QPushButton* button = ui->buttonBox->button(QDialogButtonBox::Ok))
    {
        button->setEnabled(valid);
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
