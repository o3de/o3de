/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "WelcomeScreenDialog.h"

// Qt
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolTip>
#include <QMenu>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QScreen>
#include <QDesktopWidget>
#include <QTimer>
#include <QDateTime>

#include <AzCore/Utils/Utils.h>

// AzFramework
#include <AzFramework/API/ApplicationAPI.h>

// AzToolsFramework
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

// AzQtComponents
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Utilities/PixmapScaleUtilities.h>

// Editor
#include "Settings.h"
#include "MainWindow.h"
#include "CryEdit.h"
#include "LevelFileDialog.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <WelcomeScreen/ui_WelcomeScreenDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

using namespace AzQtComponents;

#define WMSEVENTNAME "WMSEvent"
#define WMSEVENTOPERATION "operation"

static int GetSmallestScreenHeight()
{
    int smallestHeight = -1;
    for (QScreen* screen : QApplication::screens())
    {
        int screenHeight = screen->availableGeometry().height();
        if ((smallestHeight < 0) || (smallestHeight > screenHeight))
        {
            smallestHeight = screenHeight;
        }
    }

    return smallestHeight;
}

WelcomeScreenDialog::WelcomeScreenDialog(QWidget* pParent)
    : QDialog(new WindowDecorationWrapper(WindowDecorationWrapper::OptionAutoAttach | WindowDecorationWrapper::OptionAutoTitleBarButtons, pParent), Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
    , ui(new Ui::WelcomeScreenDialog)
    , m_pRecentList(nullptr)
{
    ui->setupUi(this);

    // Set the project preview image
    QString projectPreviewPath = QDir(AZ::Utils::GetProjectPath().c_str()).filePath("preview.png");
    QFileInfo projectPreviewPathInfo(projectPreviewPath);
    if (!projectPreviewPathInfo.exists() || !projectPreviewPathInfo.isFile())
    {
        projectPreviewPath = ":/WelcomeScreenDialog/DefaultProjectImage.png";
    }
    
    ui->activeProjectIcon->setPixmap(
        AzQtComponents::ScalePixmapForScreenDpi(
            QPixmap(projectPreviewPath),
            screen(),
            ui->activeProjectIcon->size(),
            Qt::KeepAspectRatioByExpanding,
            Qt::SmoothTransformation
        )
    );

    ui->recentLevelTable->setColumnCount(3);
    ui->recentLevelTable->setMouseTracking(true);
    ui->recentLevelTable->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->recentLevelTable->horizontalHeader()->hide();
    ui->recentLevelTable->verticalHeader()->hide();
    ui->recentLevelTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->recentLevelTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->recentLevelTable->setIconSize(QSize(20, 20));
    installEventFilter(this);

    auto projectName = AZ::Utils::GetProjectName();
    ui->currentProjectName->setText(projectName.c_str());

    ui->newLevelButton->setDefault(true);

    // Hide these buttons until the new functionality is added
    ui->gridButton->hide();
    ui->objectListButton->hide();
    ui->switchProjectButton->hide();

    connect(ui->recentLevelTable, &QWidget::customContextMenuRequested, this, &WelcomeScreenDialog::OnShowContextMenu);

    connect(ui->recentLevelTable, &QTableWidget::entered, this, &WelcomeScreenDialog::OnShowToolTip);
    connect(ui->recentLevelTable, &QTableWidget::clicked, this, &WelcomeScreenDialog::OnRecentLevelTableItemClicked);

    connect(ui->newLevelButton, &QPushButton::clicked, this, &WelcomeScreenDialog::OnNewLevelBtnClicked);
    connect(ui->levelFileLabel, &QLabel::linkActivated, this, &WelcomeScreenDialog::OnNewLevelLabelClicked);
    connect(ui->openLevelButton, &QPushButton::clicked, this, &WelcomeScreenDialog::OnOpenLevelBtnClicked);

    // Adjust the height, if need be
    // Do it in the constructor so that the WindowDecoratorWrapper handles it correctly
    int smallestHeight = GetSmallestScreenHeight();
    if (smallestHeight < geometry().height())
    {
        const int SOME_PADDING_IN_PIXELS = 90;
        int difference = geometry().height() - (smallestHeight - SOME_PADDING_IN_PIXELS);

        QRect newGeometry = geometry().adjusted(0, difference / 2, 0, -difference / 2);
        setMinimumSize(minimumSize().width(), newGeometry.height());
        resize(newGeometry.size());
    }

    m_levelExtension = EditorUtils::LevelFile::GetDefaultFileExtension();
}


WelcomeScreenDialog::~WelcomeScreenDialog()
{
    delete ui;
}

void WelcomeScreenDialog::done(int result)
{
    QDialog::done(result);
}

const QString& WelcomeScreenDialog::GetLevelPath()
{
    return m_levelPath;
}

bool WelcomeScreenDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Show)
    {
        ui->recentLevelTable->horizontalHeader()->resizeSection(0, ui->nameLabel->width());
        ui->recentLevelTable->horizontalHeader()->resizeSection(1, ui->modifiedLabel->width());
        ui->recentLevelTable->horizontalHeader()->resizeSection(2, ui->typeLabel->width());
    }

    return QDialog::eventFilter(watched, event);
}

