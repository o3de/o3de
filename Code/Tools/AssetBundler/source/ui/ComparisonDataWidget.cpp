/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/ui/ComparisonDataWidget.h>
#include <source/ui/ui_ComparisonDataWidget.h>

#include <source/ui/NewFileDialog.h>

#include <QComboBox>
#include <QStringList>

namespace AssetBundler
{
    //////////////////////////////////////////////////////////////////////////////////////////////////
    // ComparisonDataWidget
    //////////////////////////////////////////////////////////////////////////////////////////////////

    const QStringList ComparisonTypeStringList = { "Default", "Delta", "Union", "Intersection", "Complement", "Wildcard", "Regex" };

    ComparisonDataWidget::ComparisonDataWidget(
        AZStd::shared_ptr<AzToolsFramework::AssetFileInfoListComparison> comparisonList,
        size_t comparisonDataIndex,
        const AZStd::string& defaultAssetListFileDirectory,
        QWidget* parent)
        : QWidget(parent)
        , m_comparisonDataIndex(comparisonDataIndex)
        , m_defaultAssetListFileDirectory(defaultAssetListFileDirectory)
    {
        m_ui.reset(new Ui::ComparisonDataWidget);
        m_ui->setupUi(this);

        m_comparisonList = comparisonList;

        if (!IsComparisonDataIndexValid())
        {
            AZ_Error("AssetBundler", false,
                "ComparisonData index ( %u ) is out of bounds. ComparisonData cannot be displayed.", m_comparisonDataIndex);
            return;
        }

        // Due to initialization order, we need to hard-code this or else the value will be overwritten once this
        // ComparisonDataWidget is added to the ComparisonDataCard, and the Card.qss file is applied
        QString lineEditStyle("background-color: #CCCCCC;");
        m_ui->nameLineEdit->setStyleSheet(lineEditStyle);
        m_ui->firstInputLineEdit->setStyleSheet(lineEditStyle);
        m_ui->secondInputLineEdit->setStyleSheet(lineEditStyle);
        m_ui->filePatternLineEdit->setStyleSheet(lineEditStyle);

        m_ui->firstInputLineEdit->setReadOnly(true);
        m_ui->secondInputLineEdit->setReadOnly(true);

        SetAllDisplayValues(m_comparisonList->GetComparisonList()[m_comparisonDataIndex]);

        MouseWheelEventFilter* mouseWheelEventFilter = new MouseWheelEventFilter(this);

        connect(m_ui->nameLineEdit, &QLineEdit::textEdited, this, &ComparisonDataWidget::OnNameLineEditChanged);

        m_ui->comparisonTypeComboBox->installEventFilter(mouseWheelEventFilter);
        connect(m_ui->comparisonTypeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &ComparisonDataWidget::OnComparisonTypeComboBoxChanged);

        m_ui->firstInputComboBox->installEventFilter(mouseWheelEventFilter);
        connect(m_ui->firstInputComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &ComparisonDataWidget::OnFirstInputComboBoxChanged);
        connect(m_ui->firstInputBrowseButton,
            &QPushButton::pressed,
            this,
            &ComparisonDataWidget::OnFirstInputBrowseButtonPressed);

        m_ui->secondInputComboBox->installEventFilter(mouseWheelEventFilter);
        connect(m_ui->secondInputComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &ComparisonDataWidget::OnSecondInputComboBoxChanged);
        connect(m_ui->secondInputBrowseButton, &QPushButton::pressed, this, &ComparisonDataWidget::OnSecondInputBrowseButtonPressed);

        connect(m_ui->filePatternLineEdit, &QLineEdit::textEdited, this, &ComparisonDataWidget::OnFilePatternLineEditChanged);
    }

