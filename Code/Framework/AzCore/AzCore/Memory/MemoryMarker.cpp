/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(AZ_ENABLE_TRACING) && !defined(_RELEASE) && defined(CARBONATED)  // without tracing definition engine's memory tracking is off

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/MemoryMarker.h>

void AZ::MemoryAllocationMarker::Init(const char* name, const char* file, int line, bool isLiteral)
{
    AZ::AllocatorManager::Instance().PushMemoryMarker(AZ::AllocatorManager::CodePoint(name, file, line, isLiteral));
}
AZ::MemoryAllocationMarker::~MemoryAllocationMarker()
{
    AZ::AllocatorManager::Instance().PopMemoryMarker();
}

AZ::MemoryTagMarker::MemoryTagMarker(MemoryTagValue v)
{
    AZ::AllocatorManager::Instance().PushMemoryTag((int)v);
}
AZ::MemoryTagMarker::~MemoryTagMarker()
{
    AZ::AllocatorManager::Instance().PopMemoryTag();
}

#endif  // AZ_ENABLE_TRACING  && !_RELEASE && CARBONATED