void WelcomeScreenDialog::SetRecentFileList(RecentFileList* pList)
{
    if (!pList)
    {
        return;
    }

    m_pRecentList = pList;

    const char* engineRoot;
    EBUS_EVENT_RESULT(engineRoot, AzFramework::ApplicationRequests::Bus, GetEngineRoot);

    auto projectPath = AZ::Utils::GetProjectPath();
    QString gamePath{projectPath.c_str()};
    Path::ConvertSlashToBackSlash(gamePath);
    gamePath = Path::ToUnixPath(gamePath.toLower());
    gamePath = Path::AddSlash(gamePath);

    QString sCurDir = (Path::GetEditingGameDataFolder() + QDir::separator().toLatin1()).c_str();
    int nCurDir = sCurDir.length();

    int recentListSize = pList->GetSize();
    int currentRow = 0;
    ui->recentLevelTable->setRowCount(recentListSize);
     for (int i = 0; i < recentListSize; ++i)
    {
        const QString& recentFile = pList->m_arrNames[i];
        if (recentFile.endsWith(m_levelExtension))
        {
            if (CFileUtil::Exists(recentFile, false))
            {
                QString sCurEntryDir = recentFile.left(nCurDir);
                if (sCurEntryDir.compare(sCurDir, Qt::CaseInsensitive) == 0)
                {
                    QString fullPath = recentFile;
                    const QString name = Path::GetFile(fullPath);

                    Path::ConvertSlashToBackSlash(fullPath);
                    fullPath = Path::ToUnixPath(fullPath.toLower());
                    fullPath = Path::AddSlash(fullPath);

                    if (fullPath.contains(gamePath))
                    {
                        if (gSettings.prefabSystem)
                        {
                            QIcon icon;
                            icon.addFile(QString::fromUtf8(":/Level/level.svg"), QSize(), QIcon::Normal, QIcon::Off);
                            ui->recentLevelTable->setItem(currentRow, 0, new QTableWidgetItem(icon, name));
                        }
                        else
                        {
                            ui->recentLevelTable->setItem(currentRow, 0, new QTableWidgetItem(name));
                        }
                        QFileInfo file(recentFile);
                        QDateTime dateTime = file.lastModified();
                        QString date = QLocale::system().toString(dateTime.date(), QLocale::ShortFormat) + " " +
                            QLocale::system().toString(dateTime.time(), QLocale::LongFormat);
                        ui->recentLevelTable->setItem(currentRow, 1, new QTableWidgetItem(date));
                        ui->recentLevelTable->setItem(currentRow++, 2, new QTableWidgetItem(tr("Level")));
                        m_levels.push_back(std::make_pair(name, recentFile));
                    }
                }
            }
        }
    }
    ui->recentLevelTable->setRowCount(currentRow);
    ui->recentLevelTable->setMinimumHeight(currentRow * ui->recentLevelTable->verticalHeader()->defaultSectionSize());
    ui->recentLevelTable->setMaximumHeight(currentRow * ui->recentLevelTable->verticalHeader()->defaultSectionSize());
    ui->levelFileLabel->setVisible(currentRow ? false : true);

    ui->recentLevelTable->setCurrentIndex(QModelIndex());
}


