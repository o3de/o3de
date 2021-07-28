/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>

#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace PrefabDependencyViewer::Utils
{
    using TemplateId = AzToolsFramework::Prefab::TemplateId;

    struct MetaData
    {
    public:
        AZ_CLASS_ALLOCATOR(MetaData, AZ::SystemAllocator, 0);

        MetaData() = default;

        MetaData(TemplateId tid, AZStd::string source)
            : m_tid(tid)
            , m_source(AZStd::move(source))
        {
        }

        TemplateId GetTemplateId()
        {
            return m_tid;
        }

        const char* GetSource()
        {
            return m_source.c_str();
        }

    private:
        TemplateId m_tid;
        AZStd::string m_source;
    };
} // namespace PrefabDependencyViewer::Utils
