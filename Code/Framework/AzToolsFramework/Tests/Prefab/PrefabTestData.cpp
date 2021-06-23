/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestData.h>

namespace UnitTest
{
    InstanceData::InstanceData(const InstanceData& other)
        : m_name(other.m_name)
        , m_source(other.m_source)
    {
        m_patches.CopyFrom(other.m_patches, m_patches.GetAllocator());
    }

    InstanceData& InstanceData::InstanceData::operator=(
        const InstanceData& other)
    {
        m_name = other.m_name;
        m_source = other.m_source;
        m_patches.CopyFrom(other.m_patches, m_patches.GetAllocator());
        return *this;
    }
}
