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

void ScriptCanvasModel::Init()
{
    if (auto componentApplication = AZ::Interface<AZ::ComponentApplicationRequests>::Get())
    {
        for (auto& it : m_registry)
        {
            if (m_verbose)
            {
                AZ_TracePrintf("ScriptCanvas", "Registering BaseClass Descriptor for: %s\n", it.second.m_factory->GetName().c_str());
            }

            it.second.m_descriptor = it.second.m_factory->GetDescriptor();
            componentApplication->RegisterComponentDescriptor(it.second.m_descriptor);
        }
    }
}

void ScriptCanvasModel::Release()
{
    for (auto& entry : m_registry)
    {
        entry.second.m_descriptor->ReleaseDescriptor();
    }
}

void ScriptCanvasModel::Register(const char* gemOrModuleName, [[maybe_unused]] const char* typeName, const char* typeHash, IScriptCanvasNodeFactory* factory)
{
    AZStd::string gemOrModule = gemOrModuleName;
    if (m_registry.find(typeHash) != m_registry.end())
    {
        static int duplicateCounter = 1;

        AZStd::string newName = AZStd::string::format("%s_%d", gemOrModuleName, duplicateCounter++);

        // Duplicate hash, emit warning and change the module name to avoid the problem
        AZ_Warning("Duplicate ScriptCanvas registration, consider making the node name unique: %s::%s, %s - to retain functionality, it will be found with a new module name of: %s\n", gemOrModuleName, typeName, typeHash, newName.c_str());
        gemOrModule.append(newName.c_str());
    }

    Entry entry;
    entry.m_gemOrModuleName = gemOrModule;
    entry.m_factory = factory;
    // Do not assign m_descriptor, this is happening at static initialization time, the types do not yet exist, see ScriptCanvasModel::Init
    m_registry[typeHash] = entry;

    if (m_verbose)
    {
        AZ_TracePrintf("ScriptCanvas", "ScriptCanvas: >> REGISTERED: %s::%s, %s\n", gemOrModuleName, typeName, typeHash);
    }
}

void ScriptCanvasModel::RegisterReflection(const AZStd::string& name, ReflectFunction reflect)
{
    if (!m_registeredReflections.contains(name))
    {
        if (m_verbose)
        {
            AZ_TracePrintf("ScriptCanvas", "RegisterReflection: %s\n", name.c_str());
        }

        m_registeredReflections[name] = reflect;
    }

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

const AZ::ComponentDescriptor* ScriptCanvasModel::GetDescriptor(const char* typehash) const
{
    auto it = m_registry.find(AZStd::string(typehash));
    if (it != m_registry.end())
    {
        return it->second.m_factory->GetDescriptor();
    }

    return nullptr;
}
