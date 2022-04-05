/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Framework/Interpreter.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Builder/ScriptCanvasBuilder.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>

namespace ScriptCanvasEditor
{
    using namespace ScriptCanvas;

    Interpreter::Interpreter()
    {
        m_handlerPropertiesChanged = AZ::EventHandler<const Configuration&>([this](const Configuration&) { OnPropertiesChanged(); });
        m_handlerSourceCompiled = AZ::EventHandler<const Configuration&>([this](const Configuration&) { OnSourceCompiled(); });
        m_handlerSourceFailed = AZ::EventHandler<const Configuration&>([this](const Configuration&) { OnSourceFailed(); });
        m_configuration.ConnectToPropertiesChanged(m_handlerPropertiesChanged);
        m_configuration.ConnectToSourceCompiled(m_handlerSourceCompiled);
        m_configuration.ConnectToSourceFailed(m_handlerSourceFailed);

        // tell the data system about the script in question...and for it to keep the runtime ready for this thing
        // then for the data system will keep this up to date on what is ready and what is not 
    }

    void Interpreter::ConvertPropertiesToRuntime()
    {
        MutexLock lock(m_mutex);
        if (m_runtimePropertiesDirty)
        {
            m_runtimeDataOverrides = AZStd::move(ConvertToRuntime(m_configuration.GetOverrides()));
            m_runtimePropertiesDirty = false;
        }
    }

    // this can outrun the AP, and attempt to execute something before the asset is done, or ready, or about to be reloaded
    bool Interpreter::Execute()
    {
        MutexLock lock(m_mutex);

        if (m_runtimePropertiesDirty)
        {
            InitializeExecution();
        }
        
        if (IsExecutable())
        {
            AZ::SystemTickBus::QueueFunction([this]() { m_executor.Execute(); });
            return true;
        }
        else
        {
            return false;
        }
    }

    bool Interpreter::InitializeExecution()
    {
        ConvertPropertiesToRuntime();

        m_runtimeDataOverrides.m_runtimeAsset = AZ::Data::AssetManager::Instance().GetAsset<RuntimeAsset>
            ( m_runtimeDataOverrides.m_runtimeAsset.GetId()
            , AZ::Data::AssetLoadBehavior::PreLoad);
        m_runtimeDataOverrides.m_runtimeAsset.BlockUntilLoadComplete();

        if (m_runtimeDataOverrides.m_runtimeAsset.Get())
        {
            m_executor.Initialize(m_runtimeDataOverrides, AZStd::any(m_userData));
            return true;
        }
        else
        {
            return false;
        }
    }

    bool Interpreter::IsExecutable() const
    {
        return m_executor.IsExecutable();
    }

    void Interpreter::OnPropertiesChanged()
    {
        m_runtimePropertiesDirty = true;
    }

    void Interpreter::OnSourceCompiled()
    {
        MutexLock lock(m_mutex);
        InitializeExecution();
    }

    void Interpreter::OnSourceFailed()
    {
        MutexLock lock(m_mutex);
        m_executor.StopAndClearExecutable();
        m_runtimePropertiesDirty = true;
    }

    void Interpreter::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Interpreter>()
                ->Field("sourceName", &Interpreter::m_configuration)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Interpreter>("Script Canvas Interpreter", "Select, Configure, and Execute a ScriptCanvas Graph")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Interpreter::m_configuration, "Configuration", "Configuration")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void Interpreter::RefreshConfiguration()
    {
        MutexLock lock(m_mutex);
        m_configuration.Refresh();
    }

    void Interpreter::ResetUserData()
    {
        MutexLock lock(m_mutex);
        m_userData = Execution::Reference(this, azrtti_typeid(this));
    }

    void Interpreter::SetScript(SourceHandle source)
    {
        MutexLock lock(m_mutex);
        m_configuration.Refresh(source);
    }

    void Interpreter::SetUserData(AZStd::any&& runtimeUserData)
    {
        MutexLock lock(m_mutex);
        m_userData = runtimeUserData;
    }

    void Interpreter::Stop()
    {
        MutexLock lock(m_mutex);

        if (IsExecutable())
        {
            AZ::SystemTickBus::QueueFunction([this]()
            {
                m_executor.StopAndKeepExecutable();
                SystemRequestBus::Broadcast(&SystemRequests::RequestGarbageCollect);
            });
        }
    }
}