void WelcomeScreenDialog::RemoveLevelEntry(int index)
{
    TNamePathPair levelPath = m_levels[index];

    ui->recentLevelTable->removeRow(index);
    m_levels.erase(m_levels.begin() + index);


    if (!m_pRecentList)
    {
        return;
    }

    for (int i = 0; i < m_pRecentList->GetSize(); ++i)
    {
        QString fullPath = m_pRecentList->m_arrNames[i];
        QString fullPath2 = levelPath.second;

        // path from recent list
        Path::ConvertSlashToBackSlash(fullPath);
        fullPath = Path::ToUnixPath(fullPath.toLower());
        fullPath = Path::AddPathSlash(fullPath);

        // path from our dashboard list
        Path::ConvertSlashToBackSlash(fullPath2);
        fullPath2 = Path::ToUnixPath(fullPath2.toLower());
        fullPath2 = Path::AddPathSlash(fullPath2);

        if (fullPath == fullPath2)
        {
            m_pRecentList->Remove(index);
            break;
        }
    }

    m_pRecentList->WriteList();
}


void WelcomeScreenDialog::OnShowToolTip(const QModelIndex& index)
{
    const QString& fullPath = m_levels[index.row()].second;

    QToolTip::showText(QCursor::pos(), QString("Open level: %1").arg(fullPath));
}


void WelcomeScreenDialog::OnShowContextMenu(const QPoint& pos)
{
    QModelIndex index = ui->recentLevelTable->indexAt(pos);
    if (index.isValid())
    {
        QString level = ui->recentLevelTable->itemAt(pos)->text();

        QPoint globalPos = ui->recentLevelTable->viewport()->mapToGlobal(pos);

        QMenu contextMenu;
        contextMenu.addAction(QString("Remove " + level + " from recent list"));
        QAction* selectedItem = contextMenu.exec(globalPos);
        if (selectedItem)
        {
            RemoveLevelEntry(index.row());
        }
    }
}

void WelcomeScreenDialog::OnNewLevelBtnClicked([[maybe_unused]] bool checked)
{
    m_levelPath = "new";
    accept();
}

void WelcomeScreenDialog::OnNewLevelLabelClicked([[maybe_unused]] const QString& path)
{
    OnNewLevelBtnClicked(true);
}

void WelcomeScreenDialog::OnOpenLevelBtnClicked([[maybe_unused]] bool checked)
{
    CLevelFileDialog dlg(true, this);

    if (dlg.exec() == QDialog::Accepted)
    {
        m_levelPath = dlg.GetFileName();
        accept();
    }
}

void WelcomeScreenDialog::OnRecentLevelTableItemClicked(const QModelIndex& modelIndex)
{
    int index = modelIndex.row();

    if (index >= 0 && index < m_levels.size())
    {
        m_levelPath = m_levels[index].second;
        accept();
    }
}

void WelcomeScreenDialog::OnCloseBtnClicked([[maybe_unused]] bool checked)
{
    accept();
}

void WelcomeScreenDialog::previewAreaScrolled()
{
    //this should only be reported once per session
    if (m_messageScrollReported)
    {
        return;
    }
    m_messageScrollReported = true;
}

#include <WelcomeScreen/moc_WelcomeScreenDialog.cpp>
