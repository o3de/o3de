/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AssetMemoryAnalyzer_precompiled.h"
#include <CryCommon/IConsole.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "AssetMemoryAnalyzerSystemComponent.h"

#include <IGem.h>

namespace AssetMemoryAnalyzer
{
    class AssetMemoryAnalyzerModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(AssetMemoryAnalyzerModule, "{899B0A20-E21D-49BF-ADAF-A2396C27CFCC}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(AssetMemoryAnalyzerModule, AZ::OSAllocator, 0);

        AssetMemoryAnalyzerModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                AssetMemoryAnalyzerSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AssetMemoryAnalyzerSystemComponent>(),
            };
        }

        void OnCrySystemInitialized([[maybe_unused]] ISystem& system, [[maybe_unused]] const SSystemInitParams& systemInitParams) override
        {
            REGISTER_CVAR2_CB_DEV_ONLY(
                "assetmem_enabled",
                &m_cvarEnabled,
                0,
                VF_NULL,
                "AssetMemoryAnalyzer: Enable or disable the Asset Memory Analyzer.",
                [](ICVar* pArgs)
                {
                    bool enabled = pArgs->GetIVal() ? true : false;
                    EBUS_EVENT(AssetMemoryAnalyzerRequestBus, SetEnabled, enabled);
                }
            );

            REGISTER_COMMAND_DEV_ONLY(
                "assetmem_export_json",
                [](IConsoleCmdArgs*) { EBUS_EVENT(AssetMemoryAnalyzerRequestBus, ExportJSONFile, nullptr); },
                0,
                "AssetMemoryAnalyzer: Export JSON analysis to @log@ directory.");

            REGISTER_COMMAND_DEV_ONLY(
                "assetmem_export_csv",
                [](IConsoleCmdArgs*) { EBUS_EVENT(AssetMemoryAnalyzerRequestBus, ExportCSVFile, nullptr); },
                0,
                "AssetMemoryAnalyzer: Export CSV analysis to @log@ directory. (Top-level assets only.)");

            EBUS_EVENT(AssetMemoryAnalyzerRequestBus, SetEnabled, m_cvarEnabled != 0);
        }

    private:
        int m_cvarEnabled = 0;
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AssetMemoryAnalyzer, AssetMemoryAnalyzer::AssetMemoryAnalyzerModule)
