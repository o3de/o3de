/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "SelectDestinationDialog.h"

// Qt
#include <QValidator>
#include <QSettings>
#include <QStyle>
#include <QPushButton>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <AssetImporter/UI/ui_SelectDestinationDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING


static const char* g_assetProcessorLink = "<a href=\"https://www.o3de.org/docs/user-guide/assets/asset-processor/\">Asset Processor</a>";
static const char* g_copyFilesMessage = "The original file will remain outside of the project and the %1 will not monitor the file.";
static const char* g_moveFilesMessage = "The original file will be moved inside of the project and the %1 will monitor the file for changes.";
static const char* g_selectDestinationFilesPath = "AssetImporter/SelectDestinationFilesPath";

static const char* g_toolTipInvalidRoot = "<p style='white-space:pre'> <span style=\"color:#cccccc;\">Invalid directory. Please choose a destination directory within your game project: %1.</span> </p>";
static const char* g_toolTipPathMustExist = "<p style='white-space:pre'> <span style=\"color:#cccccc;\">Invalid directory. Please choose a destination directory that exists.</span> </p>";
static const char* g_toolTipPathMustBeDirectory = "<p style='white-space:pre'> <span style=\"color:#cccccc;\">Invalid directory. Please choose a valid destination directory.</span> </p>";
static const char* g_toolTipInvalidLength = "<p style='white-space:pre'> <span style=\"color:#cccccc;\">Invalid directory name length. Please choose a destination path that has fewer than %1 characters.</span> </p>";

namespace
{
    static QString GetAbsoluteRootDirectoryPath()
    {
        QDir gameRoot(Path::GetEditingGameDataFolder().c_str());
        return gameRoot.absolutePath();
    }
}

class DestinationDialogValidator
    : public QValidator
{
public:
    DestinationDialogValidator(QObject* parent)
        : QValidator(parent)
        , m_gameRootAbsolutePath(GetAbsoluteRootDirectoryPath())
    {
    }

    State validate(QString& input, [[maybe_unused]] int& pos) const override
    {
        m_toolTip = "";

        if (input.isEmpty())
        {
            return QValidator::Acceptable;
        }

        // The underlying file system code can't cope with long file paths, regardless of platform
        if (input.length() > (AZ_MAX_PATH_LEN - 1))
        {
            m_toolTip = QString(g_toolTipInvalidLength).arg(AZ_MAX_PATH_LEN);
            return QValidator::Intermediate;
        }

        QString normalizedInput = QDir::fromNativeSeparators(input);

        // Note: the check for the root directory is case insensitive.
        // We check if the directory actually exists after this, and at that point,
        // if the file path has to be case sensitive, the directory won't exist anyways
        // and QFileInfo::exists() will tell us so this should still work even on
        // case sensitive file systems (such as Mac and Linux)
        if (!normalizedInput.startsWith(m_gameRootAbsolutePath, Qt::CaseInsensitive))
        {
            m_toolTip = QString(g_toolTipInvalidRoot).arg(m_gameRootAbsolutePath);
            return QValidator::Intermediate;
        }

        QFileInfo fileInfo(normalizedInput);
        if (!fileInfo.exists())
        {
            m_toolTip = g_toolTipPathMustExist;
            return QValidator::Intermediate;
        }

        if (!fileInfo.isDir())
        {
            m_toolTip = g_toolTipPathMustBeDirectory;
            return QValidator::Intermediate;
        }

        return QValidator::Acceptable;
    }

    QString infoToolTip() const
    {
        return m_toolTip;
    }

private:
    QString m_gameRootAbsolutePath;
    mutable QString m_toolTip;
};

SelectDestinationDialog::SelectDestinationDialog(QString message, QWidget* parent, QString suggestedDestination)
    : QDialog(parent)
    , m_ui(new Ui::SelectDestinationDialog)
    , m_validator(new DestinationDialogValidator(this))
{
    m_ui->setupUi(this);

    QString radioButtonMessage = QString(g_copyFilesMessage).arg(g_assetProcessorLink);
    m_ui->RaidoButtonMessage->setText(radioButtonMessage);

    SetPreviousOrSuggestedDestinationDirectory(suggestedDestination);

    m_ui->DestinationLineEdit->lineEdit()->setValidator(m_validator);
    m_ui->DestinationLineEdit->setClearButtonEnabled(true);

    connect(m_ui->DestinationLineEdit->lineEdit(), &QLineEdit::textChanged, this, &SelectDestinationDialog::ValidatePath);
    connect(m_ui->DestinationLineEdit, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &SelectDestinationDialog::OnBrowseDestinationFilePath);

    UpdateMessage(message);
    InitializeButtons();
}

