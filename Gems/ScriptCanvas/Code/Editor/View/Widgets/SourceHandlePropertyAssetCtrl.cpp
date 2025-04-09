/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Components/EditorUtils.h>
#include <Editor/View/Widgets/SourceHandlePropertyAssetCtrl.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/UI/PropertyEditor/Model/AssetCompleterModel.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace ScriptCanvasEditor
{
    SourceHandlePropertyAssetCtrl::SourceHandlePropertyAssetCtrl(QWidget* parent)
        : AzToolsFramework::PropertyAssetCtrl(parent)
    {
    }

    AzToolsFramework::AssetBrowser::AssetSelectionModel SourceHandlePropertyAssetCtrl::GetAssetSelectionModel()
    {
        auto selectionModel = AssetSelectionModel::SourceAssetTypeSelection(m_sourceAssetFilterPattern);
        selectionModel.SetTitle(m_title);
        return selectionModel;
    }

    void SourceHandlePropertyAssetCtrl::PopupAssetPicker()
    {
        // Request the AssetBrowser Dialog and set a type filter
        AssetSelectionModel selection = GetAssetSelectionModel();
        selection.SetSelectedFilePath(m_selectedSourcePath.c_str());

        AZStd::string defaultDirectory;
        if (m_defaultDirectoryCallback)
        {
            m_defaultDirectoryCallback->Invoke(m_editNotifyTarget, defaultDirectory);
            selection.SetDefaultDirectory(defaultDirectory);
        }

        AssetBrowserComponentRequestBus::Broadcast(&AssetBrowserComponentRequests::PickAssets, selection, parentWidget());
        if (selection.IsValid())
        {
            const auto source = azrtti_cast<const SourceAssetBrowserEntry*>(selection.GetResult());
            AZ_Assert(source, "Incorrect entry type selected. Expected source.");
            if (source)
            {
                SetSelectedSourcePath(source->GetFullPath());
            }
        }
    }

    void SourceHandlePropertyAssetCtrl::ClearAssetInternal()
    {
        SetSelectedSourcePath("");

        PropertyAssetCtrl::ClearAssetInternal();
    }

    void SourceHandlePropertyAssetCtrl::ConfigureAutocompleter()
    {
        if (m_completerIsConfigured)
        {
            return;
        }

        AzToolsFramework::PropertyAssetCtrl::ConfigureAutocompleter();

        AssetSelectionModel selection = GetAssetSelectionModel();
        m_model->SetFetchEntryType(AssetBrowserEntry::AssetEntryType::Source);
        m_model->SetFilter(selection.GetDisplayFilter());
    }

    void SourceHandlePropertyAssetCtrl::SetSourceAssetFilterPattern(const QRegExp& filterPattern)
    {
        m_sourceAssetFilterPattern = filterPattern;
    }

    AZ::IO::Path SourceHandlePropertyAssetCtrl::GetSelectedSourcePath() const
    {
        return m_selectedSourcePath;
    }

    void SourceHandlePropertyAssetCtrl::SetSelectedSourcePath(const AZ::IO::Path& sourcePath)
    {
        m_selectedSourcePath = sourcePath;

        AZStd::string displayText;
        if (!sourcePath.empty())
        {
            AzFramework::StringFunc::Path::GetFileName(sourcePath.c_str(), displayText);
        }
        m_browseEdit->setText(displayText.c_str());

        // The AssetID gets ignored, the only important bit is triggering the change for the RequestWrite
        emit OnAssetIDChanged(AZ::Data::AssetId());
    }

    void SourceHandlePropertyAssetCtrl::OnAutocomplete(const QModelIndex& index)
    {
        SetSelectedSourcePath(m_model->GetPathFromIndex(GetSourceIndex(index)));
    }

    QWidget* SourceHandlePropertyHandler::CreateGUI(QWidget* pParent)
    {
        SourceHandlePropertyAssetCtrl* newCtrl = aznew SourceHandlePropertyAssetCtrl(pParent);
        connect(newCtrl, &SourceHandlePropertyAssetCtrl::OnAssetIDChanged, this, [newCtrl](AZ::Data::AssetId newAssetID)
        {
            (void)newAssetID;
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::OnEditingFinished, newCtrl);
        });
        return newCtrl;
    }

    void SourceHandlePropertyHandler::ConsumeAttribute(SourceHandlePropertyAssetCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        // Let ConsumeAttributeForPropertyAssetCtrl handle all of the common attributes
        AzToolsFramework::ConsumeAttributeForPropertyAssetCtrl(GUI, attrib, attrValue, debugName);

        if (attrib == AZ::Edit::Attributes::SourceAssetFilterPattern)
        {
            AZStd::string filterPattern;
            if (attrValue->Read<AZStd::string>(filterPattern))
            {
                GUI->SetSourceAssetFilterPattern(QRegExp(filterPattern.c_str(), Qt::CaseInsensitive, QRegExp::Wildcard));
            }
        }
    }

    void SourceHandlePropertyHandler::WriteGUIValuesIntoProperty
        ( [[maybe_unused]] size_t index
        , [[maybe_unused]] SourceHandlePropertyAssetCtrl* GUI
        , [[maybe_unused]] property_t& instance
        , [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        (void)index;
        (void)node;

        auto sourceHandle = SourceHandle::FromRelativePath(nullptr, GUI->GetSelectedSourcePath());
        auto completeSourceHandle = CompleteDescription(sourceHandle);
        if (completeSourceHandle)
        {
            instance = property_t(*CompleteDescription(sourceHandle));
        }
        else
        {
            instance = property_t();
        }
    }

    bool SourceHandlePropertyHandler::ReadValuesIntoGUI(size_t index, SourceHandlePropertyAssetCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        (void)index;
        (void)node;

        GUI->blockSignals(true);

        GUI->SetSelectedSourcePath(instance.RelativePath());

        const AzToolsFramework::InstanceDataNode* parentNode = node->GetParent();
        AZ_Assert(parentNode && parentNode->HasInstances(), "Configuration instance is missing.");

        // Set notify target to the parent configuration instance.
        GUI->SetEditNotifyTarget(parentNode->FirstInstance());

        GUI->blockSignals(false);
        return false;
    }
}
