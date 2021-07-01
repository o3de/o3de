/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "mainwidget.h"
#include <AzQtComponents/StyleGallery/ui_mainwidget.h>
#include <AzQtComponents/Components/ButtonStripe.h>
#include <AzQtComponents/Components/DockBarButton.h>

#include <QButtonGroup>
#include <QMenu>
#include <QMouseEvent>
#include <QStringList>
#include <QStandardItemModel>
#include <QMainWindow>
#include <QMenuBar>
#include <QDebug>

using namespace AzQtComponents;

MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget)
{
    ui->setupUi(this);

    QStringList stripeButtonsNames = QStringList();
    stripeButtonsNames << "Controls" << "ItemViews" << "Styles" << "Layouts";
    ui->buttonStripe->addButtons(stripeButtonsNames, ui->stackedWidget->currentIndex());
    connect(ui->buttonStripe, &AzQtComponents::ButtonStripe::buttonClicked,
            ui->stackedWidget, &QStackedWidget::setCurrentIndex);

    initializeControls();
    initializeItemViews();

    connect(ui->button1, &QPushButton::clicked, this, []{
        auto mainwindow = new QMainWindow();
        auto menu = new QMenu("File");
        auto mb = new QMenuBar();
        menu->addAction("Action1");
        menu->addAction("Action2");
        menu->addAction("Action3");
        mb->addMenu(menu);
        mb->addAction("Edit");
        mb->addMenu("Create");
        mb->addMenu("Select");
        mb->addMenu("Modify");
        mb->addMenu("Display");
        mainwindow->setMenuBar(mb);
        mainwindow->resize(400, 400);
        mainwindow->show();
        mainwindow->raise();
    });
}

MainWidget::~MainWidget()
{
    delete ui;
    qDebug() << "Deleted main widget";
}

void MainWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        emit reloadCSS();
    }
}

