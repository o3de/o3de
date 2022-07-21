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
#include <QDialogButtonBox>


namespace O3DE::ProjectManager
{
    CreateAGemScreen::CreateAGemScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setSpacing(0);
        vLayout->setContentsMargins(0, 0, 0, 0);

        QScrollArea* firstScreenScrollArea = CreateFirstScreen();
        QScrollArea* secondScreenScrollArea = CreateSecondScreen();
        QScrollArea* thirdScreenScrollArea = CreateThirdScreen();

        connect(m_gemDisplayName->lineEdit(), &QLineEdit::textChanged, this, &CreateAGemScreen::OnGemDisplayNameUpdated);
        connect(m_gemSystemName->lineEdit(), &QLineEdit::textChanged, this, &CreateAGemScreen::OnGemSystemNameUpdated);
        connect(m_license->lineEdit(), &QLineEdit::textChanged, this, &CreateAGemScreen::OnLicenseNameUpdated);
        connect(m_creatorName->lineEdit(), &QLineEdit::textChanged, this, &CreateAGemScreen::OnCreatorNameUpdated);
        connect(m_repositoryURL->lineEdit(), &QLineEdit::textChanged, this, &CreateAGemScreen::OnRepositoryURLUpdated);

        m_tabWidget = new TabWidget();
        m_tabWidget->setObjectName("createAGemTab");
        m_tabWidget->addTab(firstScreenScrollArea, "1. Gem Setup");
        m_tabWidget->addTab(secondScreenScrollArea, "2. Gem Details");
        m_tabWidget->addTab(thirdScreenScrollArea, "3. Creator Details");
        m_tabWidget->tabBar()->setEnabled(false);

        
        vLayout->addWidget(m_tabWidget);

        QFrame* footerFrame = new QFrame();
        footerFrame->setObjectName("createAGemFooter");
        m_backNextButtons = new QDialogButtonBox();
        m_backNextButtons->setObjectName("footer");
        QVBoxLayout* footerLayout = new QVBoxLayout();
        footerLayout->setContentsMargins(0, 0, 0, 0);
        footerFrame->setLayout(footerLayout);
        footerLayout->addWidget(m_backNextButtons);
        vLayout->addWidget(footerFrame);


        m_backButton = m_backNextButtons->addButton(tr("Back"), QDialogButtonBox::RejectRole);
        m_backButton->setProperty("secondary", true);
        m_nextButton = m_backNextButtons->addButton(tr("Next"), QDialogButtonBox::ApplyRole);

        connect(m_tabWidget, &TabWidget::currentChanged, this, &CreateAGemScreen::ConvertNextButtonToCreate);
        connect(m_backButton, &QPushButton::clicked, this, &CreateAGemScreen::HandleBackButton);
        connect(m_nextButton, &QPushButton::clicked, this, &CreateAGemScreen::HandleNextButton);

        connect(m_nextButton, &QPushButton::clicked, this, &CreateAGemScreen::OnGemDisplayNameUpdated);
        connect(m_nextButton, &QPushButton::clicked, this, &CreateAGemScreen::OnGemSystemNameUpdated);
        connect(m_nextButton, &QPushButton::clicked, this, &CreateAGemScreen::OnLicenseNameUpdated);
        connect(m_nextButton, &QPushButton::clicked, this, &CreateAGemScreen::OnCreatorNameUpdated);
        connect(m_nextButton, &QPushButton::clicked, this, &CreateAGemScreen::OnRepositoryURLUpdated);

