/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PropertyFilePathCtrl.h"

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QToolButton>
AZ_POP_DISABLE_WARNING

#include <AzCore/Utils/Utils.h>

#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

namespace AzToolsFramework
{
    PropertyFilePathCtrl::PropertyFilePathCtrl(QWidget* parent)
        : QWidget(parent)
    {
        QHBoxLayout* layout = new QHBoxLayout(this);

        m_browseEdit = new AzQtComponents::BrowseEdit(this);
        m_browseEdit->lineEdit()->setFocusPolicy(Qt::StrongFocus);
        m_browseEdit->setLineEditReadOnly(true);
        m_browseEdit->setClearButtonEnabled(true);
        m_browseEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QObject::connect(
            m_browseEdit, &AzQtComponents::BrowseEdit::attachedButtonTriggered, this, &PropertyFilePathCtrl::OnOpenButtonClicked);

        QToolButton* clearButton = AzQtComponents::LineEdit::getClearButton(m_browseEdit->lineEdit());
        AZ_Assert(clearButton, "Clear button missing from BrowseEdit");
        QObject::connect(clearButton, &QToolButton::clicked, this, &PropertyFilePathCtrl::OnClearButtonClicked);

        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);
        layout->addWidget(m_browseEdit);

        setLayout(layout);

        setFocusProxy(m_browseEdit->lineEdit());
        setFocusPolicy(m_browseEdit->lineEdit()->focusPolicy());
    };

    void PropertyFilePathCtrl::SetFilePath(AZ::IO::Path filePath)
    {
        m_currentFilePath = filePath;
        m_browseEdit->setText(filePath.c_str());

        Q_EMIT FilePathChanged();
    }

    AZ::IO::Path PropertyFilePathCtrl::GetFilePath() const
    {
        return m_currentFilePath;
    }

    void PropertyFilePathCtrl::SetFilter(const QString& filter)
    {
        m_filter = filter;
    }

    void PropertyFilePathCtrl::SetProductExtension(const AZStd::string& extension)
    {
        m_productExtension = extension;
    }

    void PropertyFilePathCtrl::SetDefaultFileName(const AZStd::string& fileName)
    {
        m_defaultFileName = fileName;
    }

    void PropertyFilePathCtrl::OnOpenButtonClicked()
    {
        QString preselectedFilePath;

        // If we don't have any output file set yet, then generate a default
        // option for the user
        if (m_currentFilePath.empty())
        {
            AZ::IO::Path defaultPath = AZ::IO::Path(AZ::Utils::GetProjectPath());

            // If a default filename has been specified, append it to
            // the project path to be the default path
            if (!m_defaultFileName.empty())
            {
                defaultPath /= AZ::IO::Path(m_defaultFileName);
            }

            preselectedFilePath = defaultPath.c_str();
        }
        // Otherwise, we need to find the proper absolute path based on the relative path
        // that is stored
        else
        {
            // The GetFullSourcePathFromRelativeProductPath API only works on product paths,
            // so we need to append the product extension to try and find it
            AZStd::string relativePath = m_currentFilePath.String() + m_productExtension;

            AZStd::string fullPath;
            bool fullPathIsValid;
            AssetSystemRequestBus::BroadcastResult(
                fullPathIsValid, &AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, relativePath, fullPath);

            // The full source path asset exists on disk, so use that as
            // the pre-selected file when the user opens the file picker dialog
            if (fullPathIsValid)
            {
                preselectedFilePath = fullPath.c_str();
            }
            // GetFullSourcePathFromRelativeProductPath failed so the file doesn't exist on disk yet
            // So we need to find it by searching in the asset safe folders
            else
            {
                bool assetSafeFoldersRetrieved = false;
                AZStd::vector<AZStd::string> assetSafeFolders;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                    assetSafeFoldersRetrieved, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetSafeFolders, assetSafeFolders);

                if (!assetSafeFoldersRetrieved)
                {
                    AZ_Error("PropertyFilePathCtrl", false, "Could not acquire a list of asset safe folders from the database.");
                    return;
                }

                // Find an asset safe folder that already has existing sub-path that
                // would satisfy the relative path for the file
                for (const AZStd::string& assetSafeFolder : assetSafeFolders)
                {
                    AZ::IO::Path assetSafeFolderPath(assetSafeFolder);
                    AZ::IO::Path relativeFolderPath = assetSafeFolderPath;

                    if (m_currentFilePath.HasParentPath())
                    {
                        relativeFolderPath /= m_currentFilePath.ParentPath();
                    }

                    QDir currentFilePathDirectory(relativeFolderPath.c_str());
                    if (currentFilePathDirectory.exists())
                    {
                        auto absoluteFilePath = (assetSafeFolderPath / m_currentFilePath);
                        preselectedFilePath = absoluteFilePath.c_str();
                        break;
                    }
                }
            }
        }

        // Popup a native file dialog to choose a new or existing file
        // Using the AzQtComponents::FileDialog::GetSaveFileName helper will protect the user from entering
        // a filename with invalid characters for the AP
        QString newFilePath =
            AzQtComponents::FileDialog::GetSaveFileName(this, QObject::tr("Open/Save File"), preselectedFilePath, m_filter);

        // If the newFilePath is empty, then the dialog was cancelled,
        // so don't process any further.
        if (newFilePath.isEmpty())
        {
            return;
        }

        // Generate a relative path given the absolute path that was picked.
        // If the absolute path wasn't inside an asset safe folder, then
        // the call to GenerateRelativeSourcePath will fail.
        AZStd::string absolutePath(newFilePath.toUtf8().constData());
        AZStd::string relativePath, rootFilePath;
        bool relativePathIsValid;
        AssetSystemRequestBus::BroadcastResult(
            relativePathIsValid, &AssetSystemRequestBus::Events::GenerateRelativeSourcePath, absolutePath, relativePath, rootFilePath);

        // We have a valid relative path, so update our entry.
        if (relativePathIsValid)
        {
            SetFilePath(relativePath);
        }
        // The user chose a path outside of an asset safe folder,
        // so give them a warning dialog explaining why it can't be saved there
        else
        {
            AZStd::string errorMessage =
                "You can only save assets to either your game project folder or the Gems folder. Update the location and try "
                "again.\n\n"
                "You can also review and update your save locations in the AssetProcessorPlatformConfig.ini file.";
            AZ_Error("PropertyFilePathCtrl", false, errorMessage.c_str());

            QMessageBox* errorDialog = new QMessageBox(GetActiveWindow());
            errorDialog->setIcon(QMessageBox::Icon::Warning);
            errorDialog->setTextFormat(Qt::RichText);
            errorDialog->setWindowTitle(QObject::tr("Invalid save location"));
            errorDialog->setText(QObject::tr(errorMessage.c_str()));
            errorDialog->setStandardButtons(QMessageBox::Cancel | QMessageBox::Retry);
            errorDialog->setDefaultButton(QMessageBox::Retry);

            QObject::connect(
                errorDialog, &QDialog::finished,
                [this, errorDialog](int resultCode)
                {
                    // If the user wants to retry, re-open the file dialog
                    // It will automatically be re-opened relative to the default
                    // asset safe folder (typically the project root)
                    if (resultCode == QMessageBox::Retry)
                    {
                        OnOpenButtonClicked();
                    }

                    // Make sure our error dialog gets deleted after it is dismissed.
                    errorDialog->deleteLater();
                });

            errorDialog->open();
        }
    }

    void PropertyFilePathCtrl::OnClearButtonClicked()
    {
        m_currentFilePath.clear();
        m_browseEdit->lineEdit()->clearFocus();

        Q_EMIT FilePathChanged();
    }

    void PropertyFilePathHandler::ConsumeAttribute(
        PropertyFilePathCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::SourceAssetFilterPattern)
        {
            AZStd::string filter;
            if (attrValue->Read<AZStd::string>(filter))
            {
                GUI->SetFilter(filter.c_str());
            }
        }
        else if (attrib == AZ::Edit::Attributes::DefaultAsset)
        {
            AZStd::string defaultAsset;
            if (attrValue->Read<AZStd::string>(defaultAsset))
            {
                GUI->SetDefaultFileName(defaultAsset);
            }
        }
    }

    AZ::u32 PropertyFilePathHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("FilePath");
    }

    bool PropertyFilePathHandler::IsDefaultHandler() const
    {
        return true;
    }

    QWidget* PropertyFilePathHandler::CreateGUI(QWidget* parent)
    {
        PropertyFilePathCtrl* newCtrl = aznew PropertyFilePathCtrl(parent);

        QObject::connect(
            newCtrl, &PropertyFilePathCtrl::FilePathChanged, this,
            [newCtrl]()
            {
                PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::RequestWrite, newCtrl);
                PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
            });

        return newCtrl;
    }

    void PropertyFilePathHandler::WriteGUIValuesIntoProperty(
        [[maybe_unused]] size_t index, PropertyFilePathCtrl* GUI, property_t& instance, [[maybe_unused]] InstanceDataNode* node)
    {
        AZ::IO::Path filePath = GUI->GetFilePath();
        instance = static_cast<property_t>(filePath);
    }

    bool PropertyFilePathHandler::ReadValuesIntoGUI(
        [[maybe_unused]] size_t index, PropertyFilePathCtrl* GUI, const property_t& instance, [[maybe_unused]] InstanceDataNode* node)
    {
        GUI->blockSignals(true);

        GUI->SetFilePath(instance);

        GUI->blockSignals(false);

        return false;
    }

    void RegisterFilePathHandler()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::RegisterPropertyType, aznew PropertyFilePathHandler());
    }
} // namespace AzToolsFramework

#include "UI/PropertyEditor/moc_PropertyFilePathCtrl.cpp"
