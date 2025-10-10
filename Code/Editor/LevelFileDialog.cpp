/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "LevelFileDialog.h"

#include <AzFramework/API/ApplicationAPI.h>

// Qt
#include <QComboBox>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSignalBlocker>

// Editor
#include "LevelTreeModel.h"
#include "LevelRoots.h"
#include "CryEditDoc.h"
#include "API/ToolsApplicationAPI.h"


#include <ui_LevelFileDialog.h>

static const char lastLoadPathFilename[] = "lastLoadPath.preset";

// Folder in which levels are stored
static const char kLevelsFolder[] = "Levels";

CLevelFileDialog::CLevelFileDialog(bool openDialog, QWidget* parent)
    : QDialog(parent)
    , m_bOpenDialog(openDialog)
    , ui(new Ui::LevelFileDialog())
    , m_model(new LevelTreeModel(this))
    , m_filterModel(new LevelTreeModelFilter(this))
{
    ui->setupUi(this);
    ui->treeView->header()->close();
    m_filterModel->setSourceModel(m_model);
    ui->treeView->setModel(m_filterModel);
    ui->treeView->installEventFilter(this);

    connect(ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
        this, &CLevelFileDialog::OnTreeSelectionChanged);

    connect(ui->treeView, &QTreeView::doubleClicked, this, [this]()
        {
            if (m_bOpenDialog && !IsValidLevelSelected())
            {
                return;
            }

            OnOK();
        });

    connect(ui->filterLineEdit, &QLineEdit::textChanged, this, &CLevelFileDialog::OnFilterChanged);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &CLevelFileDialog::OnCancel);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CLevelFileDialog::OnOK);
    connect(ui->newFolderButton, &QPushButton::clicked, this, &CLevelFileDialog::OnNewFolder);

    if (m_bOpenDialog)
    {
        setWindowTitle(tr("Open Level"));
        ui->treeView->expandToDepth(1);
        ui->newFolderButton->setVisible(false);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Open"));

        // Open keeps the legacy multi-root tree, so the target combo is
        // hidden entirely.
        ui->targetRootLabel->setVisible(false);
        ui->targetRootCombo->setVisible(false);
    }
    else
    {
        setWindowTitle(tr("Save Level As "));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Save"));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

        // Make the name input the default active field for the save as dialog
        // The filter input will still be the default active field for the open dialog
        setTabOrder(ui->nameLineEdit, ui->filterLineEdit);

        connect(ui->nameLineEdit, &QLineEdit::textChanged, this, &CLevelFileDialog::OnNameChanged);

        // Save As exposes a target combo so the user explicitly picks the
        // owning root before naming the level. The tree below is then
        // filtered to that one target, which keeps "save into an empty
        // gem" unambiguous.
        m_saveTargetRoots = LevelRoots::Enumerate(LevelRoots::Mode::AllActive);
        QSignalBlocker blocker(ui->targetRootCombo);
        ui->targetRootCombo->clear();
        for (const LevelRoots::Root& root : m_saveTargetRoots)
        {
            ui->targetRootCombo->addItem(root.displayName, root.absolutePath);
        }
        ui->targetRootCombo->setCurrentIndex(0);
        connect(
            ui->targetRootCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &CLevelFileDialog::OnTargetRootChanged);
    }

    // reject invalid file names
    ui->nameLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[a-zA-Z0-9_\\-./]*$"), ui->nameLineEdit));

    // Default to the project root until the user selects something. Enumerate()
    // is guaranteed to return the project root as the first entry.
    const QVector<LevelRoots::Root> roots = LevelRoots::Enumerate();
    if (!roots.isEmpty())
    {
        m_selectedRoot = roots.front().absolutePath;
        m_selectedRootIsProject = roots.front().isProject;
    }

    ReloadTree();
    // Open restores the last-used level so the user lands on what they
    // were working on. Save As intentionally does not - the user expects
    // a clean slate (project root, empty name) so they don't accidentally
    // overwrite the previous Save As target by reflex.
    if (m_bOpenDialog)
    {
        LoadLastUsedLevelPath();
    }
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

CLevelFileDialog::~CLevelFileDialog()
{
}

QString CLevelFileDialog::GetFileName() const
{
    return m_fileName;
}