        setObjectName("createAGemBody");
        setLayout(vLayout);

    }

    QScrollArea* CreateAGemScreen::CreateFirstScreen()
    {
        QScrollArea* firstScreenScrollArea = new QScrollArea();
        firstScreenScrollArea->setWidgetResizable(true);
        firstScreenScrollArea->setMinimumWidth(660);
        firstScreenScrollArea->setObjectName("createAGemRightPane");
        QFrame* firstScreenFrame = new QFrame();

        QWidget* firstScreenScrollWidget = new QWidget();
        firstScreenScrollArea->setWidget(firstScreenScrollWidget);

        QVBoxLayout* firstScreen = new QVBoxLayout();
        firstScreen->setAlignment(Qt::AlignTop);
        firstScreen->addWidget(firstScreenFrame);
        firstScreenScrollWidget->setLayout(firstScreen);
        QLabel* rightPaneHeader = new QLabel();
        rightPaneHeader->setObjectName("rightPaneHeader");
        rightPaneHeader->setText(tr("Please Choose a Gem Template"));
        QLabel* rightPaneSubheader = new QLabel();
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

        FormFolderBrowseEditWidget* gemTemplateLocation = new FormFolderBrowseEditWidget(tr("Gem Template Location*"), "");

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
        firstScreen->addWidget(gemTemplateLocation);

        return firstScreenScrollArea;
    }

    QScrollArea* CreateAGemScreen::CreateSecondScreen()
    {
        QScrollArea* secondScreenScrollArea = new QScrollArea();
        secondScreenScrollArea->setWidgetResizable(true);
        secondScreenScrollArea->setMinimumWidth(660);
        secondScreenScrollArea->setObjectName("createAGemRightPane");
        QFrame* secondScreenFrame = new QFrame();

        QWidget* secondScreenScrollWidget = new QWidget();
        secondScreenScrollArea->setWidget(secondScreenScrollWidget);

        QVBoxLayout* secondScreen = new QVBoxLayout();
        secondScreen->setAlignment(Qt::AlignTop);
        secondScreen->addWidget(secondScreenFrame);
        secondScreenScrollWidget->setLayout(secondScreen);

        QLabel* secondRightPaneHeader = new QLabel();
        secondRightPaneHeader->setObjectName("rightPaneHeader");
        secondRightPaneHeader->setText(tr("Enter Gem Details"));
        secondScreen->addWidget(secondRightPaneHeader);

        m_gemDisplayName = new FormLineEditWidget(tr("Gem Display name*"), "");
        QLabel* diplayNameErrorLabel = m_gemDisplayName->getErrorLabel();
        diplayNameErrorLabel->setObjectName("createAGemFormErrorLabel");
        QLineEdit* gemDisplayLineEdit = m_gemDisplayName->lineEdit();
        gemDisplayLineEdit->setPlaceholderText(tr("The name displayed on the Gem Catalog"));
        secondScreen->addWidget(m_gemDisplayName);

        m_gemSystemName = new FormLineEditWidget(tr("Gem System name*"), "");
        QLabel* gemSystemNameErrorLabel = m_gemSystemName->getErrorLabel();
        gemSystemNameErrorLabel->setObjectName("createAGemFormErrorLabel");
        QLineEdit* gemSystemLineEdit = m_gemSystemName->lineEdit();
        gemSystemLineEdit->setPlaceholderText(tr("The name used in the O3DE Gem System"));
        secondScreen->addWidget(m_gemSystemName);

        FormLineEditWidget* gemSummary = new FormLineEditWidget(tr("Gem Summary"), "");
        QLineEdit* gemSummaryLineEdit = gemSummary->lineEdit();
        gemSummaryLineEdit->setPlaceholderText(tr("A short description of your Gem"));
        secondScreen->addWidget(gemSummary);

        FormLineEditWidget* requirements = new FormLineEditWidget(tr("Requirements"), "");
        QLineEdit* requirementsLineEdit = requirements->lineEdit();
        requirementsLineEdit->setPlaceholderText(tr("Notice of any requirements your Gem. i.e. This requires X other gem"));
        secondScreen->addWidget(requirements);

        m_license = new FormLineEditWidget(tr("License*"), "");
        QLabel* licenseErrorLabel = m_license->getErrorLabel();
        licenseErrorLabel->setObjectName("createAGemFormErrorLabel");
        QLineEdit* licenseLineEdit = m_license->lineEdit();
        licenseLineEdit->setPlaceholderText(tr("License uses goes here: i.e. Apache-2.0 or MIT"));
        secondScreen->addWidget(m_license);

        FormLineEditWidget* licenseURL = new FormLineEditWidget(tr("License URL"), "");
        QLineEdit* licenseURLLineEdit = licenseURL->lineEdit();
        licenseURLLineEdit->setPlaceholderText(tr("Link to the license web site i.e. https://opensource.org/licenses/Apache-2.0"));
        secondScreen->addWidget(licenseURL);

        FormLineEditWidget* origin = new FormLineEditWidget(tr("Origin"), "");
        QLineEdit* originLineEdit = origin->lineEdit();
        originLineEdit->setPlaceholderText(tr("The name of the originator goes. i.e. XYZ Inc."));
        secondScreen->addWidget(origin);

        FormLineEditWidget* originURL = new FormLineEditWidget(tr("Origin URL"), "");
        QLineEdit* originURLLineEdit = originURL->lineEdit();
        originURLLineEdit->setPlaceholderText(tr("The primary repo for your Gem. i.e. http://www.mydomain.com"));
        secondScreen->addWidget(originURL);

        FormLineEditWidget* userDefinedGemTags = new FormLineEditWidget(tr("User-defined Gem Tags"), "");
        secondScreen->addWidget(userDefinedGemTags);

        FormFolderBrowseEditWidget* gemLocation = new FormFolderBrowseEditWidget(tr("Gem Location"), "");
        QLineEdit* gemLocationLineEdit = gemLocation->lineEdit();
        const char* gemLocationPlaceholderText = R"(C:\O3DE\Project\Gems\YourGem)";
        gemLocationLineEdit->setPlaceholderText(tr(gemLocationPlaceholderText));
        secondScreen->addWidget(gemLocation);

        FormFolderBrowseEditWidget* gemIconPath = new FormFolderBrowseEditWidget(tr("Gem Icon Path"), "");
        QLineEdit* gemIconPathLineEdit = gemIconPath->lineEdit();
        gemIconPathLineEdit->setPlaceholderText(tr("Select Gem icon path"));
        secondScreen->addWidget(gemIconPath);

        FormLineEditWidget* documentationURL = new FormLineEditWidget(tr("Documentation URL"), "");
        QLineEdit* documentationURLLineEdit = documentationURL->lineEdit();
        documentationURLLineEdit->setPlaceholderText(
            tr("Link to any documentation of your Gem i.e. https://o3de.org/docs/user-guide/gems/..."));
        secondScreen->addWidget(documentationURL);

        return secondScreenScrollArea;
    }

    QScrollArea* CreateAGemScreen::CreateThirdScreen()
    {
        QScrollArea* thirdScreenScrollArea = new QScrollArea();
        thirdScreenScrollArea->setWidgetResizable(true);
        thirdScreenScrollArea->setMinimumWidth(660);
        thirdScreenScrollArea->setObjectName("createAGemRightPane");
        QFrame* thirdScreenFrame = new QFrame();

        QWidget* thirdScreenScrollWidget = new QWidget();
        thirdScreenScrollArea->setWidget(thirdScreenScrollWidget);

        QVBoxLayout* thirdScreen = new QVBoxLayout();
        thirdScreen->setAlignment(Qt::AlignTop);
        thirdScreen->addWidget(thirdScreenFrame);
        thirdScreenScrollWidget->setLayout(thirdScreen);

        QLabel* thirdRightPaneHeader = new QLabel(this);
        thirdRightPaneHeader->setObjectName("rightPaneHeader");
        thirdRightPaneHeader->setText(tr("Enter your Details"));
        thirdScreen->addWidget(thirdRightPaneHeader);

        m_creatorName = new FormLineEditWidget(tr("Creator Name*"), "", this);
        QLabel* creatorNameErrorLabel = m_creatorName->getErrorLabel();
        creatorNameErrorLabel->setObjectName("createAGemFormErrorLabel");
        QLineEdit* creatorNameLineEdit = m_creatorName->lineEdit();
        creatorNameLineEdit->setPlaceholderText(tr("John Smith"));
        thirdScreen->addWidget(m_creatorName);

        m_repositoryURL = new FormLineEditWidget(tr("Repository URL*"), "", this);
        QLabel* repositoryURLErrorLabel = m_repositoryURL->getErrorLabel();
        repositoryURLErrorLabel->setObjectName("createAGemFormErrorLabel");
        QLineEdit* repositoryURLLineEdit = m_repositoryURL->lineEdit();
        repositoryURLLineEdit->setPlaceholderText(tr("https://github.com/Jane"));
        thirdScreen->addWidget(m_repositoryURL);

        return thirdScreenScrollArea;
    }

    bool CreateAGemScreen::ValidateGemDisplayName()
    {
        bool gemScreenNameIsValid = true;
        if (m_tabWidget->currentIndex() == 2)
        {
            if (m_gemDisplayName->lineEdit()->text().isEmpty())
            {
                gemScreenNameIsValid = false;
                m_gemDisplayName->setErrorLabelText(tr("A gem display name is required."));
            }

            m_gemDisplayName->setErrorLabelVisible(!gemScreenNameIsValid);
        }
        return gemScreenNameIsValid;
    }

    bool CreateAGemScreen::ValidateGemSystemName()
    {
        bool gemSystemNameIsValid = true;
        if (m_tabWidget->currentIndex() == 2)
        {
            if (m_gemSystemName->lineEdit()->text().isEmpty())
            {
                gemSystemNameIsValid = false;
                m_gemSystemName->setErrorLabelText(tr("A gem system name is required."));
            }

            m_gemSystemName->setErrorLabelVisible(!gemSystemNameIsValid);
        }
        return gemSystemNameIsValid;

    }

    bool CreateAGemScreen::ValidateLicenseName()
    {
        bool licenseNameIsValid = true;
        if (m_tabWidget->currentIndex() == 2)
        {
            if (m_license->lineEdit()->text().isEmpty())
            {
                licenseNameIsValid = false;
                m_license->setErrorLabelText(tr("License details are required."));
            }

            m_license->setErrorLabelVisible(!licenseNameIsValid);
        }
        return licenseNameIsValid;
    }

    bool CreateAGemScreen::ValidateCreatorName()
    {
        bool creatorNameIsValid = true;
        if (m_tabWidget->currentIndex() == 2)
        {
            m_numNextClicks += 1;
            if (m_numNextClicks > 1)
            {
                if (m_creatorName->lineEdit()->text().isEmpty())
                {
                    creatorNameIsValid = false;
                    m_creatorName->setErrorLabelText(tr("A creator's name is required."));
                }

                m_creatorName->setErrorLabelVisible(!creatorNameIsValid);
            }
        }
        return creatorNameIsValid;
    }

    bool CreateAGemScreen::ValidateRepositoryURL()
    {
        bool repositoryURLIsValid = true;
        if (m_tabWidget->currentIndex() == 2)
        {
            if (m_numNextClicks > 1)
            {
                if (m_repositoryURL->lineEdit()->text().isEmpty())
                {
                    repositoryURLIsValid = false;
                    m_repositoryURL->setErrorLabelText(tr("A repository URL is required."));
                }

                m_repositoryURL->setErrorLabelVisible(!repositoryURLIsValid);
            }
        }
        return repositoryURLIsValid;
    }

    void CreateAGemScreen::OnGemDisplayNameUpdated()
    {
        ValidateGemDisplayName();
    }

    void CreateAGemScreen::OnGemSystemNameUpdated()
    {
        ValidateGemSystemName();
    }

    void CreateAGemScreen::OnLicenseNameUpdated()
    {
        ValidateLicenseName();
    }

    void CreateAGemScreen::OnCreatorNameUpdated()
    {
        ValidateCreatorName();
    }

    void CreateAGemScreen::OnRepositoryURLUpdated()
    {
        ValidateRepositoryURL();
    }

    void CreateAGemScreen::HandleBackButton()
    {
        if (m_tabWidget->currentIndex() == 2)
        {
            m_tabWidget->setCurrentIndex(1);
            m_nextButton->setText("Next");
        }
        else if (m_tabWidget->currentIndex() == 1)
        {
            m_tabWidget->setCurrentIndex(0);
        }
    }

    void CreateAGemScreen::HandleNextButton()
    {
        if (m_tabWidget->currentIndex() == 0)
        {
            m_tabWidget->setCurrentIndex(1);
        }
        else if (m_tabWidget->currentIndex() == 1)
        {
            m_tabWidget->setCurrentIndex(2);
            m_nextButton->setText("Create");
        }
    }

    void CreateAGemScreen::ConvertNextButtonToCreate()
    {
        if (m_tabWidget->currentIndex() == 0)
        {
            m_nextButton->setText("Next");
        }
        else if (m_tabWidget->currentIndex() == 1)
        {
            m_nextButton->setText("Next");
        }
        else if (m_tabWidget->currentIndex() == 2)
        {
            m_nextButton->setText("Create");
        }
    }

} // namespace O3DE::ProjectManager

