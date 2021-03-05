/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "precompiled.h"

#include "ScriptCanvasDiagnosticSystemComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <ScriptCanvasDiagnosticLibrary/DebugDrawBus.h>
#include "DebugLibraryDefinition.h"

namespace ScriptCanvasDiagnostics
{
    void SystemComponent::Init()
    {
        AZ::EnvironmentVariable<ScriptCanvas::NodeRegistry> nodeRegistryVariable = AZ::Environment::FindVariable<ScriptCanvas::NodeRegistry>(ScriptCanvas::s_nodeRegistryName);
        if (nodeRegistryVariable)
        {
            ScriptCanvas::NodeRegistry& nodeRegistry = nodeRegistryVariable.Get();
            ScriptCanvas::Libraries::Debug::InitNodeRegistry(nodeRegistry);
        }
    }

    SystemComponent::~SystemComponent()
    {
    }

    void SystemComponent::Activate()
    {
        SystemRequestBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
    }

    void SystemComponent::Deactivate()
    {
        CrySystemEventBus::Handler::BusDisconnect();
        SystemRequestBus::Handler::BusDisconnect();
    }

    void SystemComponent::OnDebugDraw()
    {
        DebugDrawBus::Broadcast(&DebugDrawRequests::OnDebugDraw, m_system->GetIRenderer());
    }

    void SystemComponent::OnCrySystemShutdown([[maybe_unused]] ISystem& system)
    {
        m_system->GetIRenderer()->RemoveRenderDebugListener(this);
    }

    void SystemComponent::OnCrySystemInitialized(ISystem& system, [[maybe_unused]] const SSystemInitParams& systemInitParams)
    {
        m_system = &system;
        AZ_Assert(m_system->GetIRenderer(), "ScriptCanvasDiagnostics requires IRenderer");

        m_system->GetIRenderer()->AddRenderDebugListener(this);
    }

    bool SystemComponent::IsEditor()
    {
        return m_system && m_system->GetGlobalEnvironment() && m_system->GetGlobalEnvironment()->IsEditor();
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ScriptCanvas::Libraries::Debug::Reflect(context);
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SystemComponent>("Script Canvas Diagnostic", "Script Canvas Diagnostic System Component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }
}
