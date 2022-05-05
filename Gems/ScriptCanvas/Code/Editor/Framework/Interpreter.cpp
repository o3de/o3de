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
    using namespace ScriptCanvasBuilder;

    Interpreter::Interpreter()
    {
        m_handlerPropertiesChanged = m_configuration.ConnectToPropertiesChanged([this](const Configuration&) { OnPropertiesChanged(); });
        m_handlerSourceCompiled = m_configuration.ConnectToSourceCompiled([this](const Configuration&) { OnSourceCompiled(); });
        m_handlerSourceFailed = m_configuration.ConnectToSourceFailed([this](const Configuration&) { OnSourceFailed(); });

        // #scriptcanvas_component_extension
        m_configuration.SetAcceptsComponentScript(false);
        m_handlerUnacceptedComponentScript = m_configuration.ConnectToIncompatilbleScript([this](const Configuration&) { OnSourceIncompatible(); });
    }

    Interpreter::~Interpreter()
    {
        DataSystemAssetNotificationsBus::Handler::BusDisconnect();
    }

    AZ::EventHandler<const Interpreter&> Interpreter::ConnectOnStatusChanged(AZStd::function<void(const Interpreter&)>&& function) const
    {
        AZ::EventHandler<const Interpreter&> handler(function);
        handler.Connect(m_onStatusChanged);
        return handler;
    }

    bool Interpreter::Execute()
    {
        if (m_executor.IsExecutable())
        {    
            AZ::SystemTickBus::QueueFunction([this]()
            {
                MutexLock lock(m_mutex);

                if (m_runtimePropertiesDirty)
                {
                    if (m_executor.IsExecutable())
                    {
                        m_executor.StopAndClearExecutable();
                    }
                    
                    m_executor.Initialize();
                    m_runtimePropertiesDirty = false;
                }

                if (m_executor.IsExecutable())
                {
                    SetSatus(InterpreterStatus::Running);

                    if (m_executor.IsPure())
                    {
                        m_executor.Execute();
                        SetSatus(InterpreterStatus::Ready);
                    }
                    else
                    {
                        m_executor.Execute();
                    }
                }
            });

            return true;
        }
        else
        {
            return false;
        }
    }

    const Configuration& Interpreter::GetConfiguration() const
    {
        return m_configuration;
    }

    InterpreterStatus Interpreter::GetStatus() const
    {
        return m_status;
    }

    AZStd::string_view Interpreter::GetStatusString() const
    {
        return ToString(m_status);
    }

    bool Interpreter::InitializeExecution(ScriptCanvas::RuntimeAssetPtr asset)
    {
        if (asset.Get())
        {
            if (m_executor.IsExecutable())
            {
                m_executor.StopAndClearExecutable();
            }

            auto overrides = ConvertToRuntime(m_configuration.GetOverrides());
            if (ReplaceAsset(overrides, asset))
            {
                m_executor.TakeRuntimeDataOverrides(AZStd::move(overrides));
                m_executor.Initialize();
                m_runtimePropertiesDirty = false;
                return true;
            }
        }

        return false;
    }

    bool Interpreter::IsExecutable() const
    {
        return m_executor.IsExecutable();
    }

    Configuration& Interpreter::ModConfiguration()
    {
        return m_configuration;
    }

    void Interpreter::OnAssetNotReady()
    {
        MutexLock lock(m_mutex);
        if (m_executor.IsExecutable())
        {
            m_executor.StopAndClearExecutable();
        }
        m_runtimePropertiesDirty = true;
        SetSatus(InterpreterStatus::Pending);
    }

    void Interpreter::OnPropertiesChanged()
    {
        m_runtimePropertiesDirty = true;
        DataSystemAssetNotificationsBus::Handler::BusDisconnect();
        DataSystemAssetNotificationsBus::Handler::BusConnect(m_configuration.GetSource().Id());
    }

    void Interpreter::OnReady(ScriptCanvas::RuntimeAssetPtr asset)
    {
        MutexLock lock(m_mutex);
        InitializeExecution(asset);
        SetSatus(InterpreterStatus::Ready);
    }

    void Interpreter::OnSourceCompiled()
    {
        MutexLock lock(m_mutex);
        m_runtimePropertiesDirty = true;
        SetSatus(InterpreterStatus::Configured);
        BuilderAssetResult assetResult;
        DataSystemAssetRequestsBus::BroadcastResult(assetResult, &DataSystemAssetRequests::LoadAsset, m_configuration.GetSource());
        if (assetResult.status == BuilderAssetStatus::Ready)
        {
            OnReady(assetResult.data);
        }
    }

    void Interpreter::OnSourceFailed()
    {
        MutexLock lock(m_mutex);
        if (m_executor.IsExecutable())
        {
            m_executor.StopAndClearExecutable();
        }
        m_runtimePropertiesDirty = true;
        SetSatus(InterpreterStatus::Misconfigured);
    }

    void Interpreter::OnSourceIncompatible()
    {
        MutexLock lock(m_mutex);
        if (m_executor.IsExecutable())
        {
            m_executor.StopAndClearExecutable();
        }
        m_runtimePropertiesDirty = true;
        SetSatus(InterpreterStatus::Incompatible);
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
        m_runtimePropertiesDirty = true;
    }

    void Interpreter::ResetUserData()
    {
        MutexLock lock(m_mutex);
        m_executor.TakeUserData(ExecutionUserData(Execution::TypeErasedReference(this, azrtti_typeid(this))));
        m_runtimePropertiesDirty = true;
    }

    void Interpreter::SetScript(SourceHandle source)
    {
        MutexLock lock(m_mutex);
        m_configuration.Refresh(source);
    }

    void Interpreter::SetSatus(InterpreterStatus status)
    {
        if (status != m_status)
        {
            m_status = status;
            m_onStatusChanged.Signal(*this);
        }
    }

    void Interpreter::Stop()
    {
        MutexLock lock(m_mutex);

        if (m_executor.IsExecutable())
        {
            AZ::SystemTickBus::QueueFunction([this]()
            {
                MutexLock lock(m_mutex);

                if (m_executor.IsExecutable())
                {
                    m_executor.StopAndKeepExecutable();
                }

                SystemRequestBus::Broadcast(&SystemRequests::RequestGarbageCollect);

                if (m_executor.IsExecutable())
                {
                    SetSatus(InterpreterStatus::Ready);
                }
                else
                {
                    SetSatus(InterpreterStatus::Pending);
                }
            });
        }
        else
        {
            SetSatus(InterpreterStatus::Pending);
        }
    }

    void Interpreter::TakeUserData(ExecutionUserData&& runtimeUserData)
    {
        MutexLock lock(m_mutex);
        m_executor.TakeUserData(AZStd::move(runtimeUserData));
        m_runtimePropertiesDirty = true;
    }
}

