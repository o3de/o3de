/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <LyViewPaneNames.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>
#include <Editor/Framework/Configuration.h>
#include <ScriptCanvas/Components/EditorUtils.h>
#include <ScriptCanvas/Bus/RequestBus.h>

namespace ScriptCanvasEditor
{
    class OnScopeEnd
    {
    public:
        using ScopeEndFunctor = std::function<void()>;

    private:
        ScopeEndFunctor m_functor;

    public:
        OnScopeEnd(ScopeEndFunctor&& functor)
            : m_functor(AZStd::move(functor))
        {}

        OnScopeEnd(const ScopeEndFunctor& functor)
            : m_functor(functor)
        {}

        ~OnScopeEnd()
        {
            m_functor();
        }
    };

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
        ScriptCanvasBuilder::DataSystemSourceNotificationsBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
    }

    void Configuration::ClearVariables()
    {
        m_propertyOverrides.Clear();
    }

    const ScriptCanvasBuilder::BuildVariableOverrides* Configuration::CompileLatest()
    {
        return CompileLatestInternal() == BuildStatusValidation::Good ? &m_propertyOverrides : nullptr;
    }

    Configuration::BuildStatusValidation Configuration::CompileLatestInternal()
    {
        ScriptCanvasBuilder::BuilderSourceResult result;
        ScriptCanvasBuilder::DataSystemSourceRequestsBus::BroadcastResult
            ( result
            , &ScriptCanvasBuilder::DataSystemSourceRequests::CompileBuilderData
            , m_sourceHandle);

        const auto validation = ValidateBuildResult(result);
        if (validation == BuildStatusValidation::Good)
        {
            MergeWithLatestCompilation(*result.data);
        }

        return validation;
    }

    AZ::EventHandler<const Configuration&> Configuration::ConnectToPropertiesChanged(AZStd::function<void(const Configuration&)>&& function) const
    {
        AZ::EventHandler<const Configuration&> handler(function);
        handler.Connect(m_eventPropertiesChanged);
        return handler;
    }

    AZ::EventHandler<const Configuration&> Configuration::ConnectToSourceCompiled(AZStd::function<void(const Configuration&)>&& function) const
    {
        AZ::EventHandler<const Configuration&> handler(function);
        handler.Connect(m_eventSourceCompiled);
        return handler;
    }

    AZ::EventHandler<const Configuration&> Configuration::ConnectToSourceFailed(AZStd::function<void(const Configuration&)>&& function) const
    {
        AZ::EventHandler<const Configuration&> handler(function);
        handler.Connect(m_eventSourceFailed);
        return handler;
    }

    const ScriptCanvasBuilder::BuildVariableOverrides& Configuration::GetOverrides() const
    {
        return m_propertyOverrides;
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

    AZ::u32 Configuration::OnEditorChangeProperties()
    {
        m_eventPropertiesChanged.Signal(*this);
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    AZ::u32 Configuration::OnEditorChangeSource()
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
                ->Field("sourceHandle", &Configuration::m_sourceHandle)
                ->Field("sourceName", &Configuration::m_sourceName)
                ->Field("propertyOverrides", &Configuration::m_propertyOverrides)
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
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Configuration::OnEditorChangeSource)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_propertyOverrides, "Properties", "Script Canvas Graph Properties")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Configuration::OnEditorChangeProperties)
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
        ScriptCanvasBuilder::DataSystemSourceNotificationsBus::Handler::BusDisconnect();
        m_sourceHandle = sourceHandle.Describe();
        CompleteDescriptionInPlace(m_sourceHandle);

        if (m_sourceHandle.IsDescriptionValid())
        {
            m_sourceName = m_sourceHandle.Path().Filename().Native();
        }

        m_eventPropertiesChanged.Signal(*this);

        if (!m_sourceHandle.Id().IsNull())
        {
            ScriptCanvasBuilder::DataSystemSourceNotificationsBus::Handler::BusConnect(m_sourceHandle.Id());

            if (!m_sourceHandle.Path().empty())
            {
                const auto validation = CompileLatestInternal();
                if (validation == BuildStatusValidation::Good)
                {
                    m_eventSourceCompiled.Signal(*this);
                    return;
                }
                else if (validation == BuildStatusValidation::IncompatibleScript)
                {
                    AZ_Error("ScriptCanvas", false, "Selected Script is not compatible with this configuration.");
                    m_eventIncompatibleScript.Signal(*this);
                    return;
                }
                else
                {
                    AZ_Warning("ScriptCanvasBuilder", false, "Runtime information did not build for ScriptCanvas Component using source: %s"
                        , m_sourceHandle.ToString().c_str());
                }
            }
            else
            {
                AZ_Warning("ScriptCanvasBuilder", false, "Configuration had no valid path for %s and won't compile or expose variables."
                    , m_sourceHandle.ToString().c_str());
            }
        }
        else
        {
            AZ_Warning("ScriptCanvas", m_sourceHandle.Path().empty()
                , "Configuration had no valid ID for %s and won't compile or expose variables.", m_sourceHandle.Path().c_str());
        }

        m_eventSourceFailed.Signal(*this);
    }

    void Configuration::SourceFileChanged
        ( const ScriptCanvasBuilder::BuilderSourceResult& result
        , [[maybe_unused]] AZStd::string_view relativePath
        , [[maybe_unused]] AZStd::string_view scanFolder)
    {
        const auto validation = ValidateBuildResult(result);
        if (validation == BuildStatusValidation::Good)
        {
            MergeWithLatestCompilation(*result.data);
            m_eventSourceCompiled.Signal(*this);
        }
        else if (validation == BuildStatusValidation::Bad)
        {
            m_eventSourceFailed.Signal(*this);
        }
        else if (validation == BuildStatusValidation::IncompatibleScript)
        {
            m_eventIncompatibleScript.Signal(*this);
        }
    }

    void Configuration::SourceFileFailed([[maybe_unused]] AZStd::string_view relativePath
        , [[maybe_unused]] AZStd::string_view scanFolder)
    {
        m_eventSourceFailed.Signal(*this);       
        // display error icon
    }

    void Configuration::SourceFileRemoved([[maybe_unused]] AZStd::string_view relativePath
        , [[maybe_unused]] AZStd::string_view scanFolder)
    {
        m_eventSourceFailed.Signal(*this);
        // display removed icon
    }

    Configuration::BuildStatusValidation Configuration::ValidateBuildResult(const ScriptCanvasBuilder::BuilderSourceResult& result) const
    {
        if (result.status != ScriptCanvasBuilder::BuilderSourceStatus::Good || !result.data)
        {
            AZ_Error
                ( "ScriptCanvas"
                , !(result.status == ScriptCanvasBuilder::BuilderSourceStatus::Good && result.data)
                , "Configuration::SourceFileChanged received good status with no data");

            return BuildStatusValidation::Bad;
        }
        else if (result.data->m_isComponentScript && !m_acceptsComponentScript)
        {
            // #scriptcanvas_component_extension
            return BuildStatusValidation::IncompatibleScript;
        }
        else
        {
            return BuildStatusValidation::Good;
        }
    }

    // #scriptcanvas_component_extension ...
    bool Configuration::AcceptsComponentScript() const
    {
        return m_acceptsComponentScript;
    }

    void Configuration::SetAcceptsComponentScript(bool value)
    {
        m_acceptsComponentScript = value;
    }

    AZ::EventHandler<const Configuration&> Configuration::ConnectToIncompatilbleScript(AZStd::function<void(const Configuration&)>&& function) const
    {
        AZ::EventHandler<const Configuration&> handler(function);
        handler.Connect(m_eventIncompatibleScript);
        return handler;
    }
    // ... #scriptcanvas_component_extension

}
