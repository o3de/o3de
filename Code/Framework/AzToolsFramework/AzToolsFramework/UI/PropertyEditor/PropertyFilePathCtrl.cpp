/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PropertyFilePathCtrl.h"

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
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

        // Whenever the browse edit has text, the clear button will be automatically shown
        // since we enabled it with setClearButtonEnabled.
        // When it is pressed, it clears the browse edit text for us, but we need to do
        // some extra logic as well to clear our internal state and trigger our value changed.
        QToolButton* clearButton = AzQtComponents::LineEdit::getClearButton(m_browseEdit->lineEdit());
        AZ_Assert(clearButton, "Clear button missing from BrowseEdit");
        QObject::connect(clearButton, &QToolButton::clicked, this, &PropertyFilePathCtrl::OnClearButtonClicked);

        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_browseEdit);

        setLayout(layout);

        setFocusProxy(m_browseEdit->lineEdit());
        setFocusPolicy(m_browseEdit->lineEdit()->focusPolicy());
    };

    void PropertyFilePathCtrl::SetFilePath(const AZ::IO::Path& filePath)
    {
        m_currentFilePath = filePath;
        m_browseEdit->setText(filePath.c_str());

        Q_EMIT FilePathChanged();
    }

    const AZ::IO::Path& PropertyFilePathCtrl::GetFilePath() const
    {
        return m_currentFilePath;
    }

    void PropertyFilePathCtrl::SetFilter(const QString& filter)
    {
        m_filter = filter;
    }

    void PropertyFilePathCtrl::SetProductExtension(AZStd::string extension)
    {
        m_productExtension = AZStd::move(extension);
    }

    void PropertyFilePathCtrl::SetDefaultFileName(AZ::IO::Path fileName)
    {
        m_defaultFileName = AZStd::move(fileName);
    }

    void PropertyFilePathCtrl::OnOpenButtonClicked()
    {
        // Retrieve what the pre-selected file path should be for the file dialog
        QString preselectedFilePath = GetPreselectedFilePath();

        // Popup a native file dialog to choose a new or existing file
        // Using the AzQtComponents::FileDialog::GetSaveFileName helper will protect the user from entering
        // a filename with invalid characters for the AP
        QString newFilePath =
            AzQtComponents::FileDialog::GetSaveFileName(this, QObject::tr("Open/Save File"), preselectedFilePath, m_filter);

        // Try to set the new file path (if one was selected)
        // This will handle file paths outside of the safe asset folders
        HandleNewFilePathSelected(newFilePath);
    }

    QString PropertyFilePathCtrl::GetPreselectedFilePath() const
    {
        QString preselectedFilePath;

        // If we don't have any output file set yet, then generate a default
        // option for the user
        if (m_currentFilePath.empty())
        {
            AZ::IO::FixedMaxPath defaultPath = AZ::Utils::GetProjectPath();

            // If a default filename has been specified, append it to
            // the project path to be the default path
            if (!m_defaultFileName.empty())
            {
                defaultPath /= m_defaultFileName;
            }

            preselectedFilePath = QString::fromUtf8(defaultPath.c_str(), static_cast<int>(defaultPath.Native().size()));
        }
        // Otherwise, we need to find the proper absolute path based on the relative path
        // that is stored
        else
        {
            // The GetFullSourcePathFromRelativeProductPath API only works on product paths,
            // so we need to append the product extension to try and find it
            AZStd::string relativePath = m_currentFilePath.Native() + m_productExtension;

            AZStd::string fullPath;
            bool fullPathIsValid = false;
            AssetSystemRequestBus::BroadcastResult(
                fullPathIsValid, &AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, relativePath, fullPath);

            // The full source path asset exists on disk, so use that as
            // the pre-selected file when the user opens the file picker dialog
            if (fullPathIsValid)
            {
                preselectedFilePath = QString::fromUtf8(fullPath.c_str(), static_cast<int>(fullPath.size()));
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
                    return QString();
                }

                // Find an asset safe folder that already has the existing parent-path that
                // would satisfy the relative path currently stored.
                //
                // Example: m_currentFilePath has a value of "my/sub/folder/image.png"
                // It will keep looking until it finds an asset safe folder that has
                // a matching sub-directory structure that exists:
                //      <ASSET_SAFE_FOLDER>/my/sub/folder
                for (AZ::IO::Path candidateFilePath : assetSafeFolders)
                {
                    candidateFilePath /= m_currentFilePath;

                    if (AZ::IO::FixedMaxPath parentCandidatePath = candidateFilePath.ParentPath();
                        AZ::IO::SystemFile::IsDirectory(parentCandidatePath.c_str()))
                    {
                        preselectedFilePath =
                            QString::fromUtf8(candidateFilePath.c_str(), static_cast<int>(candidateFilePath.Native().size()));
                        break;
                    }
                }
            }
        }

        return preselectedFilePath;
    }

    void PropertyFilePathCtrl::HandleNewFilePathSelected(const QString& newFilePath)
    {
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
        bool relativePathIsValid = false;
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
            constexpr const char* errorMessage =
                "You can only save assets to either your game project folder or the Gems folder. Update the location and try "
                "again.\n\n"
                "You can also review and update your save locations in the Registry/AssetProcessorPlatformConfig.setreg file or your Gem's "
                "Registry/assetprocessor_settings.setreg file.";
            AZ_Error("PropertyFilePathCtrl", false, errorMessage);

            QMessageBox* errorDialog = new QMessageBox(GetActiveWindow());
            errorDialog->setIcon(QMessageBox::Icon::Warning);
            errorDialog->setTextFormat(Qt::RichText);
            errorDialog->setWindowTitle(QObject::tr("Invalid save location"));
            errorDialog->setText(QObject::tr(errorMessage));
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
