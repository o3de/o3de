/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Pass/PassAttachmentReflect.h>
#include <Atom/RPI.Reflect/Pass/PassData.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {

        //! Specifies a connection that will be pointed to by the pipeline for global reference
        struct PipelineConnection final
        {
            AZ_TYPE_INFO(PipelineConnection, "{8D708E59-E94C-4B25-8B0A-5D890BC8E6FE}");

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<PipelineConnection>()
                        ->Version(0)
                        ->Field("GlobalName", &PipelineConnection::m_globalName)
                        ->Field("ChildPass", &PipelineConnection::m_childPass)
                        ->Field("ChildPassSlot", &PipelineConnection::m_childPassBinding);
                }
            }

            //! The pipeline global name that other passes can use to reference the connection in a global way
            Name m_globalName;

            //! Name of the child pass from which to get the connection
            Name m_childPass;

            //! Name of the binding on the child pass that other passes can access directly from the pipeline using the global name above
            Name m_childPassBinding;
        };

        using PipelineConnectionList = AZStd::vector<PipelineConnection>;

        //! Custom Data for Pipeline Pass
        struct PipelinePassData
            : public PassData
        {
            AZ_RTTI(PipelinePassData, "{706C564E-705E-4053-B112-D1C083DA5C3D}", PassData);
            AZ_CLASS_ALLOCATOR(PipelinePassData, SystemAllocator, 0);

            PipelinePassData() = default;
            virtual ~PipelinePassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<PipelinePassData, PassData>()
                        ->Version(0)
                        ->Field("ImageAttachments", &PipelinePassData::m_imageAttachments)
                        ->Field("BufferAttachments", &PipelinePassData::m_bufferAttachments)
                        ->Field("PipelineConnections", &PipelinePassData::m_pipelineConnections);
                }
            }

            // Specifies global pipeline connections to the pipeline's immediate child passes
            PipelineConnectionList m_pipelineConnections;

            //! List of pipeline global image attachments
            PassImageAttachmentDescList m_imageAttachments;

            //! List of pipeline global buffer attachments
            PassBufferAttachmentDescList m_bufferAttachments;
        };

    }
}

