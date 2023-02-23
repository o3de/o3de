/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformIncl.h>
#include <AzCore/base.h>

#include <AzCore/Module/Environment.h>
#include <AzCore/Module/Module.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/Entity.h>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Tests/DLLTestVirtualClass.h>

#include "ModuleTestBus.h"

class ReflectedClass
{
public:

    AZ_TYPE_INFO(ReflectedClass, "{277E3FC1-5BE3-4890-A2CF-D389162F8D05}")

    ReflectedClass()
        : m_float(1.f)
        , m_int(2) {}

    float m_float;
    int m_int;

    static void Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ReflectedClass>()->
                Field("FloatField", &ReflectedClass::m_float)->
                Field("IntField", &ReflectedClass::m_int);
        }
    }
};

class DllModule
    : public AZ::Module
    , public ModuleTestRequestBus::Handler
{
public:
    AZ_RTTI(DllModule, "{99C6BF95-847F-4EEE-BB60-9B26D02FF577}", AZ::Module);
    AZ_CLASS_ALLOCATOR(DllModule, AZ::SystemAllocator);

    DllModule()
    {
        ModuleTestRequestBus::Handler::BusConnect();
    }

    ~DllModule() override
    {
        ModuleTestRequestBus::Handler::BusDisconnect();
    }

    /*void Reflect(AZ::ReflectContext* context) override
    {
        AZ_Printf("DLL", "Reflect called");
        ReflectedClass::Reflect(context);
    }*/

    const char* GetModuleName() override
    {
        return "DllModule";
    }
};

//==============================================================================
// Execute DLL tests
//==============================================================================
extern "C" AZ_DLL_EXPORT void DoTests()
{
    // Allocate memory from the system allocator and pass it with a bus call to be freed from another module
    void* address = azmalloc(128);

    // Use the bus to fire a message on the bus to verify it works across modules
    AZ::EntityId hackEntityIdAddress((AZ::u64)address); // HACK to pass the address to another module, otherwise we need to create a common EBus
    AZ::TransformNotificationBus::Broadcast(&AZ::TransformNotificationBus::Events::OnParentChanged, AZ::EntityId(), hackEntityIdAddress);
}


//////////////////////////////////////////////////////////////////////////
/// DLL Test shared environment variable test
AZ::EnvironmentVariable<UnitTest::DLLTestVirtualClass> s_dllTestVar;

extern "C" AZ_DLL_EXPORT void CreateDLLTestVirtualClass(const char* variableName)
{
    s_dllTestVar = AZ::Environment::CreateVariable<UnitTest::DLLTestVirtualClass>(variableName);
}

extern "C" AZ_DLL_EXPORT void DestroyDLLTestVirtualClass()
{
    s_dllTestVar.Reset();
}
//////////////////////////////////////////////////////////////////////////


extern "C" AZ_DLL_EXPORT void InitializeDynamicModule()
{
    AZ_Printf("DLL", "InitializeDynamicModule called");
}

extern "C" AZ_DLL_EXPORT AZ::Module * CreateModuleClass()
{
    AZ_Printf("DLL", "CreateModuleClass called");
    return new DllModule();
}

extern "C" AZ_DLL_EXPORT void DestroyModuleClass(AZ::Module* module)
{
    AZ_Printf("DLL", "DestroyModuleClass called");
    delete module;
}

extern "C" AZ_DLL_EXPORT void UninitializeDynamicModule()
{
    AZ_Printf("DLL", "UninitializeDynamicModule called");
}
