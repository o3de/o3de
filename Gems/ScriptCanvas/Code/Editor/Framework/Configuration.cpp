/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <LyViewPaneNames.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>
#include <Editor/Framework/Configuration.h>
#include <ScriptCanvas/Components/EditorUtils.h>
#include <ScriptCanvas/Bus/RequestBus.h>

namespace ScriptCanvasEditor
{
    Configuration::Configuration()
        : Configuration(SourceHandle())
    {
    }

    Configuration::Configuration(const SourceHandle& sourceHandle)
        : m_sourceHandle(sourceHandle)
    {
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        Refresh(m_sourceHandle);
    }

    Configuration::~Configuration()
    {
        ScriptCanvasBuilder::DataSystemNotificationsBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
    }

    void Configuration::ClearVariables()
    {
        m_propertyOverrides.Clear();
    }

    const ScriptCanvasBuilder::BuildVariableOverrides* Configuration::CompileLatest()
    {
        ScriptCanvasBuilder::BuildResult result;
        ScriptCanvasBuilder::DataSystemRequestsBus::BroadcastResult
            ( result
            , &ScriptCanvasBuilder::DataSystemRequests::CompileBuilderData
            , m_sourceHandle);

        if (result.status == ScriptCanvasBuilder::BuilderDataStatus::Good)
        {
            MergeWithLatestCompilation(result.data);
            return &m_propertyOverrides;
        }
        else
        {
            return nullptr;
        }
    }

    const SourceHandle& Configuration::GetSource() const
    {
        return m_sourceHandle;
    }

    bool Configuration::HasSource() const
    {
        return m_sourceHandle.IsDescriptionValid();
    }

    void Configuration::MergeWithLatestCompilation(const ScriptCanvasBuilder::BuildVariableOverrides& buildData)
    {
        ScriptCanvasBuilder::BuildVariableOverrides source(buildData);

        if (!m_propertyOverrides.IsEmpty())
        {
            source.CopyPreviousOverriddenValues(m_propertyOverrides);
        }

        m_propertyOverrides = AZStd::move(source);
        m_propertyOverrides.SetHandlesToDescription();
    }

    AZ::u32 Configuration::OnEditorChangeNotify()
    {
        ClearVariables();
        Refresh(m_sourceHandle);
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    void Configuration::OpenEditor([[maybe_unused]] const AZ::Data::AssetId& assetId, const AZ::Data::AssetType&)
    {
        AzToolsFramework::OpenViewPane(LyViewPane::ScriptCanvas);

        if (m_sourceHandle.IsDescriptionValid())
        {
            AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());
            GeneralRequestBus::BroadcastResult(openOutcome, &GeneralRequests::OpenScriptCanvasAsset, m_sourceHandle, Tracker::ScriptCanvasFileState::UNMODIFIED, -1);

            if (!openOutcome)
            {
                AZ_Warning("Script Canvas", openOutcome, "%s", openOutcome.GetError().data());
            }
        }
    }

    void Configuration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Configuration>()
                ->Field("sourceName", &Configuration::m_sourceName)
                ->Field("propertyOverrides", &Configuration::m_propertyOverrides)
                ->Field("sourceHandle", &Configuration::m_sourceHandle)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Configuration>("Script Canvas Configuration", "Select a Script Canvas graph and configure its properties.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/ScriptCanvas.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/ScriptCanvas/Viewport/ScriptCanvas.svg")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_sourceHandle, "Source File", "Script Canvas source file associated with this component")
                        ->Attribute("BrowseIcon", ":/stylesheet/img/UI20/browse-edit-select-files.svg")
                        ->Attribute("EditButton", "")
                        ->Attribute("EditDescription", "Open in Script Canvas Editor")
                        ->Attribute("EditCallback", &Configuration::OpenEditor)
                        ->Attribute(AZ::Edit::Attributes::AssetPickerTitle, "Script Canvas")
                        ->Attribute(AZ::Edit::Attributes::SourceAssetFilterPattern, "*.scriptcanvas")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Configuration::OnEditorChangeNotify)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_propertyOverrides, "Properties", "Script Canvas Graph Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void Configuration::Refresh()
    {
        Refresh(m_sourceHandle);
    }

    void Configuration::Refresh(const SourceHandle& sourceHandle)
    {
        ScriptCanvasBuilder::DataSystemNotificationsBus::Handler::BusDisconnect();
        m_sourceHandle = sourceHandle.Describe();
        CompleteDescriptionInPlace(m_sourceHandle);

        if (m_sourceHandle.IsDescriptionValid())
        {
            m_sourceName = m_sourceHandle.Path().Filename().Native();
        }

        if (m_sourceHandle.Id().IsNull())
        {
            AZ_Warning("ScriptCanvas", m_sourceHandle.Path().empty()
                , "Configuration had no valid ID for %s and won't properly expose variables.", m_sourceHandle.Path().c_str());
        }
        else
        {
            ScriptCanvasBuilder::DataSystemNotificationsBus::Handler::BusConnect(m_sourceHandle.Id());

            if (!(m_sourceHandle.Id().IsNull() && m_sourceHandle.Path().empty()))
            {
                if (!CompileLatest())
                {
                    AZ_Warning("ScriptCanvasBuilder", false, "Runtime information did not build for ScriptCanvas Component using source: %s"
                        , m_sourceHandle.ToString().c_str());
                }
            }
        }

        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast
            ( &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay
            , AzToolsFramework::Refresh_EntireTree_NewContent);
    }

    void Configuration::SourceFileChanged
        ( const ScriptCanvasBuilder::BuildResult& result
        , [[maybe_unused]] AZStd::string_view relativePath
        , [[maybe_unused]] AZStd::string_view scanFolder)
    {
        if (result.status == ScriptCanvasBuilder::BuilderDataStatus::Good)
        {
            MergeWithLatestCompilation(result.data);
            AzToolsFramework::ToolsApplicationNotificationBus::Broadcast
                ( &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay
                , AzToolsFramework::Refresh_EntireTree_NewContent);
        }
        else
        {
            // display error icon
        }
    }

    void Configuration::SourceFileFailed([[maybe_unused]] AZStd::string_view relativePath
        , [[maybe_unused]] AZStd::string_view scanFolder)
    {
        // display error icon
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast
            ( &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay
            , AzToolsFramework::Refresh_EntireTree_NewContent);
    }

    void Configuration::SourceFileRemoved([[maybe_unused]] AZStd::string_view relativePath
        , [[maybe_unused]] AZStd::string_view scanFolder)
    {
        // display removed icon
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast
            ( &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay
            , AzToolsFramework::Refresh_EntireTree_NewContent);
    }
}
