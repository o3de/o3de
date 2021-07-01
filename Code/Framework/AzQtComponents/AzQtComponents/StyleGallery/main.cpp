/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "mainwidget.h"
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/ToolButtonComboBox.h>
#include <AzQtComponents/Components/ToolButtonLineEdit.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Components/O3DEStylesheet.h>
#include "MyCombo.h"

#include <QPushButton>
#include <QDebug>
#include <QApplication>
#include <QStyleFactory>
#include <QMainWindow>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QToolButton>
#include <QVBoxLayout>
#include <QSettings>

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <AzQtComponents/Components/StyledDetailsTableView.h>
#include <AzQtComponents/Components/StyledDetailsTableModel.h>

using namespace AzQtComponents;

QToolBar * toolBar()
{
    auto t = new QToolBar(QString());
    QAction *a = nullptr;
    QMenu *menu = nullptr;
    auto but = new QToolButton();
    menu = new QMenu();
    but->setMenu(menu);
    but->setCheckable(true);
    but->setPopupMode(QToolButton::MenuButtonPopup);
    but->setDefaultAction(new QAction(QStringLiteral("Test"), but));
    but->setIcon(QIcon(":/stylesheet/img/undo.png"));
    t->addWidget(but);

    a = t->addAction(QIcon(":/stylesheet/img/select.png"), QString());
    a->setCheckable(true);
    t->addSeparator();

    auto tbcb = new AzQtComponents::ToolButtonComboBox();
    tbcb->setIcon(QIcon(":/stylesheet/img/angle.png"));
    tbcb->comboBox()->addItem("1");
    tbcb->comboBox()->addItem("2");
    tbcb->comboBox()->addItem("3");
    tbcb->comboBox()->addItem("4");
    t->addWidget(tbcb);

    auto tble = new AzQtComponents::ToolButtonLineEdit();
    tble->setFixedWidth(130);
    tble->setIcon(QIcon(":/stylesheet/img/16x16/Search.png"));
    t->addWidget(tble);

    auto combo = new QComboBox();
    combo->addItem("Test1");
    combo->addItem("Test3");
    combo->addItem("Selection,Area");
    t->addWidget(combo);

    const QStringList iconNames = {
        "Align_object_to_surface",
        "Align_to_grid",
        "Align_to_Object",
        "Angle",
        "Asset_Browser",
        "Audio",
        "Database_view",
        "Delete_named_selection",
        "Follow_terrain",
        "Freeze",
        "Gepetto",
        "Get_physics_state",
        "Grid",
        "layer_editor",
        "layers",
        "LIghting",
        "lineedit-clear",
        "LOD_generator",
        "LUA",
        "Material",
        "Maximize",
        "Measure",
        "Move",
        "Object_follow_terrain",
        "Object_list",
        "Redo",
        "Reset_physics_state",
        "Scale",
        "select_object",
        "Select",
        "Select_terrain",
        "Simulate_Physics_on_selected_objects",
        "Substance",
        "Terrain",
        "Terrain_Texture",
        "Texture_browser",
        "Time_of_Day",
        "Trackview",
        "Translate",
        "UI_editor",
        "undo",
        "Unfreeze_all",
        "Vertex_snapping" };

    for (auto name : iconNames) {
        auto a2 = t->addAction(QIcon(QString(":/stylesheet/img/16x16/%1.png").arg(name)), nullptr);
        a2->setToolTip(name);
        a2->setDisabled(name == "Audio");
    }

    combo = new MyComboBox();
    combo->addItem("Test1");
    combo->addItem("Test3");
    combo->addItem("Selection,Area");
    t->addWidget(combo);

    return t;
}

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
        , m_settings("org", "app")
    {
        setAttribute(Qt::WA_DeleteOnClose);
        QVariant geoV = m_settings.value("geo");
        if (geoV.isValid()) {
            restoreGeometry(geoV.toByteArray());
        }
    }

    ~MainWindow()
    {
        qDebug() << "Deleted mainwindow";
        auto geo = saveGeometry();
        m_settings.setValue("geo", geo);
    }
private:
    QSettings m_settings;
};

static void addMenu(QMainWindow *w, const QString &name)
{
    auto action = w->menuBar()->addAction(name);
    auto menu = new QMenu();
    action->setMenu(menu);
    menu->addAction("Item 1");
    menu->addAction("Item 2");
    menu->addAction("Item 3");
    menu->addAction("Longer item 4");
    menu->addAction("Longer item 5");
}