void CLevelFileDialog::OnCancel()
{
    close();
}

void CLevelFileDialog::OnOK()
{
    QString errorMessage;
    if (!ValidateSaveLevelPath(errorMessage))
    {
        QMessageBox::warning(this, tr("Error"), errorMessage);
        return;
    }

    if (m_bOpenDialog)
    {
        // For Open button
        if (!IsValidLevelSelected())
        {
            QMessageBox box(this);
            box.setText(tr("Please enter a valid level name"));
            box.setIcon(QMessageBox::Critical);
            box.exec();
            return;
        }
    }
    else
    {
        QString levelPath = GetLevelPath();
        if (CFileUtil::PathExists(levelPath) && CheckLevelFolder(levelPath))
        {
            // there is already a level folder at that location, ask before for overwriting it
            QMessageBox box(this);
            box.setText(tr("Do you really want to overwrite '%1'?").arg(GetEnteredPath()));
            box.setIcon(QMessageBox::Warning);
            box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            if (box.exec() != QMessageBox::Yes)
            {
                return;
            }
        }

        m_fileName = levelPath + "/" + Path::GetFileName(levelPath) + EditorUtils::LevelFile::GetDefaultFileExtension();
    }

    SaveLastUsedLevelPath();
    accept();
}

bool CLevelFileDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            OnOK();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

QString CLevelFileDialog::NameForIndex(const QModelIndex& index) const
{
    QStringList tokens;
    QModelIndex idx = index;

    while (idx.isValid() && idx.parent().isValid())   // the root one doesn't count
    {
        tokens.push_front(idx.data(Qt::DisplayRole).toString());
        idx = idx.parent();
    }

    QString text = tokens.join('/');
    const bool isLevelFolder = index.data(LevelTreeModel::IsLevelFolderRole).toBool();
    if (!isLevelFolder && !text.isEmpty())
    {
        text += "/";
    }

    return text;
}

bool CLevelFileDialog::IsValidLevelSelected()
{
    QString levelPath = GetLevelPath();
    m_fileName = GetFileName(levelPath);

    QString currentExtension = "." + Path::GetExt(m_fileName);

    const char* oldExtension = EditorUtils::LevelFile::GetOldCryFileExtension();
    const char* defaultExtension = EditorUtils::LevelFile::GetDefaultFileExtension();

    bool isInvalidFileExtension = (currentExtension != defaultExtension && currentExtension != oldExtension);

    if (!isInvalidFileExtension && CFileUtil::FileExists(m_fileName))
    {
        return true;
    }
    else
    {
        return false;
    }
}

QString CLevelFileDialog::GetLevelPath() const
{
    const QString enteredPath = GetEnteredPath();

    // m_selectedRoot already points to the absolute "Levels" folder of the
    // currently-targeted root (project by default, or whichever gem the user
    // navigated into in the tree).
    const QString rootPath = !m_selectedRoot.isEmpty()
        ? m_selectedRoot
        : QString("%1/%2").arg(Path::GetEditingGameDataFolder().c_str()).arg(kLevelsFolder);

    return QStringLiteral("%1/%2").arg(rootPath, enteredPath);
}

QString CLevelFileDialog::GetEnteredPath() const
{
    QString enteredPath = ui->nameLineEdit->text();
    enteredPath = enteredPath.trimmed();
    enteredPath = Path::RemoveBackslash(enteredPath);

    return enteredPath;
}

QString CLevelFileDialog::GetFileName(QString levelPath)
{
    QStringList levelFiles;
    QString fileName;

    if (CheckLevelFolder(levelPath, &levelFiles) && levelFiles.size() >= 1)
    {
        const char* oldExtension = EditorUtils::LevelFile::GetOldCryFileExtension();
        const char* defaultExtension = EditorUtils::LevelFile::GetDefaultFileExtension();

        // A level folder was entered. Prefer the .ly/.cry file with the
        // folder name, otherwise pick the first one in the list
        QString path = Path::GetFileName(levelPath);
        QString needle = path + defaultExtension;
        auto iter = std::find(levelFiles.begin(), levelFiles.end(), needle);

        if (iter != levelFiles.end())
        {
            fileName = levelPath + "/" + *iter;
        }
        else
        {
            needle = path + oldExtension;
            iter = std::find(levelFiles.begin(), levelFiles.end(), needle);
            if (iter != levelFiles.end())
            {
                fileName = levelPath + "/" + *iter;
            }
            else
            {
                fileName = levelPath + "/" + levelFiles[0];
            }
        }
    }
    else
    {
        // Otherwise try to directly load the specified file (backward compatibility)
        fileName = levelPath;
    }

    return fileName;
}

