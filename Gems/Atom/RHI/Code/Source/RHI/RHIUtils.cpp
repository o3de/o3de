/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIMemoryStatisticsInterface.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI.Reflect/MemoryStatistics.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/CommandLine/CommandLine.h>

#include <AzCore/Console/IConsoleTypes.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Utils/Utils.h>

static constexpr char GraphicsDevModeSetting[] = "/O3DE/Atom/RHI/GraphicsDevMode";

AZ_CVAR(
    bool, 
    r_EnableAutoGpuMemDump, 
    false,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate,
    "If true AZ_RHI_DUMP_POOL_INFO_ON_FAIL will write a json file containing all gpu mem allocations. By default it is false.");

namespace AZ::RHI
{
    Ptr<Device> GetRHIDevice()
    {
        RHISystemInterface* rhiSystem = RHISystemInterface::Get();
        AZ_Assert(rhiSystem, "Failed to retrieve rhi system.");
        return rhiSystem->GetDevice();
    }

    FormatCapabilities GetCapabilities(ScopeAttachmentUsage scopeUsage, ScopeAttachmentAccess scopeAccess, AttachmentType attachmentType)
    {
        scopeAccess = AdjustAccessBasedOnUsage(scopeAccess, scopeUsage);

        FormatCapabilities capabilities = FormatCapabilities::None;

        if (attachmentType == AttachmentType::Image)
        {
            if (scopeUsage == ScopeAttachmentUsage::RenderTarget)
            {
                capabilities |= FormatCapabilities::RenderTarget;
            }
            else if (scopeUsage == ScopeAttachmentUsage::DepthStencil)
            {
                capabilities |= FormatCapabilities::DepthStencil;
            }
            else if (scopeUsage == ScopeAttachmentUsage::Shader)
            {
                capabilities |= FormatCapabilities::Sample;

                if (scopeAccess == ScopeAttachmentAccess::Write)
                {
                    capabilities |= FormatCapabilities::TypedStoreBuffer;
                }
            }
        }
        else if (attachmentType == AttachmentType::Buffer)
        {
            if (scopeAccess == ScopeAttachmentAccess::Read)
            {
                capabilities |= FormatCapabilities::TypedLoadBuffer;
            }
            if (scopeAccess == ScopeAttachmentAccess::Write)
            {
                capabilities |= FormatCapabilities::TypedStoreBuffer;
            }
        }

        return capabilities;
    }

    Format GetNearestDeviceSupportedFormat(Format requestedFormat, FormatCapabilities requestedCapabilities)
    {
        Ptr<Device> device = GetRHIDevice();
        if (device)
        {
            return device->GetNearestSupportedFormat(requestedFormat, requestedCapabilities);
        }
        AZ_Assert(device, "Failed to retrieve device.");
        return Format::Unknown;
    }

    Format ValidateFormat(Format originalFormat, [[maybe_unused]] const char* location, const AZStd::vector<Format>& formatFallbacks, FormatCapabilities requestedCapabilities)
    {
        if (originalFormat != Format::Unknown)
        {
            Format format = originalFormat;
            Format nearestFormat = GetNearestDeviceSupportedFormat(format, requestedCapabilities);

            // If format not supported, check all the fallbacks in the list provided
            for (size_t i = 0; i < formatFallbacks.size() && format != nearestFormat; ++i)
            {
                format = formatFallbacks[i];
                nearestFormat = GetNearestDeviceSupportedFormat(format, requestedCapabilities);
            }

            if (format != nearestFormat)
            {
                // Fallback to format that's closest to the original
                nearestFormat = GetNearestDeviceSupportedFormat(format, requestedCapabilities);

                AZ_Warning("RHI Utils", format == nearestFormat, "%s specifies format [%s], which is not supported by this device. Overriding to nearest supported format [%s]",
                    location, ToString(format), ToString(nearestFormat));
            }
            return nearestFormat;
        }
        return originalFormat;
    }
        
    bool IsNullRHI()
    {
        return RHI::Factory::Get().GetAPIUniqueIndex() == static_cast<uint32_t>(APIIndex::Null);
    }

