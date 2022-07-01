/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AdjustableHeaderWidget.h>
#include <DownloadController.h>
#include <GemCatalog/GemCatalogHeaderWidget.h>
#include <CreateAGemScreen.h>
#include <ProjectUtils.h>
#include <PythonBindingsInterface.h>
#include <FormFolderBrowseEditWidget.h>
#include <FormLineEditWidget.h>

#include <PythonBindingsInterface.h>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHash>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QLabel>
#include <QScrollArea>



namespace O3DE::ProjectManager
{
    CreateAGemScreen::CreateAGemScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(0, 0, 0, 0);

        QHBoxLayout* widgetsHolder = new QHBoxLayout();
        QFrame* leftFrame = new QFrame();
        leftFrame->setObjectName("createAGemLeftPane");
        QVBoxLayout* leftPane = new QVBoxLayout();

        QLabel* firstPageString = new QLabel(this);
        firstPageString->setObjectName("leftPaneText");
        firstPageString->setText(tr("1. Gem Setup"));
        firstPageString->setAlignment(Qt::AlignTop);
        firstPageString->setMinimumHeight(60);
        QLabel* secondPageString = new QLabel(this);
        secondPageString->setText(tr("2. Gem Details"));
        secondPageString->setObjectName("leftPaneText");
        secondPageString->setAlignment(Qt::AlignTop);
        secondPageString->setMinimumHeight(60);
        QLabel* thirdPageString = new QLabel(this);
        thirdPageString->setText(tr("3. Creator Details"));
        thirdPageString->setObjectName("leftPaneText");
        thirdPageString->setAlignment(Qt::AlignTop);
        thirdPageString->setMinimumHeight(60);

        leftPane->addWidget(firstPageString);
        leftPane->addWidget(secondPageString);
        leftPane->addWidget(thirdPageString);
        leftPane->addStretch();

        leftFrame->setLayout(leftPane);

        QFrame* rightFrame = new QFrame();
        rightFrame->setObjectName("createAGemRightPane");
        QVBoxLayout* rightPane = new QVBoxLayout();
        QScrollArea* m_scrollArea = new QScrollArea();
        m_scrollArea->setWidgetResizable(true);
        m_scrollArea->setMinimumWidth(660);
        //QStackedWidget* m_stackedWidget = new QStackedWidget();

        QVBoxLayout* firstScreen = new QVBoxLayout();

        QLabel* rightPaneHeader = new QLabel(this);
        rightPaneHeader->setObjectName("rightPaneHeader");
        rightPaneHeader->setText(tr("Please Choose a Gem Template"));
        QLabel* rightPaneSubheader = new QLabel(this);
        rightPaneSubheader->setObjectName("rightPaneSubheader");
        rightPaneSubheader->setText(tr("Gems can contain assets such as scripts, animations, meshes, textures, and more."));
        firstScreen->addWidget(rightPaneHeader);
        firstScreen->addWidget(rightPaneSubheader);

        QRadioButton* button1 = new QRadioButton("Default Gem");
        button1->setObjectName("qRadioButton");
        QRadioButton* button2 = new QRadioButton("Prebuilt Gem");
        button2->setObjectName("qRadioButton");
        QRadioButton* button3 = new QRadioButton("Asset Gem");
        button3->setObjectName("qRadioButton");
        QRadioButton* button4 = new QRadioButton("C++ Tool Gem");
        button4->setObjectName("qRadioButton");
        QRadioButton* button5 = new QRadioButton("Python Tool Gem");
        button5->setObjectName("qRadioButton");
        QRadioButton* button6 = new QRadioButton("Choose existing template");
        button6->setObjectName("qRadioButton");

        QLabel* button1Subtext = new QLabel();
        button1Subtext->setObjectName("qRadioButtonSubtext");
        button1Subtext->setText(tr("A gem template that will be used if no gem template is specified during gem creation."));
        QLabel* button2Subtext = new QLabel();
        button2Subtext->setObjectName("qRadioButtonSubtext");
        button2Subtext->setText(tr("A gem template that can be used to create a gem that where pre-built libraries can be hooked into."));
        QLabel* button3Subtext = new QLabel();
        button3Subtext->setObjectName("qRadioButtonSubtext");
        button3Subtext->setText(tr("A gem template to create the minimal gem needed for adding asset(s)."));
        QLabel* button4Subtext = new QLabel();
        button4Subtext->setObjectName("qRadioButtonSubtext");
        button4Subtext->setText(tr("A gem template for a custom tool in C++ that gets registered with the editor."));
        QLabel* button5Subtext = new QLabel();
        button5Subtext->setObjectName("qRadioButtonSubtext");
        button5Subtext->setText(tr("A gem template for a custom tool in Python that gets registered with the editor."));