void CLevelFileDialog::OnTreeSelectionChanged()
{
    const QModelIndexList indexes = ui->treeView->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty())
    {
        const QModelIndex selected = indexes.first();

        // Update the active root before regenerating the displayed name, so
        // GetLevelPath() and validation reflect the root the user clicked
        // into (project vs. one of the gem roots).
        const QString rootPath = selected.data(LevelTreeModel::RootPathRole).toString();
        if (!rootPath.isEmpty())
        {
            m_selectedRoot = rootPath;
            m_selectedRootIsProject = selected.data(LevelTreeModel::IsProjectRootRole).toBool();
        }

        ui->nameLineEdit->setText(NameForIndex(selected));
    }
}

void CLevelFileDialog::OnNewFolder()
{
    const QModelIndexList indexes = ui->treeView->selectionModel()->selectedIndexes();

    if (indexes.isEmpty())
    {
        QMessageBox box(this);
        box.setText(tr("Please select a folder first"));
        box.setIcon(QMessageBox::Critical);
        box.exec();
        return;
    }

    const QModelIndex index = indexes.first();
    const bool isLevelFolder = index.data(LevelTreeModel::IsLevelFolderRole).toBool();

    // Creating folders is not allowed in level folders
    if (!isLevelFolder && index.isValid())
    {
        const QString parentFullPath = index.data(LevelTreeModel::FullPathRole).toString();
        QInputDialog inputDlg(this);
        inputDlg.setLabelText(tr("Please select a folder name"));

        if (inputDlg.exec() == QDialog::Accepted && !inputDlg.textValue().isEmpty())
        {
            const QString newFolderName = inputDlg.textValue();
            const QString newFolderPath = parentFullPath + "/" + newFolderName;

            if (!AZ::StringFunc::Path::IsValid(newFolderName.toUtf8().data()))
            {
                QMessageBox box(this);
                box.setText(tr("Please enter a single, valid folder name(standard English alphanumeric characters only)"));
                box.setIcon(QMessageBox::Critical);
                box.exec();
                return;
            }

            if (CFileUtil::PathExists(newFolderPath))
            {
                QMessageBox box(this);
                box.setText(tr("Folder already exists"));
                box.setIcon(QMessageBox::Critical);
                box.exec();
                return;
            }

            // The trailing / is important, otherwise CreatePath doesn't work
            if (!CFileUtil::CreatePath(newFolderPath + "/"))
            {
                QMessageBox box(this);
                box.setText(tr("Could not create folder"));
                box.setIcon(QMessageBox::Critical);
                box.exec();
                return;
            }

            m_model->AddItem(newFolderName, m_filterModel->mapToSource(index));
            ui->treeView->expand(index);
        }
    }
    else
    {
        QMessageBox box(this);
        box.setText(tr("Please select a folder first"));
        box.setIcon(QMessageBox::Critical);
        box.exec();
        return;
    }
}

void CLevelFileDialog::OnFilterChanged()
{
    m_filterModel->setFilterText(ui->filterLineEdit->text().toLower());
}

void CLevelFileDialog::OnNameChanged()
{
    if (!m_bOpenDialog)
    {
        QString errorMessage;
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ValidateSaveLevelPath(errorMessage));
    }
}

