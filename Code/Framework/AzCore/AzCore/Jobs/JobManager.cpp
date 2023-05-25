/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/JobManager.h>

#include <AzCore/Jobs/Job.h>

using namespace AZ;

AZ_CLASS_ALLOCATOR_IMPL(JobManager, SystemAllocator);

JobManager::JobManager(const JobManagerDesc& desc)
    : m_impl(desc)
{
}

JobManager::~JobManager()
{
}
