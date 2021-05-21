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

#include "WelcomeScreenDialog.h"

// Qt
#include <QStringListModel>
#include <QToolTip>
#include <QMenu>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QScreen>
#include <QDesktopWidget>
#include <QTimer>

#include <AzCore/Utils/Utils.h>

// AzFramework
#include <AzFramework/API/ApplicationAPI.h>

// AzToolsFramework
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

// AzQtComponents
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>

// Editor
#include "Settings.h"
#include "MainWindow.h"
#include "CryEdit.h"
#include "LevelFileDialog.h"

// NewsShared
#include <NewsShared/ResourceManagement/ResourceManifest.h>     // for News::ResourceManifest
#include <NewsShared/Qt/ArticleViewContainer.h>                 // for News::ArticleViewContainer

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
    , m_pRecentListModel(new QStringListModel(this))
    , m_pRecentList(nullptr)
{
    ui->setupUi(this);

    // Make our welcome screen checkboxes appear as toggle switches
//    AzQtComponents::CheckBox::applyToggleSwitchStyle(ui->autoLoadLevel);
//    AzQtComponents::CheckBox::applyToggleSwitchStyle(ui->showOnStartup);

//    ui->autoLoadLevel->setChecked(gSettings.bAutoloadLastLevelAtStartup);
//    ui->showOnStartup->setChecked(!gSettings.bShowDashboardAtStartup);

    ui->recentLevelList->setModel(m_pRecentListModel);
    ui->recentLevelList->setMouseTracking(true);
    ui->recentLevelList->setContextMenuPolicy(Qt::CustomContextMenu);

//    auto currentProjectButtonMenu = new QMenu();

//    ui->currentProjectButton->setMenu(currentProjectButtonMenu);
    auto projectName = AZ::Utils::GetProjectName();
    ui->currentProjectName->setText(projectName.c_str());

    ui->newLevelButton->setDefault(true);
    ui->gridButton->setCheckable(true);
    ui->gridButton->setChecked(true);
//    ui->currentProjectButton->adjustSize();
//    ui->currentProjectButton->setMinimumWidth(ui->currentProjectButton->width() + 40);

//    ui->documentationLink->setCursor(Qt::PointingHandCursor);
//    ui->documentationLink->installEventFilter(this);

    connect(ui->recentLevelList, &QWidget::customContextMenuRequested, this, &WelcomeScreenDialog::OnShowContextMenu);

    connect(ui->recentLevelList, &QListView::entered, this, &WelcomeScreenDialog::OnShowToolTip);
    connect(ui->recentLevelList, &QListView::clicked, this, &WelcomeScreenDialog::OnRecentLevelListItemClicked);

    connect(ui->newLevelButton, &QPushButton::clicked, this, &WelcomeScreenDialog::OnNewLevelBtnClicked);
    connect(ui->levelFileLabel, &QLabel::linkActivated, this, &WelcomeScreenDialog::OnNewLevelLabelClicked);
    connect(ui->openLevelButton, &QPushButton::clicked, this, &WelcomeScreenDialog::OnOpenLevelBtnClicked);

//    connect(ui->showOnStartup, &QCheckBox::clicked, this, &WelcomeScreenDialog::OnShowOnStartupBtnClicked);
//    connect(ui->autoLoadLevel, &QCheckBox::clicked, this, &WelcomeScreenDialog::OnAutoLoadLevelBtnClicked);

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
                    QString name = Path::GetFileName(fullPath);

                    Path::ConvertSlashToBackSlash(fullPath);
                    fullPath = Path::ToUnixPath(fullPath.toLower());
                    fullPath = Path::AddSlash(fullPath);

                    if (fullPath.contains(gamePath))
                    {
                        m_pRecentListModel->setStringList(m_pRecentListModel->stringList() << QString(name));
                        m_levels.push_back(std::make_pair(name, recentFile));
                    }
                }
            }
        }
    }

    ui->recentLevelList->setCurrentIndex(QModelIndex());
    int rowSize = ui->recentLevelList->sizeHintForRow(0) + ui->recentLevelList->spacing() * 2;
    ui->recentLevelList->setMinimumHeight(m_pRecentListModel->rowCount() * rowSize);
    ui->recentLevelList->setMaximumHeight(m_pRecentListModel->rowCount() * rowSize);
}


void WelcomeScreenDialog::RemoveLevelEntry(int index)
{
    TNamePathPair levelPath = m_levels[index];

    m_pRecentListModel->removeRow(index);
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

    //TEMPORARY:Begin This can be put back once the main window is in Qt
    //QRect itemRect = ui->recentLevelList->visualRect(index);
    QToolTip::showText(QCursor::pos(), QString("Open level: %1").arg(fullPath) /*, ui->recentLevelList, itemRect*/);
    //TEMPORARY:END
}


void WelcomeScreenDialog::OnShowContextMenu(const QPoint& pos)
{
    QModelIndex index = ui->recentLevelList->indexAt(pos);
    if (index.isValid())
    {
        QString level = m_pRecentListModel->data(index, 0).toString();

        QPoint globalPos = ui->recentLevelList->viewport()->mapToGlobal(pos);

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

void WelcomeScreenDialog::OnRecentLevelListItemClicked(const QModelIndex& modelIndex)
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
