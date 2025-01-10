/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/GpuQuery/GpuQueryTypes.h>

namespace AZ
{
    namespace RPI
    {
        class Query;

        //! The GpuQuerySystem is the interface the user communicates with in order to create RPI Queries.
        //! For each QueryType, the GpuQuerySystem creates a RPI QueryPool. The GpuQuerySystem will create a RPI Query instance in the
        //! applicable RPI QueryPool depending on the requested QueryType.
        class ATOM_RPI_PUBLIC_API GpuQuerySystemInterface
        {
        public:
            AZ_RTTI(GpuQuerySystemInterface, "{55DF69E7-3C0E-471F-86EF-EA561901407C}");

            GpuQuerySystemInterface() = default;
            virtual ~GpuQuerySystemInterface() = default;

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not.
            AZ_DISABLE_COPY_MOVE(GpuQuerySystemInterface);

            static GpuQuerySystemInterface* Get();

            //! Creates a query with the specified QueryType, returns an pointer of the query instance.
            virtual RHI::Ptr<Query> CreateQuery(RHI::QueryType queryType,
                RHI::QueryPoolScopeAttachmentType attachmentType,
                RHI::ScopeAttachmentAccess attachmentAccess) = 0;
        };
    }; // namespace RPI
}; // namespace AZ