    void ComparisonDataWidget::UpdateListOfTokenNames()
    {
        using namespace AzToolsFramework;

        if (!IsComparisonDataIndexValid())
        {
            return;
        }

        m_inputTokenNameList.clear();
        m_ui->firstInputComboBox->clear();
        m_ui->secondInputComboBox->clear();

        // Store info about the current FirstInput and SecondInput values so we may auto-select them later
        auto allComparisonDataSteps = m_comparisonList->GetComparisonList();
        AZStd::string selectedFirstInput = allComparisonDataSteps.at(m_comparisonDataIndex).m_firstInput;
        int selectedFirstInputIndex = 0;
        AZStd::string selectedSecondInput = allComparisonDataSteps.at(m_comparisonDataIndex).m_secondInput;
        int selectedSecondInputIndex = 0;

        // Build list of all Token Names that have come before the current Comparison Step
        m_inputTokenNameList.append(tr("Choose Asset List..."));
        int currentComboBoxIndex = 1;

        AZStd::string tokenName;
        for (size_t i = 0; i < m_comparisonDataIndex; ++i)
        {
            tokenName = allComparisonDataSteps.at(i).m_output;
            if (!AssetFileInfoListComparison::IsTokenFile(tokenName))
            {
                continue;
            }

            if (tokenName == selectedFirstInput)
            {
                selectedFirstInputIndex = currentComboBoxIndex;
            }
            if (tokenName == selectedSecondInput)
            {
                selectedSecondInputIndex = currentComboBoxIndex;
            }

            m_inputTokenNameList.append(RemoveTokenCharFromString(tokenName));
            ++currentComboBoxIndex;
        }

        // Update display with list of Token Names, and select current Token Name in both input combo boxes
        m_ui->firstInputComboBox->insertItems(0, m_inputTokenNameList);
        m_ui->firstInputComboBox->setCurrentIndex(selectedFirstInputIndex);
        m_isFirstInputFileNameVisible = selectedFirstInputIndex == 0;
        SetFirstInputFileVisibility(m_isFirstInputFileNameVisible);

        m_ui->secondInputComboBox->insertItems(0, m_inputTokenNameList);
        m_ui->secondInputComboBox->setCurrentIndex(selectedSecondInputIndex);
        m_isSecondInputFileNameVisible = selectedSecondInputIndex == 0;
        SetSecondInputFileVisibility(m_isSecondInputFileNameVisible);
    }

    void ComparisonDataWidget::SetAllDisplayValues(const AzToolsFramework::AssetFileInfoListComparison::ComparisonData& comparisonData)
    {
        using namespace AzToolsFramework;

        // Name (Token value)
        m_ui->nameLineEdit->setText(RemoveTokenCharFromString(comparisonData.m_output));

        // Comparison Type
        InitComparisonTypeComboBox(comparisonData);

        // Inputs
        UpdateListOfTokenNames();
        m_ui->firstInputLineEdit->setText(QString(comparisonData.m_cachedFirstInputPath.c_str()));
        m_ui->secondInputLineEdit->setText(QString(comparisonData.m_cachedSecondInputPath.c_str()));

        // Update fields that are not always visible
        UpdateOnComparisonTypeChanged(comparisonData.m_filePatternType != AssetFileInfoListComparison::FilePatternType::Default);
    }

    void ComparisonDataWidget::OnNameLineEditChanged()
    {
        if (!IsComparisonDataIndexValid())
        {
            return;
        }

        AZStd::string tokenName = m_ui->nameLineEdit->text().toUtf8().data();
        AzToolsFramework::AssetFileInfoListComparison::FormatOutputToken(tokenName);
        m_comparisonList->SetOutput(m_comparisonDataIndex, tokenName);
        emit comparisonDataChanged();
        emit comparisonDataTokenNameChanged(m_comparisonDataIndex);
    }

    void ComparisonDataWidget::UpdateOnComparisonTypeChanged(bool isFilePatternOperation)
    {
        using namespace AzToolsFramework;

        if (!IsComparisonDataIndexValid())
        {
            return;
        }

        AssetFileInfoListComparison::ComparisonData comparisonData = m_comparisonList->GetComparisonList()[m_comparisonDataIndex];
        m_ui->inputBLabel->setVisible(!isFilePatternOperation);
        m_ui->secondInputComboBox->setVisible(!isFilePatternOperation);
        SetSecondInputFileVisibility(!isFilePatternOperation && m_isSecondInputFileNameVisible); 

        m_ui->filePatternLabel->setVisible(isFilePatternOperation);
        m_ui->filePatternLineEdit->setVisible(isFilePatternOperation);
        m_ui->filePatternLineEdit->setText(comparisonData.m_filePattern.c_str());
    }

