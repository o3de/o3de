/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptCanvasAutoGenRegistry.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/fixed_string.h>

#include <ScriptCanvas/Libraries/ScriptCanvasNodeRegistry.h>

static constexpr const char* s_scriptCanvasModelName = "ScriptCanvasModel";
static AZ::EnvironmentVariable<ScriptCanvasModel> s_scriptModel;

ScriptCanvasModel& ScriptCanvasModel::Instance()
{
    if (!s_scriptModel)
    {
        s_scriptModel = AZ::Environment::CreateVariable<ScriptCanvasModel>(s_scriptCanvasModelName);
    }

    return s_scriptModel.Get();
}

void ScriptCanvasModel::RemoveDescriptor(AZ::ComponentDescriptor* descriptor)
{
    m_descriptors.remove(descriptor);
}

void ScriptCanvasModel::Release()
{
    for (auto descriptor : m_descriptors)
    {
        descriptor->ReleaseDescriptor();
    }

    m_descriptors.clear();
    m_registeredReflections.clear();
}

void ScriptCanvasModel::Init()
{
    for (auto descriptor : m_descriptors)
    {
        if (auto componentApplication = AZ::Interface<AZ::ComponentApplicationRequests>::Get())
        {
            componentApplication->RegisterComponentDescriptor(descriptor);

            if (m_verbose)
            {
                AZ_Info("ScriptCanvas", "Register Descriptor: %s", descriptor->GetName());
            }

        }
    }
}

bool ScriptCanvasModel::RegisterReflection(const AZStd::string& name, ReflectFunction reflect, AZ::ComponentDescriptor* descriptor/* = nullptr*/)
{
    if (!m_registeredReflections.contains(name))
    {
        if (descriptor)
        {
            m_descriptors.push_back(descriptor);

            if (m_verbose)
            {
                AZ_Info("ScriptCanvas", "RegisterReflection Descriptor: %s", name.c_str());
            }
        }
        else
        {
            m_registeredReflections[name] = reflect;

            if (m_verbose)
            {
                AZ_Info("ScriptCanvas", "RegisterReflection Reflect: %s", name.c_str());
            }
        }


        return true;
    }

    if (m_verbose)
    {
        AZ_Info("ScriptCanvas", "RegisterReflection: %s FAILED", name.c_str());
    }
    
    return false;
}

void ScriptCanvasModel::Reflect(AZ::ReflectContext* context)
{
    for (auto& reflection : m_registeredReflections)
    {
        if (m_verbose)
        {
            AZ_Info("ScriptCanvas", "Reflecting: %s", reflection.first.c_str());
        }

        reflection.second(context);
    }
}