int main(int argc, char **argv)
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    AzQtComponents::O3DEStylesheet stylesheet(&app);
    AZ::IO::FixedMaxPath engineRootPath;
    {
        AZ::ComponentApplication componentApplication(argc, argv);
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
    }
    stylesheet.initialize(&app, engineRootPath);

    auto wrapper = new AzQtComponents::WindowDecorationWrapper();
    QMainWindow *w = new MainWindow();
    w->setWindowTitle("Component Gallery");
    auto widget = new MainWidget(w);
    w->resize(550, 900);
    w->setMinimumWidth(500);
    wrapper->setGuest(w);
    wrapper->enableSaveRestoreGeometry("app", "org", "windowGeometry");
    w->setCentralWidget(widget);
    w->setWindowFlags(w->windowFlags() | Qt::WindowStaysOnTopHint);
    w->show();

    auto action = w->menuBar()->addAction("Menu 1");

    auto fileMenu = new QMenu();
    action->setMenu(fileMenu);
    auto openDock = fileMenu->addAction("Open dockwidget");
    QObject::connect(openDock, &QAction::triggered, w, [&w] {
        auto dock = new AzQtComponents::StyledDockWidget(QLatin1String("O3DE"), w);
        auto button = new QPushButton("Click to dock");
        auto wid = new QWidget();
        auto widLayout = new QVBoxLayout(wid);
        widLayout->addWidget(button);
        wid->resize(300, 200);
        QObject::connect(button, &QPushButton::clicked, dock, [dock]{
            dock->setFloating(!dock->isFloating());
        });
        w->addDockWidget(Qt::BottomDockWidgetArea, dock);
        dock->setWidget(wid);
        dock->resize(300, 200);
        dock->setFloating(true);
        dock->show();
    });


    QAction* newAction = fileMenu->addAction("Test StyledDetailsTableView");
    newAction->setShortcut(QKeySequence::Delete);
    QObject::connect(newAction, &QAction::triggered, w, [w]() {
        QDialog temp(w);
        temp.setWindowTitle("StyleTableWidget Test");

        QVBoxLayout* layout = new QVBoxLayout(&temp);

        AzQtComponents::StyledDetailsTableModel* tableModel = new AzQtComponents::StyledDetailsTableModel();
        tableModel->AddColumn("Status", AzQtComponents::StyledDetailsTableModel::StatusIcon);
        tableModel->AddColumn("Platform");
        tableModel->AddColumn("Message");
        tableModel->AddColumnAlias("message", "Message");

        tableModel->AddPrioritizedKey("Data3");
        tableModel->AddDeprioritizedKey("Data1");

        auto table = new AzQtComponents::StyledDetailsTableView();
        table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        table->setModel(tableModel);

        {
            AzQtComponents::StyledDetailsTableModel::TableEntry entry;
            entry.Add("Message", "A very very long first message. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus et maximus tortor, ac commodo ante. Maecenas porta posuere mauris, vel consectetur arcu ornare interdum. Praesent rhoncus consequat neque, non volutpat mauris cursus a. Proin a nisl quis dui consectetur malesuada a et diam. Integer finibus luctus nibh nec cursus.");
            entry.Add("Platform", "PC");
            entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusSuccess);
            tableModel->AddEntry(entry);
        }

        {
            AzQtComponents::StyledDetailsTableModel::TableEntry entry;
            entry.Add("Message", "Second message. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus et maximus tortor, ac commodo ante. Maecenas porta posuere mauris, vel consectetur arcu ornare interdum. Praesent rhoncus consequat neque, non volutpat mauris cursus a. Proin a nisl quis dui consectetur malesuada a et diam. Integer finibus luctus nibh nec cursus.");
            entry.Add("Platform", "PC");
            entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusError);
            entry.Add("Data1", "Deprioritized item.");
            entry.Add("Data3", "Prioritized item.");
            tableModel->AddEntry(entry);
        }

        for (int i = 0; i < 4; i++)
        {
            AzQtComponents::StyledDetailsTableModel::TableEntry entry;
            entry.Add("message", "Third message");
            entry.Add("Status", AzQtComponents::StyledDetailsTableModel::StatusWarning);
            entry.Add("Platform", "PC");
            entry.Add("Index1", "A smaller detail.");
            entry.Add("Index2", "Another small detail.");
            entry.Add("Index3", "A final small detail.");
            tableModel->AddEntry(entry);
        }

        layout->addWidget(table);

        temp.exec();
    });

    QAction* refreshAction = fileMenu->addAction("Refresh Stylesheet");
    QObject::connect(refreshAction, &QAction::triggered, refreshAction, [&stylesheet]() {
        stylesheet.Refresh();
    });


    addMenu(w, "Menu 2");
    addMenu(w, "Menu 3");
    addMenu(w, "Menu 4");

    w->addToolBar(toolBar());

    QObject::connect(widget, &MainWidget::reloadCSS, widget, [] {
        qDebug() << "Reloading CSS";
        qApp->setStyleSheet(QString());
        qApp->setStyleSheet("file:///style.qss");
    });

    return app.exec();
}