    AZStd::string GetCommandLineValue(const AZStd::string& commandLineOption)
    {
        const AzFramework::CommandLine* commandLine = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetApplicationCommandLine);

        AZStd::string commandLineValue;
        if (commandLine)
        {
            if (size_t switchCount = commandLine->GetNumSwitchValues(commandLineOption); switchCount > 0)
            {
                commandLineValue = commandLine->GetSwitchValue(commandLineOption, switchCount - 1);
            }
        }
        return commandLineValue;
    }

    bool QueryCommandLineOption(const AZStd::string& commandLineOption)
    {
        const AzFramework::CommandLine* commandLine = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetApplicationCommandLine);

        if (commandLine)
        {
            return commandLine->HasSwitch(commandLineOption);
        }
        return false;
    }

    bool IsGraphicsDevModeEnabled()
    {
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        bool graphicsDevMode = false;
        if (settingsRegistry)
        {
            settingsRegistry->Get(graphicsDevMode, GraphicsDevModeSetting);
        }
        return graphicsDevMode;
    }

    const AZ::Name& GetDefaultSupervariantNameWithNoFloat16Fallback()
    {
        const static AZ::Name DefaultSupervariantName{ "" };
        const static AZ::Name NoFloat16SupervariantName{ "NoFloat16" };
        return GetRHIDevice()->GetFeatures().m_float16 ? DefaultSupervariantName : NoFloat16SupervariantName;
    }

    // Pool attributes
    using JsonStringRef = rapidjson::Value::StringRefType;
    const char PoolNameAttribStr[] = "PoolName";
    const char HostMemoryTypeValueStr[] = "Host";
    const char DeviceMemoryTypeValueStr[] = "Device";
    const char MemoryTypeAttribStr[] = "MemoryType";
    const char BudgetInBytesAttribStr[] = "BudgetInBytes";
    const char TotalResidentInBytesAttribStr[] = "TotalResidentInBytes";
    const char UsedResidentInBytesAttribStr[] = "UsedResidentInBytes";
    const char FragmentationAttribStr[] = "Fragmentation";
    const char UniqueAllocationsInBytesAttribStr[] = "UniqueAllocationsInBytes";
    const char BufferCountAttribStr[] = "BufferCount";
    const char ImageCountAttribStr[] = "ImageCount";
    const char BuffersListAttribStr[] = "BuffersList";
    const char ImagesListAttribStr[] = "ImagesList";

    // DeviceBuffer and DeviceImage attributes
    const char BufferNameAttribStr[] = "BufferName";
    const char ImageNameAttribStr[] = "ImageName";
    const char SizeInBytesAttribStr[] = "SizeInBytes";
    const char BindFlagsAttribStr[] = "BindFlags";

    // Top level attributes
    const char PoolsAttribStr[] = "Pools";
    const char MemoryDataVersionMajorAttribStr[] = "MemoryDataVersionMajor";
    const char MemoryDataVersionMinorAttribStr[] = "MemoryDataVersionMinor";
    const char MemoryDataVersionRevisionAttribStr[] = "MemoryDataVersionRevision";

    void WritePoolsToJson(const AZStd::vector<MemoryStatistics::Pool>& pools, rapidjson::Document& doc)
    {
        rapidjson::Value& docRoot = doc.SetObject();

        rapidjson::Value poolsArray(rapidjson::kArrayType);

        for (const auto& pool : pools)
        {
            rapidjson::Value poolObject(rapidjson::kObjectType);
            poolObject.AddMember(PoolNameAttribStr, rapidjson::StringRef(!pool.m_name.IsEmpty()?pool.m_name.GetCStr():"Unnamed Pool"), doc.GetAllocator());
            if (const auto& heapMemoryUsageHost = pool.m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Host); heapMemoryUsageHost.m_totalResidentInBytes > 0)
            {
                poolObject.AddMember(MemoryTypeAttribStr, HostMemoryTypeValueStr, doc.GetAllocator());
                poolObject.AddMember(BudgetInBytesAttribStr, static_cast<uint64_t>(heapMemoryUsageHost.m_budgetInBytes), doc.GetAllocator());
                poolObject.AddMember(TotalResidentInBytesAttribStr, static_cast<uint64_t>(heapMemoryUsageHost.m_totalResidentInBytes.load()), doc.GetAllocator());
                poolObject.AddMember(UsedResidentInBytesAttribStr, static_cast<uint64_t>(heapMemoryUsageHost.m_usedResidentInBytes.load()), doc.GetAllocator());
                poolObject.AddMember(FragmentationAttribStr, heapMemoryUsageHost.m_fragmentation, doc.GetAllocator());
                poolObject.AddMember(UniqueAllocationsInBytesAttribStr, static_cast<uint64_t>(heapMemoryUsageHost.m_uniqueAllocationBytes.load()), doc.GetAllocator());
            }
            else if (const auto& heapMemoryUsageDevice = pool.m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device); heapMemoryUsageDevice.m_totalResidentInBytes > 0)
            {
                poolObject.AddMember(MemoryTypeAttribStr, DeviceMemoryTypeValueStr, doc.GetAllocator());
                poolObject.AddMember(BudgetInBytesAttribStr, static_cast<uint64_t>(heapMemoryUsageDevice.m_budgetInBytes), doc.GetAllocator());
                poolObject.AddMember(TotalResidentInBytesAttribStr, static_cast<uint64_t>(heapMemoryUsageDevice.m_totalResidentInBytes.load()), doc.GetAllocator());
                poolObject.AddMember(UsedResidentInBytesAttribStr, static_cast<uint64_t>(heapMemoryUsageDevice.m_usedResidentInBytes.load()), doc.GetAllocator());
                poolObject.AddMember(FragmentationAttribStr, heapMemoryUsageDevice.m_fragmentation, doc.GetAllocator());
                poolObject.AddMember(UniqueAllocationsInBytesAttribStr, static_cast<uint64_t>(heapMemoryUsageDevice.m_uniqueAllocationBytes.load()), doc.GetAllocator());
            }
            else
            {
                continue;
            }

            poolObject.AddMember(BufferCountAttribStr, static_cast<uint64_t>(pool.m_buffers.size()), doc.GetAllocator());
            poolObject.AddMember(ImageCountAttribStr,  static_cast<uint64_t>(pool.m_images.size()),  doc.GetAllocator());
            rapidjson::Value buffersArray(rapidjson::kArrayType);
            for (const auto& buffer : pool.m_buffers)
            {
                rapidjson::Value bufferObject(rapidjson::kObjectType);
                bufferObject.AddMember(BufferNameAttribStr, rapidjson::StringRef(!buffer.m_name.IsEmpty()?buffer.m_name.GetCStr():"Unnamed DeviceBuffer"), doc.GetAllocator());
                bufferObject.AddMember(SizeInBytesAttribStr, static_cast<uint64_t>(buffer.m_sizeInBytes), doc.GetAllocator());
                bufferObject.AddMember(BindFlagsAttribStr, static_cast<uint32_t>(buffer.m_bindFlags), doc.GetAllocator());
                buffersArray.PushBack(bufferObject, doc.GetAllocator());
            }
            poolObject.AddMember(BuffersListAttribStr, buffersArray, doc.GetAllocator());

            rapidjson::Value imagesArray(rapidjson::kArrayType);
            for (const auto& image : pool.m_images)
            {
                rapidjson::Value imageObject(rapidjson::kObjectType);
                imageObject.AddMember(ImageNameAttribStr, rapidjson::StringRef(!image.m_name.IsEmpty()?image.m_name.GetCStr():"Unnamed DeviceImage"), doc.GetAllocator());
                imageObject.AddMember(SizeInBytesAttribStr, static_cast<uint64_t>(image.m_sizeInBytes), doc.GetAllocator());
                imageObject.AddMember(BindFlagsAttribStr, static_cast<uint32_t>(image.m_bindFlags), doc.GetAllocator());
                imagesArray.PushBack(imageObject, doc.GetAllocator());
            }
            poolObject.AddMember(ImagesListAttribStr, imagesArray, doc.GetAllocator());
            poolsArray.PushBack(poolObject, doc.GetAllocator());
        }
        docRoot.AddMember(PoolsAttribStr, poolsArray, doc.GetAllocator());
        docRoot.AddMember(MemoryDataVersionMajorAttribStr, 1, doc.GetAllocator());
        docRoot.AddMember(MemoryDataVersionMinorAttribStr, 0, doc.GetAllocator());
        docRoot.AddMember(MemoryDataVersionRevisionAttribStr, 0, doc.GetAllocator());
    }

    static int GetIntMember(const rapidjson::Value& object, const char* memberName, int defaultValue)
    {
        if (object.HasMember(memberName))
        {
            return object[memberName].GetInt();
        }
        return defaultValue;
    }

    AZ::Outcome<void, AZStd::string> LoadPoolsFromJson(
        AZStd::vector<MemoryStatistics::Pool>& pools, 
        AZStd::vector<AZ::RHI::MemoryStatistics::Heap>& heaps, 
        rapidjson::Document& doc,
        const AZStd::string& fileName)
    {
        // clear out any old data first
        heaps.clear();
        heaps.resize(2);
        heaps[0].m_name = AZ::Name{ "Host Heap" };
        heaps[0].m_heapMemoryType = RHI::HeapMemoryLevel::Host;
        heaps[1].m_name = AZ::Name{ "Device Heap" };
        heaps[1].m_heapMemoryType = RHI::HeapMemoryLevel::Device;

        pools.clear();

        if (!doc.IsObject() || !doc.HasMember(PoolsAttribStr) )
        {
            AZStd::string errorStr = AZStd::string::format("File %s doesn't contain saved memory state", fileName.c_str());
            AZ_Error("RHI", false, "RHI::LoadPoolsFromJson failed with error %s", errorStr.c_str());
            return AZ::Failure(errorStr);
        }

        rapidjson::Value& poolsData = doc[PoolsAttribStr];
        int dataVersionMajor = GetIntMember(doc, MemoryDataVersionMajorAttribStr, 1);
        int dataVersionMinor = GetIntMember(doc, MemoryDataVersionMinorAttribStr, 0);
        int dataVersionRevision = GetIntMember(doc, MemoryDataVersionRevisionAttribStr, 0);

        if (dataVersionMajor > 1 || dataVersionMinor > 0 || dataVersionRevision > 0)
        {
                AZStd::string errorStr = AZStd::string::format(
                    "File %s contains an unsupported data format (%d.%d.%d), expected (1.0.0)", 
                    fileName.c_str(), 
                    dataVersionMajor, 
                    dataVersionMinor, 
                    dataVersionRevision);
                AZ_Error("RHI", false, "RHI::LoadPoolsFromJson failed with error %s", errorStr.c_str());
                return AZ::Failure(errorStr);
        }
        else
        {
            if (!poolsData.IsArray())
            {
                AZStd::string errorStr = AZStd::string::format("File %s doesn't contain saved memory state", fileName.c_str());
                AZ_Error("RHI", false, "RHI::LoadPoolsFromJson failed with error %s", errorStr.c_str());
                return AZ::Failure(errorStr);
            }
                
            pools.reserve(poolsData.Size() + pools.size());
            for (auto poolItr = poolsData.Begin(); poolItr != poolsData.End(); ++poolItr)
            {
                if (!poolItr->IsObject())
                {
                    AZStd::string errorStr = AZStd::string::format(
                        "Attempted to load memory data from %s but a parse error occurred (indicating invalid file "
                        "format)",
                        fileName.c_str());
                    AZ_Error("RHI", false, "RHI::LoadPoolsFromJson failed with error %s", errorStr.c_str());
                    return AZ::Failure(errorStr);
                }
                rapidjson::Value& poolData = *poolItr;
                RHI::MemoryStatistics::Pool pool;
                pool.m_name = poolData[PoolNameAttribStr].GetString();
                AZStd::string poolHeapType = poolData[MemoryTypeAttribStr].GetString();
                RHI::HeapMemoryUsage* heapMemUsage = nullptr;
                RHI::MemoryStatistics::Heap* globalHeapStats = nullptr;
                if (poolHeapType == "Host")
                {
                    heapMemUsage = &pool.m_memoryUsage.GetHeapMemoryUsage(AZ::RHI::HeapMemoryLevel::Host);
                    globalHeapStats = &heaps[0];
                }
                else if (poolHeapType == "Device")
                {
                    heapMemUsage = &pool.m_memoryUsage.GetHeapMemoryUsage(AZ::RHI::HeapMemoryLevel::Device);
                    globalHeapStats = &heaps[1];
                }
                else
                {
                    AZStd::string errorStr = AZStd::string::format(
                        "Attempted to load memory data from %s but a parse error occurred (indicating invalid file "
                        "format) at pool %s",
                        fileName.c_str(), pool.m_name.GetCStr());
                    AZ_Error("RHI", false, "RHI::LoadPoolsFromJson failed with error %s", errorStr.c_str());
                    return AZ::Failure(errorStr);
                }
                heapMemUsage->m_budgetInBytes = poolData[BudgetInBytesAttribStr].GetUint64();
                heapMemUsage->m_totalResidentInBytes = poolData[TotalResidentInBytesAttribStr].GetUint64();
                heapMemUsage->m_usedResidentInBytes = poolData[UsedResidentInBytesAttribStr].GetUint64();
                heapMemUsage->m_fragmentation = poolData[FragmentationAttribStr].GetFloat();
                heapMemUsage->m_uniqueAllocationBytes = poolData[UniqueAllocationsInBytesAttribStr].GetUint64();

                globalHeapStats->m_memoryUsage.m_totalResidentInBytes += heapMemUsage->m_totalResidentInBytes;
                globalHeapStats->m_memoryUsage.m_usedResidentInBytes += heapMemUsage->m_usedResidentInBytes;

                rapidjson::Value& buffersList = poolData[BuffersListAttribStr];
                if (!buffersList.IsArray())
                {
                    AZStd::string errorStr = AZStd::string::format(
                        "Attempted to load memory data from %s but a parse error occurred (indicating invalid file "
                        "format) at pool %s",
                        fileName.c_str(), pool.m_name.GetCStr());
                    AZ_Error("RHI", false, "RHI::LoadPoolsFromJson failed with error %s", errorStr.c_str());
                    return AZ::Failure(errorStr);
                }
                pool.m_buffers.reserve(buffersList.Size());
                for (auto bufferItr = buffersList.Begin(); bufferItr != buffersList.End(); ++bufferItr)
                {
                    if (!bufferItr->IsObject())
                    {
                        AZStd::string errorStr = AZStd::string::format(
                            "Attempted to load buffer memory data from %s but a parse error occurred (indicating invalid file "
                            "format) at pool %s",
                            fileName.c_str(), pool.m_name.GetCStr());
                        AZ_Error("RHI", false, "RHI::LoadPoolsFromJson failed with error %s", errorStr.c_str());
                        return AZ::Failure(errorStr);
                    }
                    rapidjson::Value& bufferData = *bufferItr;
                    RHI::MemoryStatistics::Buffer buffer;
                    buffer.m_name = bufferData[BufferNameAttribStr].GetString();
                    buffer.m_sizeInBytes = bufferData[SizeInBytesAttribStr].GetUint64();
                    buffer.m_bindFlags = static_cast<RHI::BufferBindFlags>(bufferData[BindFlagsAttribStr].GetUint());
                    pool.m_buffers.push_back(AZStd::move(buffer));
                }

                rapidjson::Value& imagesList = poolData[ImagesListAttribStr];
                if (!imagesList.IsArray())
                {
                    AZStd::string errorStr = AZStd::string::format(
                        "Attempted to load memory data from %s but a parse error occurred (indicating invalid file "
                        "format) at pool %s",
                        fileName.c_str(), pool.m_name.GetCStr());
                    AZ_Error("RHI", false, "RHI::LoadPoolsFromJson failed with error %s", errorStr.c_str());
                    return AZ::Failure(errorStr);
                }
                pool.m_images.reserve(imagesList.Size());
                for (auto imageItr = imagesList.Begin(); imageItr != imagesList.End(); ++imageItr)
                {
                    if (!imageItr->IsObject())
                    {
                        AZStd::string errorStr = AZStd::string::format(
                            "Attempted to load buffer memory data from %s but a parse error occurred (indicating invalid file "
                            "format) at pool %s",
                            fileName.c_str(), pool.m_name.GetCStr());
                        AZ_Error("RHI", false, "RHI::LoadPoolsFromJson failed with error %s", errorStr.c_str());
                        return AZ::Failure(errorStr);
                    }
                    rapidjson::Value& imageData = *imageItr;
                    RHI::MemoryStatistics::Image image;
                    image.m_name = imageData[ImageNameAttribStr].GetString();
                    image.m_sizeInBytes = imageData[SizeInBytesAttribStr].GetUint64();
                    image.m_bindFlags = static_cast<RHI::ImageBindFlags>(imageData[BindFlagsAttribStr].GetUint());
                    pool.m_images.push_back(AZStd::move(image));
                }

                pools.push_back(AZStd::move(pool));
            }
        }
        return AZ::Success();
    }


    //! Utility func to dump all resource pool gpu mem allocations to a json file in the 
    void DumpPoolInfoToJson()
    {
        AZ::IO::Path path = AZStd::string_view(AZ::Utils::GetO3deLogsDirectory());

        path /= "MemoryCaptures";

        [[maybe_unused]] const bool dirCreated = AZ::IO::SystemFile::CreateDir(path.c_str());
        AZ_Assert(dirCreated, "Failed to create a MemoryCaptures directory for gpu pool dump, requested path = %s", path.c_str())

        time_t ltime;
        time(&ltime);
        tm today;
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
        localtime_s(&today, &ltime);
#else
        today = *localtime(&ltime);
#endif
        char gpuMemDumpFileName[128];
        strftime(gpuMemDumpFileName, sizeof(gpuMemDumpFileName), "%Y-%m-%d.%H-%M-%S", &today);
        AZStd::string filename = AZStd::string::format("%s/GpuMemoryLog_%s.%ld.json", path.c_str(), gpuMemDumpFileName, static_cast<long>(ltime));

        AZ::IO::SystemFile outputFile;
        if (!outputFile.Open(filename.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            AZ_Error("RHI", false, AZStd::string::format("Failed to open file %s for writing", filename.c_str()).c_str());
            return;
        }

        rapidjson::Document doc;

        auto* rhiMemStats = AZ::RHI::RHIMemoryStatisticsInterface::Get();
        const auto* memoryStatistics = rhiMemStats->GetMemoryStatistics();
        if (!memoryStatistics)
        {
            AZ_Error("RHI", false, AZStd::string::format("Failed to capture RHI gpu memory statistics").c_str());
            return;
        }
        // this duplicates the memory pool data so it's not being modified as we try to output it.
        AZStd::vector<RHI::MemoryStatistics::Pool> pools = memoryStatistics->m_pools;

        WritePoolsToJson(pools, doc);

        rapidjson::StringBuffer jsonStringBuffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(jsonStringBuffer);
        doc.Accept(writer);

        outputFile.Write(jsonStringBuffer.GetString(), jsonStringBuffer.GetSize());
        outputFile.Close();
    }

    RHI::DrawListTagRegistry* GetDrawListTagRegistry()
    {
        RHI::DrawListTagRegistry* drawListTagRegistry = RHI::RHISystemInterface::Get()->GetDrawListTagRegistry();
        return drawListTagRegistry;
    }

    Name GetDrawListName(DrawListTag drawListTag)
    {
        RHI::DrawListTagRegistry* drawListTagRegistry = GetDrawListTagRegistry();
        return drawListTagRegistry->GetName(drawListTag);
    }

    AZStd::string DrawListMaskToString(const RHI::DrawListMask& drawListMask)
    {
        AZStd::string tagString;
        u32 maxTags = RHI::Limits::Pipeline::DrawListTagCountMax;

        u32 drawListTagCount = 0;

        for (u32 i = 0; i < maxTags; ++i)
        {
            if (drawListMask[i])
            {
                DrawListTag tag(i);
                tagString += AZStd::string::format("%s | ", GetDrawListName(tag).GetCStr());
                ++drawListTagCount;
            }
        }

        AZStd::string output = AZStd::string::format("DrawListMask has %d tags = %s", drawListTagCount, tagString.c_str());
        return output;
    }

}
