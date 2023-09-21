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
    static bool _initialized = false;
    if (!s_scriptModel)
    {
        s_scriptModel = AZ::Environment::CreateVariable<ScriptCanvasModel>(s_scriptCanvasModelName);
        _initialized = true;
    }

    return s_scriptModel.Get();
}

ScriptCanvasModel::~ScriptCanvasModel()
{
    Release();
}

void ScriptCanvasModel::Release()
{
    for (auto descriptor : m_descriptors)
    {
        descriptor->ReleaseDescriptor();
    }
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
                AZ_TracePrintf("ScriptCanvas", "Register Descriptor: %s\n", descriptor->GetName());
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
                AZ_TracePrintf("ScriptCanvas", "RegisterReflection Descriptor: %s\n", name.c_str());
            }
        }
        else
        {
            m_registeredReflections[name] = reflect;

            if (m_verbose)
            {
                AZ_TracePrintf("ScriptCanvas", "RegisterReflection Reflect: %s\n", name.c_str());
            }
        }


        return true;
    }

    if (m_verbose)
    {
        AZ_TracePrintf("ScriptCanvas", "RegisterReflection: %s FAILED\n", name.c_str());
    }
    
    return false;
}

void ScriptCanvasModel::Reflect(AZ::ReflectContext* context)
{
    for (auto& reflection : m_registeredReflections)
    {
        if (m_verbose)
        {
            AZ_TracePrintf("ScriptCanvas", "Reflecting: %s\n", reflection.first.c_str());
        }

        reflection.second(context);
    }
}
