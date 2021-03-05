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

#include "./ShaderEnableFunctor.h"
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

namespace AZ
{
    namespace Render
    {
        void ShaderEnableFunctor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderEnableFunctor, RPI::MaterialFunctor>()
                    ->Version(4)
                    ->Field("opacityModeIndex", &ShaderEnableFunctor::m_opacityModeIndex)
                    ->Field("parallaxEnableIndex", &ShaderEnableFunctor::m_parallaxEnableIndex)
                    ->Field("parallaxPdoEnableIndex", &ShaderEnableFunctor::m_parallaxPdoEnableIndex)
                    ->Field("shadowShaderNoPSIndex", &ShaderEnableFunctor::m_shadowShaderNoPSIndex)
                    ->Field("shadowShaderWithPSIndex", &ShaderEnableFunctor::m_shadowShaderWithPSIndex)
                    ->Field("depthShaderNoPSIndex", &ShaderEnableFunctor::m_depthShaderNoPSIndex)
                    ->Field("depthShaderWithPSIndex", &ShaderEnableFunctor::m_depthShaderWithPSIndex)
                    ->Field("pbrShaderNoEdsIndex", &ShaderEnableFunctor::m_pbrShaderNoEdsIndex)
                    ->Field("pbrShaderWithEdsIndex", &ShaderEnableFunctor::m_pbrShaderWithEdsIndex)
                    ->Field("depthShaderTransparentMin", &ShaderEnableFunctor::m_depthShaderTransparentMin)
                    ->Field("depthShaderTransparentMax", &ShaderEnableFunctor::m_depthShaderTransparentMax)
                ;
            }
        }

        void ShaderEnableFunctor::Process(RuntimeContext& context)
        {
            unsigned int opacityMode = context.GetMaterialPropertyValue<uint32_t>(m_opacityModeIndex);
            bool parallaxEnabled = context.GetMaterialPropertyValue<bool>(m_parallaxEnableIndex);
            bool parallaxPdoEnabled = context.GetMaterialPropertyValue<bool>(m_parallaxPdoEnableIndex);

            if (parallaxEnabled && parallaxPdoEnabled)
            {
                context.SetShaderEnabled(m_depthShaderNoPSIndex, false);
                context.SetShaderEnabled(m_shadowShaderNoPSIndex, false);
                context.SetShaderEnabled(m_pbrShaderWithEdsIndex, false);

                context.SetShaderEnabled(m_depthShaderWithPSIndex, true);
                context.SetShaderEnabled(m_shadowShaderWithPSIndex, true);
                context.SetShaderEnabled(m_pbrShaderNoEdsIndex, true);
            }
            else
            {
                context.SetShaderEnabled(m_depthShaderNoPSIndex, opacityMode == OpacityMode::Opaque );
                context.SetShaderEnabled(m_shadowShaderNoPSIndex, opacityMode == OpacityMode::Opaque);
                context.SetShaderEnabled(m_pbrShaderWithEdsIndex, opacityMode == OpacityMode::Opaque || opacityMode == OpacityMode::Blended || opacityMode == OpacityMode::TintedTransparent);

                context.SetShaderEnabled(m_depthShaderWithPSIndex, opacityMode == OpacityMode::Cutout);
                context.SetShaderEnabled(m_shadowShaderWithPSIndex, opacityMode == OpacityMode::Cutout);
                context.SetShaderEnabled(m_pbrShaderNoEdsIndex, opacityMode == OpacityMode::Cutout);
            }

            context.SetShaderEnabled(m_depthShaderTransparentMin, opacityMode == OpacityMode::Blended || opacityMode == OpacityMode::TintedTransparent);
            context.SetShaderEnabled(m_depthShaderTransparentMax, opacityMode == OpacityMode::Blended || opacityMode == OpacityMode::TintedTransparent);
        }
    }
}
