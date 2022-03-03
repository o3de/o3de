/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        //! Specifies a connection that will be pointed to by the pipeline for global reference
        struct PipelineGlobalConnection final
        {
            AZ_TYPE_INFO(PipelineGlobalConnection, "{8D708E59-E94C-4B25-8B0A-5D890BC8E6FE}");

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<PipelineGlobalConnection>()
                        ->Version(1)
                        ->Field("GlobalName", &PipelineGlobalConnection::m_globalName)
                        ->Field("Slot", &PipelineGlobalConnection::m_localBinding);
                }
            }

            //! The pipeline global name that other passes can use to reference the connection in a global way
            Name m_globalName;

            //! Name of the local binding on the pass to expose at a pipeline level for reference in a global way 
            Name m_localBinding;
        };

        using PipelineGlobalConnectionList = AZStd::vector<PipelineGlobalConnection>;

        //! Base class for custom data for Passes to be specified in a PassRequest or PassTemplate.
        //! If custom data is specified in both the PassTemplate and the PassRequest, the data
        //! specified in the PassRequest will take precedent and the data in PassTemplate ignored.
        //! All classes for custom pass data must inherit from this or one of it's derived classes.
        struct PassData
        {
            AZ_RTTI(PassData, "{F8594AE8-2588-4D64-89E5-B078A46A9AE4}");
            AZ_CLASS_ALLOCATOR(PassData, SystemAllocator, 0);

            PassData() = default;
            virtual ~PassData() = default;

            static void Reflect(ReflectContext* context)
            {
                PipelineGlobalConnection::Reflect(context);

                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<PassData>()
                        ->Version(1)
                        ->Field("PipelineGlobalConnections", &PassData::m_pipelineGlobalConnections);
                }
            }

            // Specifies global pipeline connections to the pipeline's immediate child passes
            PipelineGlobalConnectionList m_pipelineGlobalConnections;
        };
    } // namespace RPI
} // namespace AZ