    void ComparisonDataWidget::InitComparisonTypeComboBox(
        const AzToolsFramework::AssetFileInfoListComparison::ComparisonData& comparisonData)
    {
        using namespace AzToolsFramework;

        m_ui->comparisonTypeComboBox->insertItems(0, ComparisonTypeStringList);
        int initialSelectionIndex = ComparisonTypeIndex::Default;

        if (comparisonData.m_filePatternType != AssetFileInfoListComparison::FilePatternType::Default
            && comparisonData.m_comparisonType == AssetFileInfoListComparison::ComparisonType::FilePattern)
        {
            if (comparisonData.m_filePatternType == AssetFileInfoListComparison::FilePatternType::Wildcard)
            {
                initialSelectionIndex = ComparisonTypeIndex::Wildcard;
            }
            else
            {
                initialSelectionIndex = ComparisonTypeIndex::Regex;
            }
        }
        else
        {
            switch (comparisonData.m_comparisonType)
            {
            case AssetFileInfoListComparison::ComparisonType::Default:
                break;
            case AssetFileInfoListComparison::ComparisonType::Delta:
                initialSelectionIndex = ComparisonTypeIndex::Delta;
                break;
            case AssetFileInfoListComparison::ComparisonType::Union:
                initialSelectionIndex = ComparisonTypeIndex::Union;
                break;
            case AssetFileInfoListComparison::ComparisonType::Intersection:
                initialSelectionIndex = ComparisonTypeIndex::Intersection;
                break;
            case AssetFileInfoListComparison::ComparisonType::Complement:
                initialSelectionIndex = ComparisonTypeIndex::Complement;
                break;
            default:
                AZ_Warning("AssetBundler", false,
                    "ComparisonType ( %u ) is not supported in the Asset Bundler", comparisonData.m_comparisonType);
            }
        }

        m_ui->comparisonTypeComboBox->setCurrentIndex(initialSelectionIndex);
    }

    void ComparisonDataWidget::OnComparisonTypeComboBoxChanged(int index)
    {
        using namespace AzToolsFramework;

        if (!IsComparisonDataIndexValid())
        {
            return;
        }

        bool isFilePattern = false;

        switch (index)
        {
        case ComparisonTypeIndex::Default:
            m_comparisonList->SetComparisonType(m_comparisonDataIndex, AssetFileInfoListComparison::ComparisonType::Default);
            break;
        case ComparisonTypeIndex::Delta:
            m_comparisonList->SetComparisonType(m_comparisonDataIndex, AssetFileInfoListComparison::ComparisonType::Delta);
            break;
        case ComparisonTypeIndex::Union:
            m_comparisonList->SetComparisonType(m_comparisonDataIndex, AssetFileInfoListComparison::ComparisonType::Union);
            break;
        case ComparisonTypeIndex::Intersection:
            m_comparisonList->SetComparisonType(m_comparisonDataIndex, AssetFileInfoListComparison::ComparisonType::Intersection);
            break;
        case ComparisonTypeIndex::Complement:
            m_comparisonList->SetComparisonType(m_comparisonDataIndex, AssetFileInfoListComparison::ComparisonType::Complement);
            break;
        case ComparisonTypeIndex::Wildcard:
            m_comparisonList->SetComparisonType(m_comparisonDataIndex, AssetFileInfoListComparison::ComparisonType::FilePattern);
            m_comparisonList->SetFilePatternType(m_comparisonDataIndex, AssetFileInfoListComparison::FilePatternType::Wildcard);
            isFilePattern = true;
            break;
        case ComparisonTypeIndex::Regex:
            m_comparisonList->SetComparisonType(m_comparisonDataIndex, AssetFileInfoListComparison::ComparisonType::FilePattern);
            m_comparisonList->SetFilePatternType(m_comparisonDataIndex, AssetFileInfoListComparison::FilePatternType::Regex);
            isFilePattern = true;
            break;
        }

        UpdateOnComparisonTypeChanged(isFilePattern);
        emit comparisonDataChanged();
    }

    void ComparisonDataWidget::OnFilePatternLineEditChanged()
    {
        if (!IsComparisonDataIndexValid())
        {
            return;
        }

        m_comparisonList->SetFilePattern(m_comparisonDataIndex, m_ui->filePatternLineEdit->text().toUtf8().data());
        emit comparisonDataChanged();
    }

    void ComparisonDataWidget::OnFirstInputComboBoxChanged(int index)
    {
        AZStd::string firstInputValue;

        if (!IsComparisonDataIndexValid())
        {
            return;
        }

        m_isFirstInputFileNameVisible = index == 0;

        if (!m_isFirstInputFileNameVisible)
        {
            // The 0th index is the default value, which translates to an empty token string.
            firstInputValue = m_ui->firstInputComboBox->currentText().toUtf8().data();
            AzToolsFramework::AssetFileInfoListComparison::FormatOutputToken(firstInputValue);
        }

        SetFirstInputFileVisibility(m_isFirstInputFileNameVisible);

        m_comparisonList->SetFirstInput(m_comparisonDataIndex, firstInputValue);
        emit comparisonDataChanged();
    }

    void ComparisonDataWidget::SetFirstInputFileVisibility(bool isVisible)
    {
        m_ui->firstInputLineEdit->setVisible(isVisible);
        m_ui->firstInputBrowseButton->setVisible(isVisible);
    }