void MainWidget::initializeControls()
{
    auto menu = new QMenu();
    menu->addAction("Test 1");
    menu->addAction("Test 2");
    ui->button3->setMenu(menu);

    auto menu2 = new QMenu();
    menu2->addAction("Test 1");
    menu2->addAction("Test 2");
    ui->disabledButton3->setMenu(menu2);

    ui->selectedCheckBox->setCheckState(Qt::Checked);
    ui->enabledCheckBox->setCheckState(Qt::Unchecked);
    ui->tristateCheckBox->setCheckState(Qt::PartiallyChecked);

    // TODO: Move these into subclass, can't be done from .css
    // Nicolas: or do it in the proxy style
    // Sergio: css doesn't work well with proxy style , says Qt's docs.
    ui->progressBar->setFormat(QString());
    ui->progressBar->setMaximumHeight(8);
    ui->progressBarFull->setFormat(QString());
    ui->progressBarFull->setMaximumHeight(8);

    ui->button2->setProperty("class", "Primary");
    ui->disabledButton1->setProperty("class", "Primary");

    ui->invalidLineEdit->setFlavor(AzQtComponents::StyledLineEdit::Invalid);
    ui->validLineEdit->setFlavor(AzQtComponents::StyledLineEdit::Valid);
    ui->questionLineEdit->setFlavor(AzQtComponents::StyledLineEdit::Question);
    ui->infoLineEdit->setFlavor(AzQtComponents::StyledLineEdit::Information);

    // In order to try out validator impact on flavor
    ui->invalidLineEdit->setPlaceholderText("Enter 4 digits");
    ui->invalidLineEdit->setValidator(new QRegExpValidator(QRegExp("[1-9]\\d{3,3}")));

    ui->disabledVectorEdit->setColors(qRgb(84, 190, 93), qRgb(226, 82, 67), qRgb(66, 133, 244));
    ui->vectorEdit->setColors(qRgb(84, 190, 93), qRgb(226, 82, 67), qRgb(66, 133, 244));
    ui->unknownVectorEdit->setColors(qRgb(84, 190, 93), qRgb(226, 82, 67), qRgb(66, 133, 244));
    ui->warningVectorEdit->setColors(qRgb(84, 190, 93), qRgb(226, 82, 67), qRgb(66, 133, 244));
    ui->warningVectorEdit->setFlavor(AzQtComponents::VectorEditElement::Invalid);

    QStringList rpy = {tr("R"), tr("P"), tr("Y")};
    ui->disabledRPYVectorEdit->setLabels(rpy);
    ui->rPYVectorEdit->setLabels(rpy);
    ui->unknownRPYVectorEdit->setLabels(rpy);
    ui->warningRPYVectorEdit->setLabels(rpy);

    ui->filterNameLineEdit->setClearButtonEnabled(true);

    ui->comboBox_4->setEnabled(false);
    for (int i = 0; i < 10; ++i) {
        ui->comboBox->addItem(QStringLiteral("combo 0: Item %1").arg(i));
        ui->comboBox_2->addItem(QStringLiteral("combo 2: Item %1").arg(i));
        ui->comboBox_3->addItem(QStringLiteral("combo 3: Item %1").arg(i));
        ui->comboBox_4->addItem(QStringLiteral("combo 4: Item %1").arg(i));
        ui->comboBox_5->addItem(QStringLiteral("combo 5: Item %1").arg(i));
        ui->comboBox_6->addItem(QStringLiteral("combo 6: Item %1").arg(i));
    }

    ui->allCombo->setProperty("class", "condition");
    ui->allCombo->setFixedWidth(37);
    ui->allCombo->setFixedHeight(21);
    ui->allCombo->addItem("all");
    ui->allCombo->addItem("any");


    // Construct the available tag list.
    const int numAvailableTags = 128;
    QVector<QString> availableTags;
    availableTags.reserve(numAvailableTags+8);

    availableTags.push_back("Healthy");
    availableTags.push_back("Tired");
    availableTags.push_back("Laughing");
    availableTags.push_back("Happy");
    availableTags.push_back("Smiling");
    availableTags.push_back("Weapon Left");
    availableTags.push_back("Weapon Right");
    availableTags.push_back("Loooooooooooooooooong Tag");

    for (const QString& tag : availableTags)
    {
        ui->searchWidget->AddTypeFilter("Foo", tag);
    }

    for (int i = 0; i < numAvailableTags; ++i)
    {
        QString tagName = "tag" + QString::number(i + 1);
        availableTags.push_back(tagName);
        if (i < 10)
        {
            ui->searchWidget->AddTypeFilter("Bar", tagName);
        }
    }

    ui->tagSelector->Reinit(availableTags);

    // Pre-select some of the tags.
    ui->tagSelector->SelectTag("Healthy");
    ui->tagSelector->SelectTag("Laughing");
    ui->tagSelector->SelectTag("Smiling");
    ui->tagSelector->SelectTag("Happy");
    ui->tagSelector->SelectTag("Weapon Left");


    ui->toolButton->setProperty("class", "QToolBar");
    ui->toolButton_2->setProperty("class", "QToolBar");
    ui->toolButton_3->setProperty("class", "QToolBar");
    ui->toolButton_4->setProperty("class", "QToolBar");
    ui->toolButton_5->setProperty("class", "QToolBar");
    ui->toolButton_6->setProperty("class", "QToolBar");
    ui->toolButton_7->setProperty("class", "QToolBar");
    ui->toolButton_8->setProperty("class", "QToolBar");
    ui->toolButton_9->setProperty("class", "QToolBar");
    ui->toolButton_10->setProperty("class", "QToolBar");
    ui->toolButton_11->setProperty("class", "QToolBar");
    ui->toolButton_12->setProperty("class", "QToolBar");

    ui->conditionGroup->appendCondition();

    // place the viewportTitle menu into the place holder we prepared in the ui file
    /*ViewportTitleDlg* viewPortTitleDlg = new ViewportTitleDlg(ui->ViewportTitle);
    auto viewPortTitleLayout = new QHBoxLayout();
    viewPortTitleLayout->addWidget(viewPortTitleDlg);
    ui->ViewportTitle->setLayout(viewPortTitleLayout);*/

    QStringList fontSizeMockup = {"8pt", "9pt", "10pt", "11pt", "12pt", "14pt", "16pt", "18pt", "24pt", "30pt",};
    QStringList viewZoomMockup = {"20", "32", "40", "80", "160", "240"};
    const auto tbcbList = ui->toolButtonsComboBoxWidget->findChildren<AzQtComponents::ToolButtonComboBox*>();
    foreach (auto tbcb, tbcbList) {
        if (tbcb->objectName().contains("1")) {
            tbcb->setIcon(QIcon(QStringLiteral(":/stylesheet/img/16x16/Font_text.png")));
            tbcb->comboBox()->addItems(fontSizeMockup);
        } else if (tbcb->objectName().contains("2")) {
            tbcb->setIcon(QIcon(QStringLiteral(":/stylesheet/img/16x16/Picture_view.png")));
            tbcb->comboBox()->addItems(viewZoomMockup);
        } else if (tbcb->objectName().contains("3")) {
            tbcb->setIcon(QIcon(QStringLiteral(":/stylesheet/img/16x16/List_view.png")));
            tbcb->comboBox()->addItems(fontSizeMockup);
        }
    }

    ui->WidgetHeader1->setForceInactive(true);
    ui->WidgetHeader1->setButtons({ AzQtComponents::DockBarButton::DividerButton, AzQtComponents::DockBarButton::MaximizeButton,
                                    AzQtComponents::DockBarButton::DividerButton, AzQtComponents::DockBarButton::CloseButton });
    ui->WidgetHeader1->setWindowTitleOverride("Widget Header");
    ui->WidgetHeader2->setButtons({AzQtComponents::DockBarButton::DividerButton, AzQtComponents::DockBarButton::MaximizeButton,
                                   AzQtComponents::DockBarButton::DividerButton, AzQtComponents::DockBarButton::CloseButton});
    ui->WidgetHeader2->setWindowTitleOverride("Widget Header");

    ui->NonDockableWidget1->setForceInactive(true);
    ui->NonDockableWidget1->setWindowTitleOverride("Non-Dockable Widget");
    ui->NonDockableWidget1->setButtons({ AzQtComponents::DockBarButton::CloseButton });
    ui->NonDockableWidget1->setTearEnabled(false);
    ui->NonDockableWidget1->setDragEnabled(false);
    ui->NonDockableWidget2->setWindowTitleOverride("Non-Dockable Widget");
    ui->NonDockableWidget2->setButtons({ AzQtComponents::DockBarButton::CloseButton });
    ui->NonDockableWidget2->setTearEnabled(false);
    ui->NonDockableWidget2->setDragEnabled(false);

    const auto rpbList = ui->RoundedButtonWidget->findChildren<QPushButton*>();
    foreach (auto rpb, rpbList) {
        if (rpb->objectName().contains("Small"))
            rpb->setProperty("class", "smallRounded");
        else
            rpb->setProperty("class", "rounded");
    }

    ui->buttonOrange1->setProperty("class", "Primary");
    ui->buttonOrange2->setProperty("class", "Primary");

    ui->tabWidget->tabBar()->setTabsClosable(true);
    connect(ui->tabWidget->tabBar(), &QTabBar::tabCloseRequested, this, [this](int index) {
        ui->tabWidget->removeTab(index);
    });
}

void MainWidget::initializeItemViews()
{
    auto model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({QString(), tr("Priority"), tr("State"), tr("ID"), tr("Completed"), tr("Platform")});

    for (int i = 0; i < 20; ++i) {
        auto col0Item = new QStandardItem();
        col0Item->setCheckState(Qt::Checked);
        col0Item->setData(QVariant(QSize(10, 20)), Qt::SizeHintRole);
        col0Item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        auto col1Item = new QStandardItem((i < 10) ? tr("Top"): "");
        model->appendRow({
            col0Item,
            col1Item,
            new QStandardItem(i == 0 ? tr("Running") : (i < 10 ? tr("Failed") : tr("Waiting"))),
            new QStandardItem("Environnment/Sky/Cloud/Cloudy-cloud"),
            new QStandardItem(QString::number(i)),
            new QStandardItem(i % 2 ? tr("Android") : tr("iPhone"))
        });
    }

    ui->treeView->setModel(model);
    ui->treeView->setSortingEnabled(true);
    ui->treeView->setColumnWidth(0, 50);
}

#include "StyleGallery/moc_mainwidget.cpp"
