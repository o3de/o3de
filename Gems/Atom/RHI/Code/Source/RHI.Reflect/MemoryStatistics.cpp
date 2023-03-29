#include <Atom/RHI.Reflect/MemoryStatistics.h>

#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Utils/Utils.h>

#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    // POOL attributes
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

    // Buffer and Image attributes
    const char BufferNameAttribStr[] = "BufferName";
    const char ImageNameAttribStr[] = "ImageName";
    const char SizeInBytesAttribStr[] = "SizeInBytes";
    const char BindFlagsAttribStr[] = "BindFlags";

    // Top level attributes
    const char PoolsAttribStr[] = "Pools";
    const char MemoryDataVersionMajorAttribStr[] = "MemoryDataVersionMajor";
    const char MemoryDataVersionMinorAttribStr[] = "MemoryDataVersionMinor";
    const char MemoryDataVersionRevisionAttribStr[] = "MemoryDataVersionRevision";

    void WritePoolsToJson(AZStd::vector<MemoryStatistics::Pool>& pools, rapidjson::Document& doc, rapidjson::Value& docRoot)
    {
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
                poolObject.AddMember(MemoryTypeAttribStr,               DeviceMemoryTypeValueStr, doc.GetAllocator());
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
                bufferObject.AddMember(BufferNameAttribStr, rapidjson::StringRef(!buffer.m_name.IsEmpty()?buffer.m_name.GetCStr():"Unnamed Buffer"), doc.GetAllocator());
                bufferObject.AddMember(SizeInBytesAttribStr, static_cast<uint64_t>(buffer.m_sizeInBytes), doc.GetAllocator());
                bufferObject.AddMember(BindFlagsAttribStr, static_cast<uint32_t>(buffer.m_bindFlags), doc.GetAllocator());
                buffersArray.PushBack(bufferObject, doc.GetAllocator());
            }
            poolObject.AddMember(BuffersListAttribStr, buffersArray, doc.GetAllocator());

            rapidjson::Value imagesArray(rapidjson::kArrayType);
            for (const auto& image : pool.m_images)
            {
                rapidjson::Value imageObject(rapidjson::kObjectType);
                imageObject.AddMember(ImageNameAttribStr, rapidjson::StringRef(!image.m_name.IsEmpty()?image.m_name.GetCStr():"Unnamed Image"), doc.GetAllocator());
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
            char sTemp[128];
            strftime(sTemp, sizeof(sTemp), "%Y-%m-%d.%H-%M-%S", &today);
            AZStd::string filename = AZStd::string::format("%s/GpuMemoryLog_%s.%" PRIu64 ".json", path.c_str(), sTemp, ltime);

            AZ::IO::SystemFile outputFile;
            if (!outputFile.Open(filename.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
            {
                AZ_Error("RHI", false, AZStd::string::format("Failed to open file %s for writing", filename.c_str()).c_str());
                return;
            }

            rapidjson::Document doc;
            rapidjson::Value& root = doc.SetObject();

            auto* rhiSystem = AZ::RHI::RHISystemInterface::Get();
            const auto* memoryStatistics = rhiSystem->GetMemoryStatistics();
            if (!memoryStatistics)
            {
                AZ_Error("RHI", false, AZStd::string::format("Failed to capture RHI gpu memory statistics").c_str());
                return;
            }
            // this duplicates the memory pool data so it's not being modifies as we try to output it.
            AZStd::vector<MemoryStatistics::Pool> pools = memoryStatistics->m_pools;

            WritePoolsToJson(pools, doc, root);

            rapidjson::StringBuffer jsonStringBuffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(jsonStringBuffer);
            doc.Accept(writer);

            outputFile.Write(jsonStringBuffer.GetString(), jsonStringBuffer.GetSize());
            outputFile.Close();

    }
}