    void ComparisonDataWidget::OnFirstInputBrowseButtonPressed()
    {
        if (!IsComparisonDataIndexValid())
        {
            return;
        }

        QString absoluteFilePath = BrowseButtonPressed();
        if (absoluteFilePath.isEmpty())
        {
            // User canceled out of the dialog
            return;
        }

        m_ui->firstInputLineEdit->setText(absoluteFilePath);
        m_comparisonList->SetCachedFirstInputPath(m_comparisonDataIndex, absoluteFilePath.toUtf8().data());
    }

    void ComparisonDataWidget::OnSecondInputComboBoxChanged(int index)
    {
        if (!IsComparisonDataIndexValid())
        {
            return;
        }

        AZStd::string secondInputValue;

        m_isSecondInputFileNameVisible = index == 0;

        if (!m_isSecondInputFileNameVisible)
        {
            // The 0th index is the default value, which translates to an empty token string.
            secondInputValue = m_ui->secondInputComboBox->currentText().toUtf8().data();
            AzToolsFramework::AssetFileInfoListComparison::FormatOutputToken(secondInputValue);
        }

        SetSecondInputFileVisibility(m_isSecondInputFileNameVisible);

        m_comparisonList->SetSecondInput(m_comparisonDataIndex, secondInputValue);
        emit comparisonDataChanged();
    }

    void ComparisonDataWidget::SetSecondInputFileVisibility(bool isVisible)
    {
        m_ui->secondInputLineEdit->setVisible(isVisible);
        m_ui->secondInputBrowseButton->setVisible(isVisible);
    }

    void ComparisonDataWidget::OnSecondInputBrowseButtonPressed()
    {
        if (!IsComparisonDataIndexValid())
        {
            return;
        }

        QString absoluteFilePath = BrowseButtonPressed();
        if (absoluteFilePath.isEmpty())
        {
            // User canceled out of the dialog
            return;
        }

        m_ui->secondInputLineEdit->setText(absoluteFilePath);
        m_comparisonList->SetCachedSecondInputPath(m_comparisonDataIndex, absoluteFilePath.toUtf8().data());
    }

    QString ComparisonDataWidget::BrowseButtonPressed()
    {
        AZStd::string selectedPath = NewFileDialog::OSNewFileDialog(
            this,
            AzToolsFramework::AssetSeedManager::GetAssetListFileExtension(),
            "Asset List",
            m_defaultAssetListFileDirectory);
        AzToolsFramework::RemovePlatformIdentifier(selectedPath);
        return selectedPath.c_str();
    }

    bool ComparisonDataWidget::IsComparisonDataIndexValid()
    {
        return m_comparisonList->GetComparisonList().size() > m_comparisonDataIndex;
    }

    QString ComparisonDataWidget::RemoveTokenCharFromString(const AZStd::string& tokenName)
    {
        QString displayName = tokenName.c_str();
        if (displayName.startsWith(AzToolsFramework::AssetFileInfoListComparison::GetTokenIdentifier()))
        {
            displayName.remove(0, 1);
        }

        return displayName;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // MouseWheelEventFilter
    //////////////////////////////////////////////////////////////////////////////////////////////////

    bool MouseWheelEventFilter::eventFilter(QObject* obj, QEvent* ev)
    {
        if (ev->type() == QEvent::Wheel)
        {
            return true;
        }

        return QObject::eventFilter(obj, ev);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // ComparisonDataCard
    //////////////////////////////////////////////////////////////////////////////////////////////////

    ComparisonDataCard::ComparisonDataCard(
        AZStd::shared_ptr<AzToolsFramework::AssetFileInfoListComparison> comparisonList,
        size_t comparisonDataIndex,
        const AZStd::string& defaultAssetListFileDirectory,
        QWidget* parent)
        : AzQtComponents::Card(parent)
    {
        m_comparisonDataWidget = new ComparisonDataWidget(comparisonList, comparisonDataIndex, defaultAssetListFileDirectory, this);
        setContentWidget(m_comparisonDataWidget);

        connect(this, &AzQtComponents::Card::contextMenuRequested, this, &ComparisonDataCard::OnContextMenuRequested);
    }

    void ComparisonDataCard::OnContextMenuRequested(const QPoint& position)
    {
        emit comparisonDataCardContextMenuRequested(m_comparisonDataWidget->GetComparisonDataIndex(), position);
    }

} // namespace AssetBundler

#include <source/ui/moc_ComparisonDataWidget.cpp>
