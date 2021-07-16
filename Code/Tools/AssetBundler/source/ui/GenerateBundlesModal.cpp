/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/ui/GenerateBundlesModal.h>
#include <source/ui/ui_GenerateBundlesModal.h>

#include <source/ui/AssetListTabWidget.h>
#include <source/ui/NewFileDialog.h>
#include <source/utils/utils.h>

#include <AzCore/IO/FileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/AssetBundle/AssetBundleAPI.h>

#include <QMessageBox>

const char CustomBundleSettingsText[] = "Custom";

namespace AssetBundler
{
    GenerateBundlesModal::GenerateBundlesModal(
        QWidget* parent,
        const AZStd::string& assetListFileAbsolutePath,
        const AZStd::string& defaultBundleDirectory,
        const AZStd::string& defaultBundleSettingsDirectory,
        AssetListTabWidget* assetListTabWidget)
        : QDialog(parent)
        , m_assetListFileAbsolutePath(assetListFileAbsolutePath)
        , m_defaultBundleDirectory(defaultBundleDirectory)
        , m_defaultBundleSettingsDirectory(defaultBundleSettingsDirectory)
        , m_assetListTabWidget(assetListTabWidget)
    {
        m_ui.reset(new Ui::GenerateBundlesModal);
        m_ui->setupUi(this);

        m_platformName = AzToolsFramework::GetPlatformIdentifier(m_assetListFileAbsolutePath);

        // Selected Asset List
        m_ui->selectedAssetListPathLabel->setText(QString(m_assetListFileAbsolutePath.c_str()));

        // Platform
        m_ui->platformNameLabel->setText(QString(m_platformName.c_str()));

        // Bundle Output
        m_ui->outputBundlePathLineEdit->setReadOnly(true);
        connect(m_ui->outputBundlePathBrowseButton,
            &QPushButton::clicked,
            this,
            &GenerateBundlesModal::OnOutputBundleLocationBrowseButtonPressed);

        // Bundle Settings files
        m_ui->bundleSettingsFileLineEdit->setReadOnly(true);
        m_ui->bundleSettingsFileLineEdit->setText(tr(CustomBundleSettingsText));
        connect(m_ui->bundleSettingsFileBrowseButton,
            &QPushButton::clicked,
            this,
            &GenerateBundlesModal::OnBundleSettingsBrowseButtonPressed);
        connect(m_ui->bundleSettingsFileSaveButton,
            &QPushButton::clicked,
            this,
            &GenerateBundlesModal::OnBundleSettingsSaveButtonPressed);

        // Max Bundle Size
        m_ui->maxBundleSizeSpinBox->setRange(1, AzToolsFramework::MaxBundleSizeInMB);
        m_ui->maxBundleSizeSpinBox->setValue(AzToolsFramework::MaxBundleSizeInMB);
        m_ui->maxBundleSizeSpinBox->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);
        m_ui->maxBundleSizeSpinBox->setSuffix(" MB");
        connect(m_ui->maxBundleSizeSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &GenerateBundlesModal::OnMaxBundleSizeChanged);

