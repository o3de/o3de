/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: LuaScriptModule.
 * Create: 2021-07-14
 */
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <LuaScriptSystemComponent.h>

namespace LuaScript
{
    class LuaScriptModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(LuaScriptModule, "{B485B260-BC8F-4FC3-ADA3-F97696FF43AE}", AZ::Module);
        AZ_CLASS_ALLOCATOR(LuaScriptModule, AZ::SystemAllocator, 0);

        LuaScriptModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                LuaScriptSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<LuaScriptSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_LuaScript, LuaScript::LuaScriptModule)
