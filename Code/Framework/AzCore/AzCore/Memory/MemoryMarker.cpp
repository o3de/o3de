/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(AZ_ENABLE_TRACING) && defined(AZ_MEMORY_TAG_TRACING) && !defined(_RELEASE) && defined(CARBONATED)

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/MemoryMarker.h>

namespace AZ
{
    void MemoryAllocationMarker::Init(const char* name, const char* file, int line, bool isLiteral)
    {
        AllocatorManager::Instance().PushMemoryMarker(AZ::AllocatorManager::CodePoint(name, file, line, isLiteral));
    }
    MemoryAllocationMarker::~MemoryAllocationMarker()
    {
        AllocatorManager::Instance().PopMemoryMarker();
    }

    MemoryTagMarker::MemoryTagMarker(MemoryTagValue v)
    {
        AllocatorManager::Instance().PushMemoryTag((int)v);
    }
    MemoryTagMarker::~MemoryTagMarker()
    {
        AllocatorManager::Instance().PopMemoryTag();
    }

    AssetMemoryTagMarker::AssetMemoryTagMarker(const char* name)
    {
        AllocatorManager::Instance().PushAssetMemoryTag(name);
    }
    AssetMemoryTagMarker::~AssetMemoryTagMarker()
    {
        AllocatorManager::Instance().PopAssetMemoryTag();
    }

    AssetMemoryTagLimit::AssetMemoryTagLimit(size_t limit)
    {
        AllocatorManager::Instance().SetAssetMemoryLimit(limit);
    }
    AssetMemoryTagLimit::~AssetMemoryTagLimit()
    {
        AllocatorManager::Instance().SetAssetMemoryLimit(0);
    }
    void AssetMemoryTagLimit::SetLimit(size_t limit)
    {
        AllocatorManager::Instance().SetAssetMemoryLimit(limit);
    }
    size_t AssetMemoryTagLimit::GetLimit()
    {
        return AllocatorManager::Instance().GetAssetMemoryLimit();
    }


} // namespace AZ

#endif  // AZ_ENABLE_TRACING && AZ_MEMORY_TAG_TRACING && !_RELEASE CARBONATED