SelectDestinationDialog::~SelectDestinationDialog()
{
}

void SelectDestinationDialog::InitializeButtons()
{
    m_ui->CopyFileRadioButton->setChecked(true);

    m_ui->buttonBox->setContentsMargins(0, 0, 16, 16);

    QPushButton* importButton = m_ui->buttonBox->addButton(tr("Import"), QDialogButtonBox::AcceptRole);
    QPushButton* cancelButton = m_ui->buttonBox->addButton(QDialogButtonBox::Cancel);

    importButton->setProperty("class", "Primary");
    importButton->setDefault(true);

    cancelButton->setProperty("class", "AssetImporterButton");
    cancelButton->style()->unpolish(cancelButton);
    cancelButton->style()->polish(cancelButton);
    cancelButton->update();

    connect(importButton, &QPushButton::clicked, this, &SelectDestinationDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &SelectDestinationDialog::reject);
    connect(this, &SelectDestinationDialog::UpdateImportButtonState, importButton, &QPushButton::setEnabled);

    // To make sure the import button state is up to date
    ValidatePath();
    importButton->setAutoDefault(true);

}

void SelectDestinationDialog::SetPreviousOrSuggestedDestinationDirectory(QString suggestedDestination)
{
    QString gameRootAbsPath = GetAbsoluteRootDirectoryPath();
    QSettings settings;
    QString previousDestination = settings.value(g_selectDestinationFilesPath).toString();

    // Case 1: if currentDestination is empty at this point, that means this is the first time
    //         users using the Asset Importer, set the default directory to be the current game project's root folder
    // Case 2: if the current folder directory stored in the registry doesn't exist anymore,
    //         that means users have removed the directory already (deleted or use the Move feature).
    // Case 3: if it's a directory outside of the game root folder, then in general,
    //         users have modified the folder directory in the registry. It should not be happening.
    if (previousDestination.isEmpty() || !QDir(previousDestination).exists() || !previousDestination.startsWith(gameRootAbsPath, Qt::CaseInsensitive))
    {
        previousDestination = gameRootAbsPath;
    }

    if (!suggestedDestination.isEmpty())
    {
        previousDestination = suggestedDestination;
    }

    m_ui->DestinationLineEdit->setText(QDir::toNativeSeparators(previousDestination));
}

void SelectDestinationDialog::accept()
{
    QDialog::accept();

    // This prevent users from not editing the destination line edit (manually type the directory or browse for the directory)
    Q_EMIT SetDestinationDirectory(DestinationDirectory());

    if (m_ui->CopyFileRadioButton->isChecked())
    {
        Q_EMIT DoCopyFiles();
    }
    else if (m_ui->MoveFileRadioButton->isChecked())
    {
        Q_EMIT DoMoveFiles();
    }
}

void SelectDestinationDialog::reject()
{
    Q_EMIT Cancel();
    QDialog::reject();
}

void SelectDestinationDialog::ShowMessage()
{
    QString message = m_ui->CopyFileRadioButton->isChecked() ? g_copyFilesMessage : g_moveFilesMessage;
    m_ui->RaidoButtonMessage->setText(message.arg(g_assetProcessorLink));
}

void SelectDestinationDialog::OnBrowseDestinationFilePath()
{
    Q_EMIT BrowseDestinationPath(m_ui->DestinationLineEdit->lineEdit());
}

void SelectDestinationDialog::UpdateMessage(QString message)
{
    setWindowTitle(message);
}

void SelectDestinationDialog::ValidatePath()
{
    if (!m_ui->DestinationLineEdit->hasAcceptableInput())
    {
        m_ui->DestinationLineEdit->setToolTip(m_validator->infoToolTip());
        Q_EMIT UpdateImportButtonState(false);
    }
    else
    {
        QString destinationDirectory = DestinationDirectory();
        int strLength = destinationDirectory.length();

        // store the updated acceptable destination directory into the registry,
        // so that when users manually modify the directory,
        // the Asset Importer will remember it
        Q_EMIT SetDestinationDirectory(destinationDirectory);

        m_ui->DestinationLineEdit->setToolTip("");
        Q_EMIT UpdateImportButtonState(strLength > 0);   
    } 
}

QString SelectDestinationDialog::DestinationDirectory() const
{
    return QDir::fromNativeSeparators(m_ui->DestinationLineEdit->text());
}

#include <AssetImporter/UI/moc_SelectDestinationDialog.cpp>

