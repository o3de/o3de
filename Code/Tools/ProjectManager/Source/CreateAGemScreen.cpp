/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemCatalogHeaderWidget.h>
#include <CreateAGemScreen.h>
#include <ProjectUtils.h>
#include <PythonBindingsInterface.h>

#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
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

        m_tabWidget = new TabWidget();
        
        m_tabWidget->setObjectName("createAGemTab");
        m_tabWidget->addTab(CreateFirstScreen(), tr("1. Gem Setup"));
        m_tabWidget->addTab(CreateSecondScreen(), tr("2. Gem Details"));
        m_tabWidget->addTab(CreateThirdScreen(), tr("3. Creator Details"));
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

        connect(m_tabWidget, &TabWidget::currentChanged, this, &CreateAGemScreen::UpdateNextButtonToCreate);
        connect(m_backButton, &QPushButton::clicked, this, &CreateAGemScreen::HandleBackButton);
        connect(m_nextButton, &QPushButton::clicked, this, &CreateAGemScreen::HandleNextButton);

        setObjectName("createAGemBody");
        setLayout(vLayout);
    }

    void CreateAGemScreen::LoadButtonsFromGemTemplatePaths([[maybe_unused]]QVBoxLayout* firstScreen)
    {
        QHash<QString, QString> formattedNames;
        formattedNames.insert(QString("Prebuilt Gem Tenokate"), QString(tr("Prebuilt Gem")));
        formattedNames.insert(QString("Asset Gem Template"), QString(tr("Asset Gem")));
        formattedNames.insert(QString("Default Gem Template"), QString(tr("Default Gem")));
        formattedNames.insert(QString("CppToolGem"), QString(tr("C++ Tool Gem")));
        formattedNames.insert(QString("PythonToolGem"), QString(tr("Python Tool Gem")));

        m_radioButtonGroup = new QButtonGroup();
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

        LoadButtonsFromGemTemplatePaths(firstScreen);
        m_formFolderRadioButton = new QRadioButton("Choose existing template");
        m_formFolderRadioButton->setObjectName("createAGem");
        m_radioButtonGroup->addButton(m_formFolderRadioButton);

        m_gemTemplateLocation = new FormFolderBrowseEditWidget(tr("Gem Template Location*"), "");

        firstScreen->addWidget(m_formFolderRadioButton);
        firstScreen->addWidget(m_gemTemplateLocation);

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
        QLineEdit* gemDisplayLineEdit = m_gemDisplayName->lineEdit();
        gemDisplayLineEdit->setPlaceholderText(tr("The name displayed on the Gem Catalog"));
        secondScreen->addWidget(m_gemDisplayName);

        m_gemSystemName = new FormLineEditWidget(tr("Gem System name*"), "");
        QLineEdit* gemSystemLineEdit = m_gemSystemName->lineEdit();
        gemSystemLineEdit->setPlaceholderText(tr("The name used in the O3DE Gem System"));
        secondScreen->addWidget(m_gemSystemName);

        m_gemSummary = new FormLineEditWidget(tr("Gem Summary"), "");
        QLineEdit* gemSummaryLineEdit = m_gemSummary->lineEdit();
        gemSummaryLineEdit->setPlaceholderText(tr("A short description of your Gem"));
        secondScreen->addWidget(m_gemSummary);

        m_requirements = new FormLineEditWidget(tr("Requirements"), "");
        QLineEdit* requirementsLineEdit = m_requirements->lineEdit();
        requirementsLineEdit->setPlaceholderText(tr("Notice of any requirements your Gem. i.e. This requires X other gem"));
        secondScreen->addWidget(m_requirements);

        m_license = new FormLineEditWidget(tr("License*"), "");
        QLineEdit* licenseLineEdit = m_license->lineEdit();
        licenseLineEdit->setPlaceholderText(tr("License uses goes here: i.e. Apache-2.0 or MIT"));
        secondScreen->addWidget(m_license);

        m_licenseURL = new FormLineEditWidget(tr("License URL"), "");
        QLineEdit* licenseURLLineEdit = m_licenseURL->lineEdit();
        licenseURLLineEdit->setPlaceholderText(tr("Link to the license web site i.e. https://opensource.org/licenses/Apache-2.0"));
        secondScreen->addWidget(m_licenseURL);

        m_origin = new FormLineEditWidget(tr("Origin"), "");
        QLineEdit* originLineEdit = m_origin->lineEdit();
        originLineEdit->setPlaceholderText(tr("The name of the originator goes. i.e. XYZ Inc."));
        secondScreen->addWidget(m_origin);

        m_originURL = new FormLineEditWidget(tr("Origin URL"), "");
        QLineEdit* originURLLineEdit = m_originURL->lineEdit();
        originURLLineEdit->setPlaceholderText(tr("The primary repo for your Gem. i.e. http://www.mydomain.com"));
        secondScreen->addWidget(m_originURL);

        m_firstDropdown = new FormComboBoxWidget(tr("Global Gem Tag*"));
        AddDropdownActions(m_firstDropdown);
        secondScreen->addWidget(m_firstDropdown);

        m_secondDropdown = new FormComboBoxWidget(tr("Optional Global Gem Tag"));
        AddDropdownActions(m_secondDropdown);
        secondScreen->addWidget(m_secondDropdown);

        m_thirdDropdown = new FormComboBoxWidget(tr("Optional Global Gem Tag"));
        AddDropdownActions(m_thirdDropdown);
        secondScreen->addWidget(m_thirdDropdown);

        m_userDefinedGemTags = new FormLineEditWidget(tr("User-defined Gem Tags (Comma separated list)"), "");
        secondScreen->addWidget(m_userDefinedGemTags);

        m_gemLocation = new FormFolderBrowseEditWidget(tr("Gem Location"), "");
        QLineEdit* gemLocationLineEdit = m_gemLocation->lineEdit();
        const char* gemLocationPlaceholderText = R"(C:\O3DE\Project\Gems\YourGem)";
        gemLocationLineEdit->setPlaceholderText(tr(gemLocationPlaceholderText));
        secondScreen->addWidget(m_gemLocation);
        
        m_gemIconPath = new FormFolderBrowseEditWidget(tr("Gem Icon Path"), "");
        QLineEdit* gemIconPathLineEdit = m_gemIconPath->lineEdit();
        gemIconPathLineEdit->setPlaceholderText(tr("Select Gem icon path"));
        secondScreen->addWidget(m_gemIconPath);

        m_documentationURL = new FormLineEditWidget(tr("Documentation URL"), "");
        QLineEdit* documentationURLLineEdit = m_documentationURL->lineEdit();
        documentationURLLineEdit->setPlaceholderText(tr("Link to any documentation of your Gem i.e. https://o3de.org/docs/user-guide/gems/..."));
        secondScreen->addWidget(m_documentationURL);

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
        QLineEdit* creatorNameLineEdit = m_creatorName->lineEdit();
        creatorNameLineEdit->setPlaceholderText(tr("John Smith"));
        thirdScreen->addWidget(m_creatorName);

        m_repositoryURL = new FormLineEditWidget(tr("Repository URL*"), "", this);
        QLineEdit* repositoryURLLineEdit = m_repositoryURL->lineEdit();
        repositoryURLLineEdit->setPlaceholderText(tr("https://github.com/Jane"));
        thirdScreen->addWidget(m_repositoryURL);

        return thirdScreenScrollArea;
    }

    bool CreateAGemScreen::ValidateGemTemplateLocation()
    {
        bool gemTemplateLocationFilled = true;
        if (m_formFolderRadioButton->isChecked())
        {
            if (m_gemTemplateLocation->lineEdit()->text().isEmpty())
            {
                gemTemplateLocationFilled = false;
                m_gemTemplateLocation->setErrorLabelText(tr("A path must be provided."));
            }

        }
        m_gemTemplateLocation->setErrorLabelVisible(!gemTemplateLocationFilled);
        return gemTemplateLocationFilled;
    }

    bool CreateAGemScreen::ValidateGemDisplayName()
    {
        bool gemScreenNameIsValid = true;
        if (m_gemDisplayName->lineEdit()->text().isEmpty())
        {
            gemScreenNameIsValid = false;
            m_gemDisplayName->setErrorLabelText(tr("A gem display name is required."));
        }
        m_gemDisplayName->setErrorLabelVisible(!gemScreenNameIsValid);
        return gemScreenNameIsValid;
    }

    bool CreateAGemScreen::ValidateGemSystemName()
    {
        bool gemSystemNameIsValid = true;
        if (m_gemSystemName->lineEdit()->text().isEmpty())
        {
            gemSystemNameIsValid = false;
            m_gemSystemName->setErrorLabelText(tr("A gem system name is required."));
        }
        else
        {
            std::string gemSystemName = m_gemSystemName->lineEdit()->text().toStdString();
            _int64 count = std::count_if(
                gemSystemName.begin(),
                gemSystemName.end(),
                [](char c)
                {
                    return !(std::isalnum(c));
                });
            if (count > 0)
            {
                gemSystemNameIsValid = false;
                m_gemSystemName->setErrorLabelText(tr("A gem system name can only contain alphanumeric characters."));
            }
        }
        m_gemSystemName->setErrorLabelVisible(!gemSystemNameIsValid);
        return gemSystemNameIsValid;
    }

    bool CreateAGemScreen::ValidateLicenseName()
    {
        bool licenseNameIsValid = true;
        if (m_license->lineEdit()->text().isEmpty())
        {
            licenseNameIsValid = false;
            m_license->setErrorLabelText(tr("License details are required."));
        }

        m_license->setErrorLabelVisible(!licenseNameIsValid);
        return licenseNameIsValid;
    }

    bool CreateAGemScreen::ValidateGlobalGemTag()
    {
        bool globalGemTagIsValid = true;
        if (m_firstDropdown->comboBox()->currentIndex() == 0)
        {
            globalGemTagIsValid = false;
            m_firstDropdown->setErrorLabelText(tr("At least one global gem tag is required."));
        }
        m_firstDropdown->setErrorLabelVisible(!globalGemTagIsValid);
        return globalGemTagIsValid;
    }

    bool CreateAGemScreen::ValidateOptionalGemTags()
    {
        bool firstOptionalGemTagIsValid = true;
        bool secondOptionalGemTagIsValid = true;
        if (m_secondDropdown->comboBox()->currentIndex() != 0 && m_secondDropdown->comboBox()->currentIndex() ==
            m_firstDropdown->comboBox()->currentIndex())
        {
            firstOptionalGemTagIsValid = false;
            m_secondDropdown->setErrorLabelText(tr("This gem has already been selected"));
        }

        if (m_thirdDropdown->comboBox()->currentIndex() != 0 && (m_thirdDropdown->comboBox()->currentIndex() == m_secondDropdown->comboBox()->currentIndex() ||
            m_thirdDropdown->comboBox()->currentIndex() == m_firstDropdown->comboBox()->currentIndex()))
        {
            secondOptionalGemTagIsValid = false;
            m_thirdDropdown->setErrorLabelText(tr("This gem has already been selected"));
        }

        m_secondDropdown->setErrorLabelVisible(!firstOptionalGemTagIsValid);
        m_thirdDropdown->setErrorLabelVisible(!secondOptionalGemTagIsValid);
        return firstOptionalGemTagIsValid && secondOptionalGemTagIsValid;
    }


    bool CreateAGemScreen::ValidateCreatorName()
    {
        bool creatorNameIsValid = true;
        if (m_creatorName->lineEdit()->text().isEmpty())
        {
            creatorNameIsValid = false;
            m_creatorName->setErrorLabelText(tr("A creator's name is required."));
        }
        m_creatorName->setErrorLabelVisible(!creatorNameIsValid);
        return creatorNameIsValid;
    }

    bool CreateAGemScreen::ValidateRepositoryURL()
    {
        bool repositoryURLIsValid = true;
        if (m_repositoryURL->lineEdit()->text().isEmpty())
        {
            repositoryURLIsValid = false;
            m_repositoryURL->setErrorLabelText(tr("A repository URL is required."));
        }
        m_repositoryURL->setErrorLabelVisible(!repositoryURLIsValid);
        return repositoryURLIsValid;
    }

    void CreateAGemScreen::HandleBackButton()
    {
        if (m_tabWidget->currentIndex() == 2)
        {
            m_tabWidget->setCurrentIndex(1);
            m_nextButton->setText(tr("Next"));
        }
        else if (m_tabWidget->currentIndex() == 1)
        {
            m_backButton->setVisible(false);
            m_tabWidget->setCurrentIndex(0);
        }
        else if (m_tabWidget->currentIndex() == 0)
        {
            m_backButton->setVisible(false);
        }
    }

    void CreateAGemScreen::HandleNextButton()
    {
        if (m_tabWidget->currentIndex() == 0)
        {
            bool templateLocationIsValid = ValidateGemTemplateLocation();
            if (!m_formFolderRadioButton->isChecked())
            {
                m_backButton->setVisible(true);
                m_tabWidget->setCurrentIndex(1);
            }
            else if (templateLocationIsValid && m_formFolderRadioButton->isChecked())
            {
                m_backButton->setVisible(true);
                m_tabWidget->setCurrentIndex(1);
            }
        }
        else if (m_tabWidget->currentIndex() == 1)
        {
            bool displayNameIsValid = ValidateGemDisplayName();
            bool systemNameIsValid = ValidateGemSystemName();
            bool licenseNameIsValid = ValidateLicenseName();
            bool globalGemTagIsValid = ValidateGlobalGemTag();
            bool optionalGemTagsAreValid = ValidateOptionalGemTags();
            if (systemNameIsValid && licenseNameIsValid && globalGemTagIsValid && displayNameIsValid && optionalGemTagsAreValid)
            {
                m_createAGemInfo.m_displayName = m_gemDisplayName->lineEdit()->text();
                m_createAGemInfo.m_name = m_gemSystemName->lineEdit()->text();
                m_createAGemInfo.m_summary = m_gemSummary->lineEdit()->text();
                m_createAGemInfo.m_requirement = m_requirements->lineEdit()->text();
                m_createAGemInfo.m_licenseText = m_license->lineEdit()->text();
                m_createAGemInfo.m_licenseLink = m_licenseURL->lineEdit()->text();
                m_createAGemInfo.m_licenseText = m_license->lineEdit()->text();
                m_createAGemInfo.m_documentationLink = m_documentationURL->lineEdit()->text();
                m_createAGemInfo.m_path = m_gemLocation->lineEdit()->text();

                m_tabWidget->setCurrentIndex(2);
                m_nextButton->setText(tr("Create"));

            }
        }
        else if (m_tabWidget->currentIndex() == 2)
        {
            bool creatorNameIsValid = ValidateCreatorName();
            bool repoURLIsValid = ValidateRepositoryURL();
            if (creatorNameIsValid && repoURLIsValid)
            {
                m_createAGemInfo.m_creator = m_creatorName->lineEdit()->text();
                m_createAGemInfo.m_repoUri = m_repositoryURL->lineEdit()->text();
                //auto result = PythonBindingsInterface::Get()->CreateGem(m_createAGemInfo);
                emit CreateButtonPressed();
            }
        }
    }

    void CreateAGemScreen::UpdateNextButtonToCreate()
    {
        if (m_tabWidget->currentIndex() == 2)
        {
            m_nextButton->setText(tr("Create"));
        }
        else
        {
            m_nextButton->setText(tr("Next"));
        }
    }

    void CreateAGemScreen::AddDropdownActions(FormComboBoxWidget* dropdown)
    {
        QComboBox* comboBox = dropdown->comboBox();
        comboBox->addItem(tr(""));
        comboBox->addItem(tr("Achievements"));
        comboBox->addItem(tr("Animation"));
        comboBox->addItem(tr("Assets"));
        comboBox->addItem(tr("Audio"));
        comboBox->addItem(tr("Barrier"));
        comboBox->addItem(tr("Content"));
        comboBox->addItem(tr("Core"));
        comboBox->addItem(tr("Creation"));
        comboBox->addItem(tr("DCC"));
        comboBox->addItem(tr("Debug"));
        comboBox->addItem(tr("Design"));
        comboBox->addItem(tr("Digital"));
        comboBox->addItem(tr("Editor"));
        comboBox->addItem(tr("Environment"));
        comboBox->addItem(tr("Framework"));
        comboBox->addItem(tr("Gameplay"));
        comboBox->addItem(tr("Input"));
        comboBox->addItem(tr("Materials"));
        comboBox->addItem(tr("Multiplayer"));
        comboBox->addItem(tr("Network"));
        comboBox->addItem(tr("PBR"));
        comboBox->addItem(tr("Physics"));
        comboBox->addItem(tr("Profiler"));
        comboBox->addItem(tr("Rendering"));
        comboBox->addItem(tr("SDK"));
        comboBox->addItem(tr("Sample"));
        comboBox->addItem(tr("Scripting"));
        comboBox->addItem(tr("Simulation"));
        comboBox->addItem(tr("Slices"));
        comboBox->addItem(tr("Synergy"));
        comboBox->addItem(tr("Terrain"));
        comboBox->addItem(tr("Tools"));
        comboBox->addItem(tr("UI"));
        comboBox->addItem(tr("Unity"));
    }

} // namespace O3DE::ProjectManager

