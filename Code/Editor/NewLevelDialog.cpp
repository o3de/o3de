/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"
#include <AzCore/Utils/Utils.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include "NewLevelDialog.h"

// Qt
#include <QtWidgets/QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QToolButton>
#include <QListWidgetItem>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_NewLevelDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING


// Folder in which levels are stored
static const char kNewLevelDialog_LevelsFolder[] = "Levels";
static constexpr const char* RegistryKey_CustomTemplatePaths = "/O3DE/Preferences/Prefab/CustomTemplatePaths";
static constexpr const char* DefaultTemplate = "Default_Level.prefab";

class LevelFolderValidator : public QValidator
{
public:
    LevelFolderValidator(QObject* parent)
        : QValidator(parent)
    {
        m_parentDialog = qobject_cast<CNewLevelDialog*>(parent);
    }

    QValidator::State validate([[maybe_unused]] QString& input, [[maybe_unused]] int& pos) const override
    {
        if (m_parentDialog->ValidateLevel())
        {
            return QValidator::Acceptable;
        }

        return QValidator::Intermediate;
    }

private:
    CNewLevelDialog* m_parentDialog;
};

static QString ChangeFileExtension(const QString& filePath, const QString& newExtension)
{
    QFileInfo fileInfo(filePath);
    QString newFilePath = fileInfo.absolutePath() + QDir::separator() + fileInfo.baseName() + "." + newExtension;
    return newFilePath;
}

// CNewLevelDialog dialog

CNewLevelDialog::CNewLevelDialog(QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , ui(new Ui::CNewLevelDialog)
    , m_initialized(false)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("New Level"));
    setStyleSheet("QListWidget::item {height: 148px; padding-left: 0px; padding-right: 0px; background-color: transparent;}");
    InitTemplateListWidget();

    // Level name only supports ASCII characters
    QRegExp rx("[_a-zA-Z0-9-]+");
    QValidator* validator = new QRegExpValidator(rx, this);
    ui->LEVEL->setValidator(validator);

    validator = new LevelFolderValidator(this);
    ui->LEVEL_FOLDERS->lineEdit()->setValidator(validator);
    ui->LEVEL_FOLDERS->setErrorToolTip(
        QString("The location must be a folder underneath the current project's %1 folder. (%2)")
            .arg(kNewLevelDialog_LevelsFolder)
            .arg(GetLevelsFolder()));

    ui->LEVEL_FOLDERS->setClearButtonEnabled(true);
    QToolButton* clearButton = AzQtComponents::LineEdit::getClearButton(ui->LEVEL_FOLDERS->lineEdit());
    assert(clearButton);
    connect(clearButton, &QToolButton::clicked, this, &CNewLevelDialog::OnClearButtonClicked);

    connect(ui->LEVEL_FOLDERS->lineEdit(), &QLineEdit::textEdited, this, &CNewLevelDialog::OnLevelNameChange);
    connect(ui->LEVEL_FOLDERS, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &CNewLevelDialog::PopupAssetPicker);
    connect(ui->LEVEL, &QLineEdit::textChanged, this, &CNewLevelDialog::OnLevelNameChange);

    m_levelFolders = GetLevelsFolder();
    m_level = "";
    // First of all, keyboard focus is related to widget tab order, and the default tab order is based on the order in which
    // widgets are constructed. Therefore, creating more widgets changes the keyboard focus. That is why setFocus() is called last.
    // in OnStartup()
    // Secondly, using singleShot() allows OnStartup() slot of the QLineEdit instance to be invoked right after the event system
    // is ready to do so. Therefore, it is better to use singleShot() than directly call OnStartup().
    QTimer::singleShot(0, this, &CNewLevelDialog::OnStartup);

    ReloadLevelFolder();
}



CNewLevelDialog::~CNewLevelDialog()
{
}

void CNewLevelDialog::InitTemplateListWidget() const
{
    ui->listTemplates->clear();

    QStringList templatePaths;
    if (AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry)
    {
        auto AppendCustomTemplatePath = [&templatePaths](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            AZ::IO::FixedMaxPath customTemplatePath;
            if (visitArgs.m_registry.Get(customTemplatePath.Native(), visitArgs.m_jsonKeyPath))
            {
                if (AZ::IO::FileIOBase::GetInstance()->ResolvePath(customTemplatePath, customTemplatePath))
                {
                    templatePaths.push_back(QString::fromUtf8(customTemplatePath.c_str(), int(customTemplatePath.Native().size())));
                }
            }

            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };

        AZ::SettingsRegistryVisitorUtils::VisitObject(*settingsRegistry, AppendCustomTemplatePath, RegistryKey_CustomTemplatePaths);
    }

    // Get all prefab files.
    const QStringList fileFilter = {"*.prefab"};
    QStringList allTemplateFiles;
    int defaultItem = 0;
    for(const QString& path: templatePaths)
    {
        QDir projectTemplateDirectory(path);
        projectTemplateDirectory.setNameFilters(fileFilter);

        const QStringList projectTemplateFiles = projectTemplateDirectory.entryList(QDir::Files);
        for (const QString& fileName: projectTemplateFiles)
        {
            if (fileName.compare(QString::fromUtf8(DefaultTemplate), Qt::CaseInsensitive) == 0)
            {
                defaultItem = allTemplateFiles.size();
            }
            allTemplateFiles.push_back(projectTemplateDirectory.filePath(fileName));
        }
    }

    // Create the item with its icons to the QListWidget.
    const QIcon defaultIcon(":/NewLevel/res/Prefab_80.svg");
    for (const QString& fileName: allTemplateFiles)
    {
        QFileInfo info(fileName);
        auto* item = new QListWidgetItem(info.baseName());
        const QString iconPath = ChangeFileExtension(fileName, "png");
        const QIcon itemIcon = QFile::exists(iconPath) ? QIcon(iconPath) : defaultIcon;
        item->setIcon(itemIcon);
        item->setData(Qt::UserRole, fileName);
        ui->listTemplates->addItem(item);
    }

    const QSize iconSize(128, 128);
    ui->listTemplates->setViewMode(QListWidget::IconMode);
    ui->listTemplates->setIconSize(iconSize);
    ui->listTemplates->setDragDropMode(QAbstractItemView::NoDragDrop);
    if (ui->listTemplates->count() > 0)
    {
        ui->listTemplates->setCurrentRow(defaultItem);
    }
}

QString CNewLevelDialog::GetTemplateName() const
{
    const auto* item = ui->listTemplates->currentItem();
    if (item == nullptr)
    {
        if (ui->listTemplates->count() > 0)
        {
            // for safety, return the 0th item.
            return ui->listTemplates->item(0)->data(Qt::UserRole).toString();
        }
        else
        {
            // if we have no templates at all, return an empty string.
            return QString();
        }
    }

    const QString name =item->data(Qt::UserRole).toString();
    return name;
}

void CNewLevelDialog::OnStartup()
{
    UpdateData(false);
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
    QString relativePath = projectLevelsDir.relativeFilePath(absolutePath);

    // Prevent saving to a different drive.
    if (projectLevelsDir.absolutePath()[0] != absolutePath[0])
    {
        return false;
    }

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
    const bool valid = !m_level.isEmpty() && ValidateLevel();

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