        // Bundle Version
        m_ui->bundleVersionSpinBox->setRange(1, AzFramework::AssetBundleManifest::CurrentBundleVersion);
        m_ui->bundleVersionSpinBox->setValue(AzFramework::AssetBundleManifest::CurrentBundleVersion);
        m_ui->bundleVersionSpinBox->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);
        connect(m_ui->bundleVersionSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &GenerateBundlesModal::OnBundleVersionChanged);

        // Cancel and Generate Bundles buttons
        m_ui->generateBundlesButton->setEnabled(false);
        connect(m_ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_ui->generateBundlesButton, &QPushButton::clicked, this, &GenerateBundlesModal::OnGenerateBundlesButtonPressed);

        // In-memory Bundle Settings
        m_bundleSettings.m_assetFileInfoListPath = m_assetListFileAbsolutePath;
        m_bundleSettings.m_platform = m_platformName;
        m_bundleSettings.m_maxBundleSizeInMB = static_cast<AZ::u64>(m_ui->maxBundleSizeSpinBox->value());
        m_bundleSettings.m_bundleVersion = m_ui->bundleVersionSpinBox->value();
    }

    GenerateBundlesModal::~GenerateBundlesModal()
    {
        m_assetListTabWidget = nullptr;
    }

    void GenerateBundlesModal::OnOutputBundleLocationBrowseButtonPressed()
    {
        AZStd::string outputBundleAbsolutePath = NewFileDialog::OSNewFileDialog(
            this,
            AzToolsFramework::AssetBundleSettings::GetBundleFileExtension(),
            "Bundle",
            m_defaultBundleDirectory);

        if (outputBundleAbsolutePath.empty())
        {
            // User canceled out of the dialog
            return;
        }

        AzToolsFramework::RemovePlatformIdentifier(outputBundleAbsolutePath);
        AddPlatformIdentifier(outputBundleAbsolutePath, m_platformName);

        m_bundleSettings.m_bundleFilePath = outputBundleAbsolutePath;

        m_ui->outputBundlePathLineEdit->setText(outputBundleAbsolutePath.c_str());
        m_ui->generateBundlesButton->setEnabled(true);
    }

    void GenerateBundlesModal::OnBundleSettingsBrowseButtonPressed()
    {
        AZStd::string bundleSettingsAbsolutePath = NewFileDialog::OSNewFileDialog(
            this,
            AzToolsFramework::AssetBundleSettings::GetBundleSettingsFileExtension(),
            "Bundle Settings",
            m_defaultBundleSettingsDirectory);

        if (bundleSettingsAbsolutePath.empty())
        {
            // User canceled out of the dialog
            return;
        }

        // Read in the values from the Bundle Settings file
        bool loadResult = LoadBundleSettingsValues(bundleSettingsAbsolutePath);
        if (!loadResult)
        {
            return;
        }

        // Update the display name for our settings
        UpdateBundleSettingsDisplayName(bundleSettingsAbsolutePath);
    }

    bool GenerateBundlesModal::LoadBundleSettingsValues(const AZStd::string& absoluteBundleSettingsFilePath)
    {
        using namespace AzToolsFramework;

        auto outcome = AssetBundleSettings::Load(absoluteBundleSettingsFilePath);
        if (!outcome.IsSuccess())
        {
            AZ_Error("AssetBundler", false, outcome.GetError().c_str());
            return false;
        }

        // We don't want to overwrite all of the in-memory bundle settings, so we will just update a few
        AssetBundleSettings loadedSettings = outcome.TakeValue();

        m_bundleSettings.m_maxBundleSizeInMB = loadedSettings.m_maxBundleSizeInMB;
        m_ui->maxBundleSizeSpinBox->setValue(static_cast<int>(m_bundleSettings.m_maxBundleSizeInMB));

        m_bundleSettings.m_bundleVersion = loadedSettings.m_bundleVersion;
        m_ui->bundleVersionSpinBox->setValue(m_bundleSettings.m_bundleVersion);

        return true;
    }

    void GenerateBundlesModal::UpdateBundleSettingsDisplayName(const AZStd::string& absoluteBundleSettingsFilePath)
    {
        if (absoluteBundleSettingsFilePath.empty())
        {
            m_ui->bundleSettingsFileLineEdit->setText(tr(CustomBundleSettingsText));
            return;
        }

        AZStd::string bundleSettingsFileName = absoluteBundleSettingsFilePath;
        AzToolsFramework::RemovePlatformIdentifier(bundleSettingsFileName);
        AzFramework::StringFunc::Path::GetFileName(bundleSettingsFileName.c_str(), bundleSettingsFileName);
        m_ui->bundleSettingsFileLineEdit->setText(bundleSettingsFileName.c_str());
    }

    void GenerateBundlesModal::OnBundleSettingsSaveButtonPressed()
    {
        using namespace AzToolsFramework;

        // Ask the user where they want to save the Bundle Settings file
        AZStd::string bundleSettingsAbsolutePath = NewFileDialog::OSNewFileDialog(
            this,
            AssetBundleSettings::GetBundleSettingsFileExtension(),
            "Bundle Settings",
            m_defaultBundleSettingsDirectory);

        if (bundleSettingsAbsolutePath.empty())
        {
            // User canceled out of the operation
            return;
        }

        AzToolsFramework::RemovePlatformIdentifier(bundleSettingsAbsolutePath);
        AddPlatformIdentifier(bundleSettingsAbsolutePath, m_platformName);

        if (AZ::IO::FileIOBase::GetInstance()->Exists(bundleSettingsAbsolutePath.c_str()))
        {
            QString messageBoxText = QString(tr(
                "Bundle Settings ( %1 ) already exists on-disk. Saving the current settings will override the existing settings. \n\nDo you wish to continue?")).arg(bundleSettingsAbsolutePath.c_str());

            QMessageBox::StandardButton confirmDeleteFileResult =
                QMessageBox::question(this, QString(tr("Replace Existing Settings")), messageBoxText);
            if (confirmDeleteFileResult != QMessageBox::StandardButton::Yes)
            {
                // User canceled out of the operation
                return;
            }
        }

        bool saveResult = AssetBundleSettings::Save(m_bundleSettings, bundleSettingsAbsolutePath);
        if (!saveResult)
        {
            // Error has already been thrown
            return;
        }

        m_assetListTabWidget->AddScanPathToAssetBundlerSettings(AssetBundlingFileType::BundleSettingsFileType, bundleSettingsAbsolutePath);

        UpdateBundleSettingsDisplayName(bundleSettingsAbsolutePath);
    }

    void GenerateBundlesModal::OnMaxBundleSizeChanged()
    {
        m_bundleSettings.m_maxBundleSizeInMB = static_cast<AZ::u64>(m_ui->maxBundleSizeSpinBox->value());
        UpdateBundleSettingsDisplayName("");
    }

    void GenerateBundlesModal::OnBundleVersionChanged()
    {
        m_bundleSettings.m_bundleVersion = m_ui->bundleVersionSpinBox->value();
        UpdateBundleSettingsDisplayName("");
    }

    void GenerateBundlesModal::OnGenerateBundlesButtonPressed()
    {
        using namespace AzToolsFramework;

        if (AZ::IO::FileIOBase::GetInstance()->Exists(m_bundleSettings.m_bundleFilePath.c_str()))
        {
            QString messageBoxText = QString(tr(
                "Asset Bundle ( %1 ) already exists on-disk. Generating a new Bundle will override the existing Bundle. \n\nDo you wish to permanently delete the existing Bundle?")).arg(m_bundleSettings.m_bundleFilePath.c_str());

            QMessageBox::StandardButton confirmDeleteFileResult =
                QMessageBox::question(this, QString(tr("Replace Existing Bundle")), messageBoxText);
            if (confirmDeleteFileResult != QMessageBox::StandardButton::Yes)
            {
                // User canceled out of the operation
                return;
            }
        }

        bool result = false;
        AssetBundleCommandsBus::BroadcastResult(result, &AssetBundleCommandsBus::Events::CreateAssetBundle, m_bundleSettings);

        // This operation will take long enough that the OS will throw up its own wait cursor,
        // but there is a slight delay where the UI is unresponsive but the cursor hasn't changed.
        // We are changing the cursor manually to avoid that. 
        setCursor(Qt::WaitCursor);

        emit QDialog::accept();

        if (result)
        {
            m_assetListTabWidget->AddScanPathToAssetBundlerSettings(
                AssetBundlingFileType::BundleFileType,
                m_bundleSettings.m_bundleFilePath);

            // The watched files list was updated after the files were created, so we need to force-reload them
            m_assetListTabWidget->GetGUIApplicationManager()->UpdateFiles(
                AssetBundlingFileType::BundleFileType,
                { m_bundleSettings.m_bundleFilePath });
        }

        AZStd::vector<AZStd::string> generatedFilePaths = { m_bundleSettings.m_bundleFilePath };
        NewFileDialog::FileGenerationResultMessageBox(this, generatedFilePaths, !result);

        unsetCursor();
    }

} // namespace AssetBundler

#include <source/ui/moc_GenerateBundlesModal.cpp>
