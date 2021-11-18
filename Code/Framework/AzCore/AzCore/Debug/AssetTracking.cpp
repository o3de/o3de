/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetTracking.h"

#include <AzCore/Debug/AssetTrackingTypes.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/HphaSchema.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::Debug
{
    namespace
    {
        struct AssetTreeNode;

        // Per-thread data that needs to be stored.
        struct ThreadData
        {
            AZStd::vector<AssetTreeNodeBase*, AZStdAssetTrackingAllocator> m_currentAssetStack;
        };

        // Access thread data through a virtual function to ensure that the same thread-local data is being shared across DLLs.
        // Otherwise, the thread_local variables are replicated across DLLs that link the AzCore library, and you'll get a
        // different version in each module.
        class ThreadDataProvider
        {
        public:
            virtual ThreadData& GetThreadData() = 0;
        };
    }

    class AssetTrackingImpl final :
        public ThreadDataProvider
    {
    public:
        AZ_TYPE_INFO(AssetTrackingImpl, "{01E2A099-3523-40BE-80E0-E0ADD861BEE1}");
        AZ_CLASS_ALLOCATOR(AssetTrackingImpl, OSAllocator, 0);

        AssetTrackingImpl(AssetTreeBase* assetTree, AssetAllocationTableBase* allocationTable);
        ~AssetTrackingImpl();

        void AssetBegin(const char* id, const char* file, int line);
        void AssetAttach(void* otherAllocation, const char* file, int line);
        void AssetEnd();

        ThreadData& GetThreadData() override;

    private:
        static EnvironmentVariable<AssetTrackingImpl*>& GetEnvironmentVariable();
        static AssetTrackingImpl* GetSharedInstance();
        static ThreadData& GetSharedThreadData();

        using PrimaryAssets = AZStd::unordered_map<AssetTrackingId, AssetPrimaryInfo, AZStd::hash<AssetTrackingId>, AZStd::equal_to<AssetTrackingId>, AZStdAssetTrackingAllocator>;
        using ThreadData = ThreadData;
        using mutex_type = AZStd::mutex;
        using lock_type = AZStd::lock_guard<mutex_type>;

        mutex_type m_mutex;
        PrimaryAssets m_primaryAssets;
        AssetTreeNodeBase* m_assetRoot = nullptr;
        AssetAllocationTableBase* m_allocationTable = nullptr;
        bool m_performingAnalysis = false;

        friend class AssetTracking;
        friend class AssetTracking::Scope;
    };


///////////////////////////////////////////////////////////////////////////////
// AssetTrackingImpl methods
///////////////////////////////////////////////////////////////////////////////

    AssetTrackingImpl::AssetTrackingImpl(AssetTreeBase* assetTree, AssetAllocationTableBase* allocationTable) :
        m_assetRoot(&assetTree->GetRoot()),
        m_allocationTable(allocationTable)
    {
        AZ_Assert(!GetSharedInstance(), "Only one AssetTrackingImpl can exist!");

        GetEnvironmentVariable().Set(this);
        AllocatorManager::Instance().EnterProfilingMode();
    }

    AssetTrackingImpl::~AssetTrackingImpl()
    {
        AllocatorManager::Instance().ExitProfilingMode();
        GetEnvironmentVariable().Reset();
    }

    void AssetTrackingImpl::AssetBegin(const char* id, const char* file, int line)
    {
        // In the future it may be desirable to organize assets based on where in code the asset was entered into.
        // For now these are ignored.
        AZ_UNUSED(file);
        AZ_UNUSED(line);

        using namespace Internal;

        AssetTrackingId assetId(id);
        auto& threadData = GetSharedThreadData();
        AssetTreeNodeBase* parentAsset = threadData.m_currentAssetStack.empty() ? nullptr : threadData.m_currentAssetStack.back();
        AssetTreeNodeBase* childAsset;
        AssetPrimaryInfo* assetPrimaryInfo;

        if (!parentAsset)
        {
            parentAsset = m_assetRoot;
        }

        {
            lock_type lock(m_mutex);

            // Locate or create the primary record for this asset
            auto primaryItr = m_primaryAssets.find(assetId);

            if (primaryItr != m_primaryAssets.end())
            {
                assetPrimaryInfo = &primaryItr->second;
            }
            else
            {
                auto insertResult = m_primaryAssets.emplace(assetId, AssetPrimaryInfo());
                assetPrimaryInfo = &insertResult.first->second;
                assetPrimaryInfo->m_id = &insertResult.first->first;
            }

            // Add this asset to the stack for this thread's context
            childAsset = parentAsset->FindOrAddChild(assetId, assetPrimaryInfo);
        }

        threadData.m_currentAssetStack.push_back(childAsset);
    }

    void AssetTrackingImpl::AssetAttach(void* otherAllocation, const char* file, int line)
    {
        AZ_UNUSED(file);
        AZ_UNUSED(line);

        using namespace Internal;

        AssetTreeNodeBase* assetInfo = m_allocationTable->FindAllocation(otherAllocation);

        // We will push back a nullptr if there is no asset, this is necessary to balance the call to AssetEnd()
        GetSharedThreadData().m_currentAssetStack.push_back(assetInfo);
    }

    void AssetTrackingImpl::AssetEnd()
    {
        AZ_Assert(!GetSharedThreadData().m_currentAssetStack.empty(), "AssetEnd() called without matching AssetBegin() or AssetAttach. Use the AZ_ASSET_NAMED_SCOPE and AZ_ASSET_ATTACH_TO_SCOPE macros to avoid this!");
        GetSharedThreadData().m_currentAssetStack.pop_back();
    }

    AssetTrackingImpl* AssetTrackingImpl::GetSharedInstance()
    {
        auto environmentVariable = GetEnvironmentVariable();

        if(environmentVariable)
        {
            return *environmentVariable;
        }

        return nullptr;
    }

    ThreadData& AssetTrackingImpl::GetSharedThreadData()
    {
        // Cast to the base type so our virtual call doesn't get optimized away. We require GetThreadData() to be executed in the same DLL every time.
        return static_cast<ThreadDataProvider*>(GetSharedInstance())->GetThreadData();
    }

    AssetTrackingImpl::ThreadData& AssetTrackingImpl::GetThreadData()
    {
        static thread_local ThreadData* data = nullptr;
        static thread_local typename AZStd::aligned_storage_t<sizeof(ThreadData), alignof(ThreadData)> storage;

        if (!data)
        {
            data = new (&storage) ThreadData;
        }

        return *data;
    }

    EnvironmentVariable<AssetTrackingImpl*>& AssetTrackingImpl::GetEnvironmentVariable()
    {
        static EnvironmentVariable<AssetTrackingImpl*> assetTrackingImpl = Environment::CreateVariable<AssetTrackingImpl*>(AzTypeInfo<AssetTrackingImpl*>::Name());

        return assetTrackingImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // AssetTracking::Scope functions
    ///////////////////////////////////////////////////////////////////////////////

    AssetTracking::Scope AssetTracking::Scope::ScopeFromAssetId(const char* file, int line, const char* fmt, ...)
    {
        if (auto impl = AssetTrackingImpl::GetSharedInstance())
        {
            static const int BUFFER_SIZE = 1024;

            char buffer[BUFFER_SIZE];
            va_list args;
            va_start(args, fmt);
            azvsnprintf(buffer, BUFFER_SIZE, fmt, args);
            va_end(args);

            impl->AssetBegin(buffer, file, line);
        }

        return Scope();
    }

    AssetTracking::Scope AssetTracking::Scope::ScopeFromAttachment(void* attachTo, const char* file, int line)
    {
        if (auto impl = AssetTrackingImpl::GetSharedInstance())
        {
            impl->AssetAttach(attachTo, file, line);
        }

        return Scope();
    }

    AssetTracking::Scope::~Scope()
    {
        if (auto impl = AssetTrackingImpl::GetSharedInstance())
        {
            impl->AssetEnd();
        }
    }

    AssetTracking::Scope::Scope()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////
    // AssetTracking functions
    ///////////////////////////////////////////////////////////////////////////////

    void AssetTracking::EnterScopeByAssetId(const char* file, int line, const char* fmt, ...)
    {
        if (auto impl = AssetTrackingImpl::GetSharedInstance())
        {
            static const int BUFFER_SIZE = 1024;

            char buffer[BUFFER_SIZE];
            va_list args;
            va_start(args, fmt);
            azvsnprintf(buffer, BUFFER_SIZE, fmt, args);
            va_end(args);

            impl->AssetBegin(buffer, file, line);
        }
    }

    void AssetTracking::EnterScopeByAttachment(void* attachTo, const char* file, int line)
    {
        if (auto impl = AssetTrackingImpl::GetSharedInstance())
        {
            impl->AssetAttach(attachTo, file, line);
        }
    }

    void AssetTracking::ExitScope()
    {
        if (auto impl = AssetTrackingImpl::GetSharedInstance())
        {
            impl->AssetEnd();
        }
    }

    const char* AssetTracking::GetDebugScope()
    {
        // Output debug information about the current asset scope in the current thread.
        // Do not use in production code.
#ifndef RELEASE
        static const int BUFFER_SIZE = 1024;
        static char buffer[BUFFER_SIZE];
        const auto& assetStack = AssetTrackingImpl::GetSharedInstance()->GetThreadData().m_currentAssetStack;

        if (assetStack.empty())
        {
            azsnprintf(buffer, BUFFER_SIZE, "<none>");
        }
        else
        {
            char* pos = buffer;
            for (auto itr = assetStack.rbegin(); itr != assetStack.rend(); ++itr)
            {
                pos += azsnprintf(pos, BUFFER_SIZE - (pos - buffer), "%s\n", (*itr)->GetAssetPrimaryInfo()->m_id->m_id.c_str());

                if (pos >= buffer + BUFFER_SIZE)
                {
                    break;
                }
            }
        }

        return buffer;
#else
        return "";
#endif
    }

    AssetTracking::AssetTracking(AssetTreeBase* assetTree, AssetAllocationTableBase* allocationTable)
    {
        m_impl.reset(aznew AssetTrackingImpl(assetTree, allocationTable));
    }

    AssetTracking::~AssetTracking()
    {
    }

    AssetTreeNodeBase* AssetTracking::GetCurrentThreadAsset() const
    {
        const auto& assetStack = m_impl->GetThreadData().m_currentAssetStack;
        AssetTreeNodeBase* result = assetStack.empty() ? nullptr : assetStack.back();

        return result;
    }
} // namespace AzFramework
