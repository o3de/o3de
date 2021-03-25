/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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