        firstScreen->addWidget(button1);    
        firstScreen->addWidget(button1Subtext);
        firstScreen->addWidget(button2);
        firstScreen->addWidget(button2Subtext);
        firstScreen->addWidget(button3);
        firstScreen->addWidget(button3Subtext);
        firstScreen->addWidget(button4);
        firstScreen->addWidget(button4Subtext);
        firstScreen->addWidget(button5);
        firstScreen->addWidget(button5Subtext);
        firstScreen->addWidget(button6);

        firstScreen->addStretch();

        rightPane->addLayout(firstScreen);

        /*
        QVBoxLayout* secondScreen = new QVBoxLayout();

        QLabel* secondRightPaneHeader = new QLabel(this);
        secondRightPaneHeader->setObjectName("rightPaneHeader");
        secondRightPaneHeader->setText(tr("Enter Gem Details"));
        secondScreen->addWidget(secondRightPaneHeader);

        FormLineEditWidget* displayName = new FormLineEditWidget(tr("Display name*"), "", this);
        secondScreen->addWidget(displayName);
        FormLineEditWidget* gemSystemName = new FormLineEditWidget(tr("Gem System name*"), "", this);
        secondScreen->addWidget(gemSystemName);
        FormLineEditWidget* gemSummary = new FormLineEditWidget(tr("Gem Summary"), "", this);
        secondScreen->addWidget(gemSummary);
        FormLineEditWidget* requirements = new FormLineEditWidget(tr("Requirements"), "", this);
        secondScreen->addWidget(requirements);
        FormLineEditWidget* license = new FormLineEditWidget(tr("License*"), "", this);
        secondScreen->addWidget(license);
        FormLineEditWidget* licenseURL = new FormLineEditWidget(tr("License URL"), "", this);
        secondScreen->addWidget(licenseURL);
        FormLineEditWidget* origin = new FormLineEditWidget(tr("Origin"), "", this);
        secondScreen->addWidget(origin);
        FormLineEditWidget* originURL = new FormLineEditWidget(tr("Origin URL"), "", this);
        secondScreen->addWidget(originURL);
        FormLineEditWidget* userDefinedGemTags = new FormLineEditWidget(tr("User-defined Gem Tags"), "", this);
        secondScreen->addWidget(userDefinedGemTags);
        FormFolderBrowseEditWidget* gemLocation = new FormFolderBrowseEditWidget(tr("Gem Location"), "", this);
        secondScreen->addWidget(gemLocation);
        FormFolderBrowseEditWidget* gemIconPath = new FormFolderBrowseEditWidget(tr("Gem Icon Path"), "", this);
        secondScreen->addWidget(gemIconPath);
        FormLineEditWidget* documentationURL = new FormLineEditWidget(tr("Documentation URL"), "", this);
        secondScreen->addWidget(documentationURL);
        secondScreen->addStretch();

        rightPane->addLayout(secondScreen);
        */

        /*
        QVBoxLayout* thirdScreen = new QVBoxLayout();

        QLabel* thirdRightPaneHeader = new QLabel(this);
        thirdRightPaneHeader->setObjectName("rightPaneHeader");
        thirdRightPaneHeader->setText(tr("Enter your Details"));
        thirdScreen->addWidget(thirdRightPaneHeader);

        FormLineEditWidget* creatorName = new FormLineEditWidget(tr("Creator Name*"), "", this);
        thirdScreen->addWidget(creatorName);
        FormLineEditWidget* repoURL = new FormLineEditWidget(tr("Repository URL*"), "", this);
        thirdScreen->addWidget(repoURL);
        thirdScreen->addStretch();

        rightPane->addLayout(thirdScreen);
        */


        rightFrame->setLayout(rightPane);

        /*

        QStackedWidget* m_stack = new QStackedWidget(this);
        m_stack->setObjectName("body");
        m_stack->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        QFrame* rightFrame = new QFrame();
        rightFrame->setLayout(rightPane);
        //rightPane->addWidget(m_stack);

        */

        m_scrollArea->setWidget(rightFrame);



        widgetsHolder->addWidget(leftFrame);
        widgetsHolder->addWidget(m_scrollArea);
        vLayout->addLayout(widgetsHolder);

        setLayout(vLayout);

    }

} // namespace O3DE::ProjectManager

