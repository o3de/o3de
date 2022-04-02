/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Framework/Interpreter.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ScriptCanvasEditor
{
    using namespace ScriptCanvas;

    bool Interpreter::Execute()
    {
        if (IsExecutable())
        {
            m_executor.InitializeAndExecute(m_runtimeDataOverrides, AZStd::any(m_runtimeUserData));
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

    void Interpreter::OnAssetAvailable()
    {
        // copy build, convert to runtime
        m_executor.Initialize(m_runtimeDataOverrides, AZStd::any(m_runtimeDataOverrides));
    }

    void Interpreter::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Interpreter>()
                ->Field("sourceName", &Interpreter::m_configuration)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
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

    void Interpreter::ResetRuntimeUserData()
    {
        m_runtimeUserData = Execution::Reference(this, azrtti_typeid(this));
    }

    void Interpreter::SetScript(SourceHandle source)
    {
        m_configuration.Refresh(source);
        // register OnAssetAvailable() for execution
    }

    void Interpreter::SetRuntimeUserData(AZStd::any&& runtimeUserData)
    {
        m_runtimeUserData = runtimeUserData;
    }

    void Interpreter::Stop()
    {
        if (IsExecutable())
        {
            m_executor.Stop();
            // consider running a full garbage collect
        }
    }
}