void CLevelFileDialog::ReloadTree()
{
    if (m_bOpenDialog)
    {
        // Open keeps a tree with every gem that already has an
        // Assets/Levels folder so the user can browse all of them at once.
        m_model->ReloadTree(m_bOpenDialog, LevelRoots::Mode::ExistingOnly);
        return;
    }

    // Save As shows only the contents of the currently-selected target
    // root. Defaults to the first entry (the project) until the user
    // picks something else.
    if (m_saveTargetRoots.isEmpty())
    {
        m_model->ReloadTree(m_bOpenDialog, LevelRoots::Mode::AllActive);
        return;
    }

    const int index = ui->targetRootCombo->currentIndex();
    const int safeIndex = (index >= 0 && index < m_saveTargetRoots.size()) ? index : 0;
    const LevelRoots::Root& root = m_saveTargetRoots[safeIndex];
    m_model->ReloadTreeFromRoot(m_bOpenDialog, root);
    m_selectedRoot = root.absolutePath;
    m_selectedRootIsProject = root.isProject;

    // Auto-expand the single root so the user immediately sees its
    // contents (or its emptiness, for a fresh gem).
    const QModelIndex rootIndex = m_filterModel->index(0, 0);
    if (rootIndex.isValid())
    {
        ui->treeView->expand(rootIndex);
    }
}

void CLevelFileDialog::OnTargetRootChanged(int /*index*/)
{
    ReloadTree();
}

//////////////////////////////////////////////////////////////////////////
// Heuristic to detect a level folder, also returns all .cry/.ly files in it
//////////////////////////////////////////////////////////////////////////
bool CLevelFileDialog::CheckLevelFolder(const QString folder, QStringList* levelFiles)
{
    CFileEnum fileEnum;
    QFileInfo fileData;
    bool bIsLevelFolder = false;

    for (bool bFoundFile = fileEnum.StartEnumeration(folder, "*", &fileData);
         bFoundFile; bFoundFile = fileEnum.GetNextFile(&fileData))
    {
        const QString fileName = fileData.fileName();

        if (!fileData.isDir())
        {
            QString ext = "." + Path::GetExt(fileName);

            const char* defaultExtension = EditorUtils::LevelFile::GetDefaultFileExtension();

            if (ext == defaultExtension)
            {
                bIsLevelFolder = true;

                if (levelFiles)
                {
                    levelFiles->push_back(fileName);
                }
            }
        }
    }

    return bIsLevelFolder;
}

bool CLevelFileDialog::ValidateSaveLevelPath(QString& errorMessage) const
{
    const QString enteredPath = GetEnteredPath();
    const QString levelPath = GetLevelPath();

    if (!AZ::StringFunc::Path::IsValid(Path::GetFileName(levelPath).toUtf8().data()))
    {
        errorMessage = tr("Please enter a valid level name (standard English alphanumeric characters only)");
        return false;
    }

    //Verify that we are not using the temporary level name
    const char* temporaryLevelName = GetIEditor()->GetDocument()->GetTemporaryLevelName();
    if (QString::compare(Path::GetFileName(levelPath), temporaryLevelName) == 0)
    {
        errorMessage = tr("Please enter a level name that is different from the temporary name");
        return false;
    }

    if (!ValidateLevelPath(enteredPath))
    {
        errorMessage = tr("Please enter a valid level location.\nYou cannot save levels inside levels.");
        return false;
    }

    if (CFileUtil::FileExists(levelPath))
    {
        errorMessage = tr("A file with that name already exists");
        return false;
    }

    if (CFileUtil::PathExists(levelPath) && !CheckLevelFolder(levelPath))
    {
        errorMessage = tr("Please enter a level name");
        return false;
    }

    if (!ui->nameLineEdit->hasAcceptableInput())
    {
        QString message = tr("The level name %1 contains illegal characters.");
        errorMessage = message.arg(enteredPath);
        return false;
    }

    return true;
}


//////////////////////////////////////////////////////////////////////////
// Checks if a given path is a valid level path
//////////////////////////////////////////////////////////////////////////
bool CLevelFileDialog::ValidateLevelPath(const QString& levelPath) const
{
    if (levelPath.isEmpty() || Path::GetExt(levelPath) != "")
    {
        return false;
    }

    // Split path
    QStringList splittedPath = levelPath.split(QRegularExpression(QStringLiteral(R"([\\/])")), Qt::SkipEmptyParts);

    // This shouldn't happen, but be careful
    if (splittedPath.empty())
    {
        return false;
    }

    // Make sure that no folder before the last in the name contains a level.
    // Walk from the active root (project or gem) so the check matches the
    // location the user is actually saving into.
    if (splittedPath.size() > 1)
    {
        QString currentPath = !m_selectedRoot.isEmpty()
            ? m_selectedRoot
            : QString::fromUtf8((Path::GetEditingGameDataFolder() + "/" + kLevelsFolder).c_str());

        for (size_t i = 0; i < splittedPath.size() - 1; ++i)
        {
            currentPath += "/" + splittedPath[static_cast<int>(i)];

            if (CFileUtil::FileExists(currentPath) || CheckLevelFolder(currentPath))
            {
                return false;
            }
        }
    }

    return true;
}

