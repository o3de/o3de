/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TestCatalog.h"

#include <algorithm>
#include <AzCore/IO/SystemFile.h>
#include <AZTestShared/Utils/Utils.h>

namespace UnitTest
{
    AssetDefinition* AssetDefinition::AddNoLoad(const Uuid& id)
    {
        ProductDependency dependency;

        dependency.m_assetId = AssetId(id, 0);
        dependency.m_flags = ProductDependencyInfo::CreateFlags(AssetLoadBehavior::NoLoad);

        m_noLoadDependencies.push_back(AZStd::move(dependency));

        return this;
    }

    AssetDefinition* AssetDefinition::AddPreload(const Uuid& id)
    {
        m_preloadDependencies.push_back(ProductDependency(AssetId(id, 0), {}));

        return this;
    }

    AssetDefinition* AssetDefinition::AddQueueLoad(const Uuid& id)
    {
        m_queueLoadDependencies.push_back(ProductDependency(AssetId(id, 0), {}));

        return this;
    }

    DataDrivenHandlerAndCatalog::DataDrivenHandlerAndCatalog()
    {
        BusConnect();
    }

    DataDrivenHandlerAndCatalog::~DataDrivenHandlerAndCatalog()
    {
        BusDisconnect();
    }

    AssetPtr DataDrivenHandlerAndCatalog::CreateAsset([[maybe_unused]] const AssetId& id, const AssetType& type)
    {
        AZ_Assert(m_context, "Serialization context must be set");

        if (m_createDelay > 0)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(m_createDelay));
        }

        for (auto&& definition : m_assetDefinitions)
        {
            if (definition.m_type == type)
            {
                ++m_numCreations;
                const auto* classData = m_context->FindClassData(definition.m_type);

                return static_cast<AssetPtr>(classData->m_factory->Create("Unit Test Object"));
            }
        }

        return nullptr;
    }

    AssetHandler::LoadResult DataDrivenHandlerAndCatalog::LoadAssetData(const Asset<AssetData>& asset, AZStd::shared_ptr<AssetDataStream> stream, const AssetFilterCB& assetLoadFilterCB)
    {
        AssetData* data = asset.Get();
        const auto* definition = FindById(asset.GetId());

        if (definition->m_loadSynchronizer)
        {
            // Keep track of how many LoadAssetData calls have reached the blocking point.
            auto loadSynchronizer = definition->m_loadSynchronizer;
            loadSynchronizer->m_numBlocking++;

            // Wait until the unit test has signaled that the jobs can proceed forward.
            AZStd::unique_lock<AZStd::mutex> lk(loadSynchronizer->m_conditionMutex);
            loadSynchronizer->m_condition.wait(lk, [&loadSynchronizer] { return loadSynchronizer->m_readyToLoad; });
        }

        ++m_numLoads;

        size_t assetLoadDelay = definition ? definition->m_loadDelay : 0;
        size_t loadDelay = m_loadDelay + assetLoadDelay;

        if (loadDelay > 0)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(loadDelay));
        }

        if(m_failLoad)
        {
            return LoadResult::Error;
        }

        return Utils::LoadObjectFromStreamInPlace(*stream, m_context, data->RTTI_GetType(),
                                                  data, ObjectStream::FilterDescriptor(assetLoadFilterCB))
                   ? LoadResult::LoadComplete
                   : LoadResult::Error;
    }

    bool DataDrivenHandlerAndCatalog::SaveAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream)
    {
        return Utils::SaveObjectToStream(*stream, DataStream::ST_XML, asset.Get(), asset->RTTI_GetType(), m_context);
    }

    void DataDrivenHandlerAndCatalog::DestroyAsset(AssetPtr ptr)
    {
        ++m_numDestructions;
        delete ptr;
    }

    void DataDrivenHandlerAndCatalog::GetHandledAssetTypes(AZStd::vector<AssetType>& assetTypes)
    {
        AZStd::unordered_set<AssetType> typeSet;

        for (auto& definition : m_assetDefinitions)
        {
            if (!definition.m_noHandler)
            {
                if(typeSet.insert(definition.m_type).second)
                {
                    assetTypes.push_back(definition.m_type);
                }
            }
        }
    }

    void DataDrivenHandlerAndCatalog::GetDefaultAssetLoadPriority([[maybe_unused]] AssetType type,
        AZStd::chrono::milliseconds& defaultDeadline, AZ::IO::IStreamerTypes::Priority& defaultPriority) const
    {
        defaultDeadline = m_defaultDeadline;
        defaultPriority = m_defaultPriority;
    }

    const char* DataDrivenHandlerAndCatalog::GetStreamName(const AssetId& id)
    {
        const auto* def = FindById(id);

        if (def)
        {
            return def->m_fileName.c_str();
        }

        return "";
    }

    AssetStreamInfo DataDrivenHandlerAndCatalog::GetStreamInfoForLoad(const AssetId& id, const AssetType&)
    {
        AssetStreamInfo info;
        info.m_dataOffset = 0;
        info.m_streamFlags = IO::OpenMode::ModeRead;

        info.m_streamName = GetStreamName(id);

        if (!info.m_streamName.empty())
        {
            AZStd::string fullName = GetTestFolderPath() + info.m_streamName;
            IO::FileIOBase* io = IO::FileIOBase::GetInstance();
            io->Size(fullName.c_str(), info.m_dataLen);
        }
        else
        {
            info.m_dataLen = 0;
        }

        return info;
    }

    AssetStreamInfo DataDrivenHandlerAndCatalog::GetStreamInfoForSave(const AssetId& id, const AssetType&)
    {
        AssetStreamInfo info;
        info.m_dataOffset = 0;
        info.m_streamFlags = IO::OpenMode::ModeWrite;

        info.m_streamName = GetStreamName(id);

        if (!info.m_streamName.empty())
        {
            IO::FileIOBase* io = AZ::IO::FileIOBase::GetInstance();

            AZStd::string fullName = GetTestFolderPath() + info.m_streamName;

            io->Size(fullName.c_str(), info.m_dataLen);
        }
        else
        {
            info.m_dataLen = 0;
        }

        return info;
    }

    void DataDrivenHandlerAndCatalog::AddDependenciesHelper(const AZStd::vector<ProductDependency>& list, AZStd::vector<ProductDependency>& listOutput)
    {
        for (auto&& dependency : list)
        {
            if (std::find_if(listOutput.begin(), listOutput.end(), [dependency](const ProductDependency& productDependency)
                {
                    return dependency.m_assetId == productDependency.m_assetId;
                }) == listOutput.end())
            {
                listOutput.push_back(dependency);

                if (const auto* dependencyDefinition = FindById(dependency.m_assetId); dependencyDefinition)
                {
                    AddDependencies(dependencyDefinition, listOutput);
                }
            }
        }
    }

    void DataDrivenHandlerAndCatalog::AddDependencies(const AssetDefinition* def, AZStd::vector<ProductDependency>& listOutput)
    {
        AddDependenciesHelper(def->m_preloadDependencies, listOutput);
        AddDependenciesHelper(def->m_queueLoadDependencies, listOutput);
    }

    void DataDrivenHandlerAndCatalog::AddPreloads(const AssetDefinition* def, PreloadAssetListType& preloadList)
    {
        for (auto&& dependency : def->m_preloadDependencies)
        {
            if (preloadList[def->m_assetId].find(dependency.m_assetId) == preloadList[def->m_assetId].end())
            {
                preloadList[def->m_assetId].insert(dependency.m_assetId);

                if (const auto* dependencyDefinition = FindById(dependency.m_assetId); dependencyDefinition)
                {
                    AddPreloads(dependencyDefinition, preloadList);
                }
            }
        }

        for(auto&& dependency : def->m_queueLoadDependencies)
        {
            if (const auto* dependencyDefinition = FindById(dependency.m_assetId); dependencyDefinition)
            {
                AddPreloads(dependencyDefinition, preloadList);
            }
        }
    }

    void DataDrivenHandlerAndCatalog::AddNoLoads(const AssetDefinition* def, AZStd::unordered_set<AssetId>& noloadSet)
    {
        for (auto&& dependency : def->m_noLoadDependencies)
        {
            if (noloadSet.insert(dependency.m_assetId).second)
            {
                if (const auto* dependencyDefinition = FindById(dependency.m_assetId); dependencyDefinition)
                {
                    AddNoLoads(dependencyDefinition, noloadSet);
                }
            }
        }
    }

    void DataDrivenHandlerAndCatalog::SetArtificialDelayMilliseconds(size_t createDelay, size_t loadDelay)
    {
        m_createDelay = createDelay;
        m_loadDelay = loadDelay;
    }

    Outcome<AZStd::vector<ProductDependency>, AZStd::string> DataDrivenHandlerAndCatalog::GetAllProductDependencies(const AssetId& assetId)
    {
        const auto* def = FindById(assetId);

        if (def)
        {
            AZStd::vector<ProductDependency> dependencyList;

            AddDependencies(def, dependencyList);
            AddDependenciesHelper(def->m_noLoadDependencies, dependencyList);

            return Success(dependencyList);
        }

        return AZ::Failure<AZStd::string>("Unknown asset");
    }

    Outcome<AZStd::vector<ProductDependency>, AZStd::string> DataDrivenHandlerAndCatalog::GetLoadBehaviorProductDependencies(const AssetId& assetId, AZStd::unordered_set<AssetId>& noloadSet, PreloadAssetListType& preloadList)
    {
        const auto* def = FindById(assetId);

        if (def)
        {
            AZStd::vector<ProductDependency> dependencyList;

            AddDependencies(def, dependencyList);
            AddNoLoads(def, noloadSet);
            AddPreloads(def, preloadList);

            return Success(dependencyList);
        }

        return AZ::Failure<AZStd::string>("Unknown asset");
    }

    AssetInfo DataDrivenHandlerAndCatalog::GetAssetInfoById(const AssetId& assetId)
    {
        AssetInfo result;
        const auto* def = FindById(assetId);

        if (def && !def->m_noAssetData)
        {
            result.m_assetId = def->m_assetId;
            result.m_assetType = def->m_type;
            result.m_relativePath = "ContainerAssetInfo";
        }

        return result;
    }

    const AssetDefinition* DataDrivenHandlerAndCatalog::FindByType(const Uuid& type)
    {
        for (auto&& def : m_assetDefinitions)
        {
            if (def.m_type == type)
            {
                return &def;
            }
        }

        return nullptr;
    }

    const AssetDefinition* DataDrivenHandlerAndCatalog::FindById(const AssetId& id)
    {
        for (auto&& def : m_assetDefinitions)
        {
            if (def.m_assetId == id)
            {
                return &def;
            }
        }

        return nullptr;
    }
}
