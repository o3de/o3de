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
#include "EditorDefs.h"

#include "NewEntityDialog.h"

// Qt
#include <QPushButton>
#include <QToolTip>
#include <QMessageBox>

// Editor
#include "Dialogs/QT/ui_NewEntityDialog.h"


NewEntityDialog::NewEntityDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::NewEntityDialog)
{
    entityNameValidator = new EntityNameValidator(this);

    ui->setupUi(this);

    ui->entityName->setFocus();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(ui->entityName, SIGNAL(textChanged(QString)), this, SLOT(ValidateInput()));
    connect(ui->categoryName, SIGNAL(textChanged(QString)), this, SLOT(ValidateInput()));

    SetCategoryCompleterPath((Path::GetEditingGameDataFolder() + "/Scripts/Entities").c_str());
    SetNameValidatorPath((Path::GetEditingGameDataFolder() + "/Entities").c_str());
}

NewEntityDialog::~NewEntityDialog()
{
    SAFE_DELETE(entityNameValidator);
    SAFE_DELETE(folderNameCompleter);
    delete ui;
}

void NewEntityDialog::SetCategoryCompleterPath(CryStringT<char> path)
{
    SAFE_DELETE(folderNameCompleter);

    QDirIterator directoryIt(QString::fromLocal8Bit(path.c_str(), path.length()), QDir::NoDotAndDotDot | QDir::AllDirs, QDirIterator::Subdirectories);
    baseDir = directoryIt.path() + "/";

    QStringList dirs;
    while (directoryIt.hasNext())
    {
        QString dir = directoryIt.next().remove(baseDir);
        dirs.append(dir);
    }

    folderNameCompleter = new QCompleter(dirs);
    folderNameCompleter->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    folderNameCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui->categoryName->setCompleter(folderNameCompleter);
}

void NewEntityDialog::SetNameValidatorPath(CryStringT<char> path)
{
    QDir dir(QString::fromLocal8Bit(path.c_str(), path.length()));
    nameBaseDir = dir.path() + "/";
}

void NewEntityDialog::ValidateInput()
{
    int cursorPos = ui->entityName->cursorPosition();
    QString text = ui->entityName->text();
    bool validText = entityNameValidator->validate(text, cursorPos);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(validText);
}

void NewEntityDialog::accept()
{
    if (ui->categoryName->text().isEmpty()
        && QMessageBox::question(this, "Are you sure?", "Create entity without category?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
        return;
    }
    const char* devRoot = gEnv->pFileIO->GetAlias("@engroot@");
    QString devRootPath(devRoot);
    QFile entTemplateFile(devRootPath + "/Editor/NewEntityTemplate.ent_template");
    QFile luaTemplateFile(devRootPath + "/Editor/NewEntityTemplate.lua_template");
    QFile entDestFile(nameBaseDir + ui->entityName->text() + ".ent");
    QFile luaDestFile(baseDir + ui->categoryName->text() + "/" + ui->entityName->text() + ".lua");

    if (!entTemplateFile.exists() || !luaTemplateFile.exists())
    {
        QMessageBox::critical(this, tr("Missing Template Files"), tr("In order to create default entities the NewEntityTemplate.lua and NewEntityTemplate.ent template files must exist in the Templates folder!"));
        return;
    }

    //generate the .ent file
    QDir pathMaker(nameBaseDir);
    pathMaker.mkpath(pathMaker.path());
    QString entFileString;
    if (!entTemplateFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        AZ_Warning("Editor", false, "Enable to open template file for ent : %s", entTemplateFile.fileName().toUtf8().constData());
        return;
    }
    else
    {
        entFileString = entTemplateFile.readAll();
        entTemplateFile.close();
    }

    entFileString.replace(QString("[CATEGORY_NAME]"), ui->categoryName->text());
    entFileString.replace(QString("[ENTITY_NAME]"), ui->entityName->text());
    if (!entDestFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        AZ_Warning("Editor", false, "Enable to open destination file for ent : %s", entDestFile.fileName().toUtf8().constData());
        return;
    }
    else
    {
        entDestFile.write(entFileString.toUtf8());
        entDestFile.close();
    }

    //generate the .lua file
    pathMaker.setPath(baseDir);
    pathMaker.mkpath(ui->categoryName->text() + "/");

    QString luaFileString;
    if (!luaTemplateFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        AZ_Warning("Editor", false, "Enable to open template file for lua : %s", luaTemplateFile.fileName().toUtf8().constData());
        return;
    }
    else
    {
        luaFileString = luaTemplateFile.readAll();
        luaTemplateFile.close();
    }

    luaFileString.replace(QString("[ENTITY_NAME]"), ui->entityName->text());
    if (!luaDestFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        AZ_Warning("Editor", false, "Enable to open destination file for lua : %s", luaDestFile.fileName().toUtf8().constData());
        return;
    }
    else
    {
        luaDestFile.write(luaFileString.toUtf8());
        luaDestFile.close();
    }


    if (ui->openLuaCB->isChecked())
    {
        CFileUtil::EditTextFile(luaDestFile.fileName().toLocal8Bit().data());
    }

    QDialog::accept();
}

QValidator::State NewEntityDialog::EntityNameValidator::validate(QString& input, [[maybe_unused]] int& pos) const
{
    if (!m_Parent)
    {
        return Invalid;
    }

    if (input.isEmpty())
    {
        return Invalid;
    }

    if (input.contains("/"))
    {
        return Invalid;
    }

    QString fileBaseName = m_Parent->ui->entityName->text();

    // Characters
    const char* notAllowedChars = ",^@=+{}[]~!?:&*\"|#%<>$\"'();`' ";
    for (const char* c = notAllowedChars; *c; c++)
    {
        if (fileBaseName.contains(QLatin1Char(*c)))
        {
            const QChar qc = QLatin1Char(*c);
            if (qc.isSpace())
            {
                QToolTip::showText(m_Parent->ui->entityName->mapToGlobal(QPoint()), tr("Name may not contain white space."), m_Parent->ui->entityName, m_Parent->ui->entityName->rect(), 2000);
            }
            else
            {
                QToolTip::showText(m_Parent->ui->entityName->mapToGlobal(QPoint()), tr("Invalid character \"%1\".").arg(qc), m_Parent->ui->entityName, m_Parent->ui->entityName->rect(), 2000);
            }
            return Invalid;
        }
    }

    QString filename(m_Parent->nameBaseDir + m_Parent->ui->entityName->text() + ".ent");
    QFile newFile(filename);

    if (newFile.exists())
    {
        QToolTip::showText(m_Parent->ui->entityName->mapToGlobal(QPoint()), tr("Filename already exists!"), m_Parent->ui->entityName, m_Parent->ui->entityName->rect(), 2000);
        return Invalid;
    }

    return Acceptable;
}

#include <Dialogs/QT/moc_NewEntityDialog.cpp>
