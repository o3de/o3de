/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_CSVEXPORTSETTINGS_H
#define DRILLER_CSVEXPORTSETTINGS_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#pragma once

namespace Driller
{
    class CSVExportSettings
    {
    public:
        AZ_CLASS_ALLOCATOR(CSVExportSettings, AZ::SystemAllocator, 0);

        CSVExportSettings()
            : m_shouldExportColumnDescriptor(true)
        {
        }

        virtual ~CSVExportSettings()
        {
        }

        void SetShouldExportColumnDescriptors(bool shouldExport)
        {
            m_shouldExportColumnDescriptor = shouldExport;
        }

        bool ShouldExportColumnDescriptors() const
        {
            return m_shouldExportColumnDescriptor;
        }

    private:
        bool m_shouldExportColumnDescriptor;
    };
}

#endif
