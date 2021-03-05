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

#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/MultisampleState.h>
#include <Atom/RHI.Reflect/Size.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! A collection of render settings that passes can query from the pipeline.
        //! This allows the render pipeline to specify certain settings that can affect the underlying
        //! passes, for example the MSAA state or the render resolution of certain passes.
        //! Please note that settings these values doesn't have an automatic effect and the passes
        //! in question need to specify that they use source size/format/msaaState from the pipeline.
        //! See PassAttachment::Update and Pass::CreateAttachmentFromDesc for how settings are referenced
        struct PipelineRenderSettings final
        {
            AZ_TYPE_INFO(PipelineRenderSettings, "{2F794FB5-78E4-478A-AC1B-4A71AE172340}");

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<PipelineRenderSettings>()
                        ->Version(0)
                        ->Field("Size", &PipelineRenderSettings::m_size)
                        ->Field("Format", &PipelineRenderSettings::m_format)
                        ->Field("MultisampleState", &PipelineRenderSettings::m_multisampleState)
                        ;
                }
            }

            //! The pipeline can specify a custom size that passes can then chose to query
            //! Example use case: render at a fixed resolution regardless of swap chain size
            RHI::Size m_size;

            //! The pipeline can specify a custom format that passes can then chose to query
            //! Example use case: choose whether to render at R8G8B8A8 or R16G16B16A16
            RHI::Format m_format = RHI::Format::Unknown;

            //! The pipeline can specify a custom MSAA state that passes can then chose to query
            //! Example use case: choose whether to render at 2x MSAA, 4x, 8x or no MSAA
            RHI::MultisampleState m_multisampleState;
        };

    } // namespace RPI
} // namespace AZ
