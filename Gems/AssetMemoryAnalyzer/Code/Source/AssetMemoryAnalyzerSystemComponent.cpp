/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <time.h>

#include <AzCore/Debug/AssetTrackingTypesImpl.h>
#include <AzCore/IO/SystemFile.h> // For AZ_MAX_PATH_LEN
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "AssetMemoryAnalyzerSystemComponent.h"

#include "AssetMemoryAnalyzer.h"
#include "DebugImGUI.h"
#include "ExportCSV.h"
#include "ExportJSON.h"

namespace AssetMemoryAnalyzer
{
    namespace
    {
        static const char* GetExportFile(const char* customFilename, const char* extension)
        {
            static char sharedBuffer[AZ_MAX_PATH_LEN];

            if (customFilename)
            {
                azsnprintf(sharedBuffer, AZ_ARRAY_SIZE(sharedBuffer), "@log@/%s", customFilename);
            }
            else
            {
                time_t ltime;
                time(&ltime);
                struct tm timeInfo;
                AZ_TRAIT_CTIME_LOCALTIME(&timeInfo, &ltime);
                strftime(sharedBuffer, AZ_ARRAY_SIZE(sharedBuffer), "@log@/assetmem-%Y-%m-%d-%H-%M-%S.", &timeInfo);
                azstrcat(sharedBuffer, AZ_ARRAY_SIZE(sharedBuffer), extension);
            }

            return sharedBuffer;
        }
    }

    static const char* VRAM_CATEGORIES[] =
    {
        "Texture",
        "Buffer",
        "Misc"
    };

    static const char* VRAM_SUBCATEGORIES[] =
    {
        "Rendertarget",
        "Texture",
        "Dynamic",
        "VB",
        "IB",
        "CB",
        "Other",
        "Misc"
    };

    class AssetMemoryAnalyzerSystemComponent::Impl
    {
    private:
        AZStd::unique_ptr<Analyzer> m_analyzer;
        DebugImGUI m_debugImGUI;
        ExportCSV m_exportCSV;
        ExportJSON m_exportJSON;

        friend class AssetMemoryAnalyzerSystemComponent;
    };


    AssetMemoryAnalyzerSystemComponent::AssetMemoryAnalyzerSystemComponent() : m_impl(new Impl)
    {
        AZ::AllocatorInstance<AZ::Debug::AssetTrackingAllocator>::Create();
    }

    AssetMemoryAnalyzerSystemComponent::~AssetMemoryAnalyzerSystemComponent()
    {
        m_impl.reset();  // Must delete objects before destroying the allocator
        AZ::AllocatorInstance<AZ::Debug::AssetTrackingAllocator>::Destroy();
    }

    void AssetMemoryAnalyzerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AssetMemoryAnalyzerSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AssetMemoryAnalyzerSystemComponent>("AssetMemoryAnalyzer", "Provides access to asset memory debugging features")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AssetMemoryAnalyzerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("AssetMemoryAnalyzerService", 0x23c52412));
    }

    void AssetMemoryAnalyzerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("AssetMemoryAnalyzerService", 0x23c52412));
    }

    void AssetMemoryAnalyzerSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void AssetMemoryAnalyzerSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    const char** AssetMemoryAnalyzerSystemComponent::GetVRAMCategories()
    {
        return VRAM_CATEGORIES;
    }

    const char** AssetMemoryAnalyzerSystemComponent::GetVRAMSubCategories()
    {
        return VRAM_SUBCATEGORIES;
    }

    bool AssetMemoryAnalyzerSystemComponent::IsEnabled() const
    {
        return m_impl->m_analyzer.get() != nullptr;
    }

    AZStd::shared_ptr<FrameAnalysis> AssetMemoryAnalyzerSystemComponent::GetAnalysis()
    {
        AZStd::shared_ptr<FrameAnalysis> result;

        if (m_impl->m_analyzer)
        {
            result = m_impl->m_analyzer->GetAnalysis();
        }

        return result;
    }

    void AssetMemoryAnalyzerSystemComponent::SetEnabled(bool enabled)
    {
        if (enabled)
        {
            if (!m_impl->m_analyzer)
            {
                m_impl->m_analyzer.reset(aznew Analyzer);
            }
        }
        else
        {
            m_impl->m_analyzer.reset();
        }
    }

    void AssetMemoryAnalyzerSystemComponent::ExportCSVFile(const char* path)
    {
        const char* outputPath = GetExportFile(path, "csv");
        m_impl->m_exportCSV.OutputCSV(outputPath);
    }

    void AssetMemoryAnalyzerSystemComponent::ExportJSONFile(const char* path)
    {
        const char* outputPath = GetExportFile(path, "json");
        m_impl->m_exportJSON.OutputJSON(outputPath);
    }

    void AssetMemoryAnalyzerSystemComponent::Init()
    {
        m_impl->m_debugImGUI.Init(this);
        m_impl->m_exportCSV.Init(this);
        m_impl->m_exportJSON.Init(this);
    }

    void AssetMemoryAnalyzerSystemComponent::Activate()
    {
        AssetMemoryAnalyzerRequestBus::Handler::BusConnect();
    }

    void AssetMemoryAnalyzerSystemComponent::Deactivate()
    {
        AssetMemoryAnalyzerRequestBus::Handler::BusDisconnect();
    }
}
