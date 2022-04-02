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

// \todo consider making a template version with return values, similar to execution out
// or perhaps safety checked versions with an array / table of any. something parsable
// or consider just having users make ebuses that the graphs will handle
// and wrapping the whole thing in a single class
// interpreter + ebus, and calling it EZ SC Hook or something like that

// the EZ Code driven thing, when it uses the click button, opens up a graph
// and drops in the main function WItH the typed arguments and return values stubbed out
// and makes those the required function of the graph!
// this code include using an ebus, for easiy switching to C++ extension

namespace ScriptCanvasEditor
{
    using namespace ScriptCanvas;

    Interpreter::Interpreter()
    {
        m_handlerSourceCompiled = AZ::EventHandler<const Configuration&>([this](const Configuration&) { OnSourceCompiled(); });
        m_handlerSourceFailed = AZ::EventHandler<const Configuration&>([this](const Configuration&) { OnSourceFailed(); });
        m_configuration.ConnectToSourceCompiled(m_handlerSourceCompiled);
        m_configuration.ConnectToSourceFailed(m_handlerSourceFailed);
    }

    // this can outrun the AP, and attmept to execute something before the asset is done, or ready, or about to be reloaded
    bool Interpreter::Execute()
    {
        MutexLock lock(m_mutex);

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

    bool Interpreter::IsExecutable() const
    {
        return m_executor.IsExecutable();
    }

    void Interpreter::OnSourceCompiled()
    {
        MutexLock lock(m_mutex);
        m_runtimeDataOverrides = AZStd::move(ConvertToRuntime(m_configuration.GetOverrides()));
        auto asset = AZ::Data::AssetManager::Instance().GetAsset<RuntimeAsset>
            ( m_runtimeDataOverrides.m_runtimeAsset.GetId()
            , AZ::Data::AssetLoadBehavior::PreLoad);
        asset.BlockUntilLoadComplete();
        m_runtimeDataOverrides.m_runtimeAsset = asset;
        m_executor.Initialize(m_runtimeDataOverrides, AZStd::any(m_userData));
    }

    void Interpreter::OnSourceFailed()
    {
        MutexLock lock(m_mutex);
        m_executor.StopAndClearExecutable();
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
                // consider running a full garbage collect
            });
        }
    }
}

