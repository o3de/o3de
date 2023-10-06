/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialPipelineSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataHolder.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialPipelineSourceData::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderTemplate>()
                    ->Version(1)
                    ->Field("shader", &ShaderTemplate::m_shader)
                    ->Field("azsli", &ShaderTemplate::m_azsli)
                    ->Field("tag", &ShaderTemplate::m_shaderTag)
                    ;

                serializeContext->Class<RuntimeControls>()
                    ->Version(1)
                    ->Field("properties", &RuntimeControls::m_materialTypeInternalProperties)
                    ->Field("functors", &RuntimeControls::m_materialFunctorSourceData)
                    ;

                serializeContext->Class<MaterialPipelineSourceData>()
                    ->Version(4)    // Object Srg Additions
                    ->Field("shaderTemplates", &MaterialPipelineSourceData::m_shaderTemplates)
                    ->Field("runtime", &MaterialPipelineSourceData::m_runtimeControls)
                    ->Field("pipelineScript", &MaterialPipelineSourceData::m_pipelineScript)
                    ->Field("objectSrgAdditions", &MaterialPipelineSourceData::m_objectSrgAdditions)
                    ;
            }
        }

    } // namespace RPI
} // namespace AZ