void CLevelFileDialog::SaveLastUsedLevelPath()
{
    const QString settingPath = QString(Path::GetUserSandboxFolder()) + lastLoadPathFilename;

    XmlNodeRef lastUsedLevelPathNode = XmlHelpers::CreateXmlNode("lastusedlevelpath");
    lastUsedLevelPathNode->setAttr("path", ui->nameLineEdit->text().toUtf8().data());
    // Persist which root the relative "path" is anchored to, so the next
    // session can re-open a level that lives under a gem rather than the
    // project. Older preset files that omit this attribute are still
    // interpreted as project-relative (legacy behaviour).
    lastUsedLevelPathNode->setAttr("root", m_selectedRoot.toUtf8().data());
    lastUsedLevelPathNode->saveToFile(settingPath.toUtf8().data());
}

void CLevelFileDialog::LoadLastUsedLevelPath()
{
    const QString settingPath = Path::GetUserSandboxFolder() + QString(lastLoadPathFilename);

    XmlNodeRef lastUsedLevelPathNode = XmlHelpers::LoadXmlFromFile(settingPath.toUtf8().data());
    if (lastUsedLevelPathNode == nullptr)
    {
        return;
    }

    QString lastLoadedFileName;
    lastUsedLevelPathNode->getAttr("path", lastLoadedFileName);

    QString lastUsedRoot;
    lastUsedLevelPathNode->getAttr("root", lastUsedRoot);

    if (m_filterModel->rowCount() < 1)
    {
        // Defensive, doesn't happen
        return;
    }

    // In Save As mode the tree is filtered to one root, so move the combo
    // to the persisted root first; that triggers ReloadTree() and rebuilds
    // the tree under that root before we walk it for the level entry.
    if (!m_bOpenDialog && !lastUsedRoot.isEmpty() && !m_saveTargetRoots.isEmpty())
    {
        for (int i = 0; i < m_saveTargetRoots.size(); ++i)
        {
            if (m_saveTargetRoots[i].absolutePath.compare(lastUsedRoot, Qt::CaseInsensitive) == 0)
            {
                ui->targetRootCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    // Locate the top-level node that matches the persisted root. Falls back
    // to the first root (the project) for older preset files that did not
    // record one.
    QModelIndex currentIndex = m_filterModel->index(0, 0);
    if (!lastUsedRoot.isEmpty())
    {
        for (int i = 0, n = m_filterModel->rowCount(); i < n; ++i)
        {
            const QModelIndex candidate = m_filterModel->index(i, 0);
            if (candidate.data(LevelTreeModel::RootPathRole).toString().compare(lastUsedRoot, Qt::CaseInsensitive) == 0)
            {
                currentIndex = candidate;
                m_selectedRoot = lastUsedRoot;
                m_selectedRootIsProject = candidate.data(LevelTreeModel::IsProjectRootRole).toBool();
                break;
            }
        }
    }

    QStringList segments = Path::SplitIntoSegments(lastLoadedFileName);
    for (auto it = segments.cbegin(), end = segments.cend(); it != end; ++it)
    {
        const int numChildren = m_filterModel->rowCount(currentIndex);
        for (int i = 0; i < numChildren; ++i)
        {
            const QModelIndex subIndex = m_filterModel->index(i, 0, currentIndex);
            if (*it == subIndex.data(Qt::DisplayRole).toString())
            {
                ui->treeView->expand(currentIndex);
                currentIndex = subIndex;
                break;
            }
        }
    }

    if (currentIndex.isValid())
    {
        ui->treeView->selectionModel()->select(currentIndex, QItemSelectionModel::Select);
    }

    ui->nameLineEdit->setText(lastLoadedFileName);
}

