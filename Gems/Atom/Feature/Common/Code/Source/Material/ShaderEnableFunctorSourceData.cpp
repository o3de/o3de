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

#include "./ShaderEnableFunctorSourceData.h"
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void ShaderEnableFunctorSourceData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderEnableFunctorSourceData>()
                    ->Version(5)
                    ->Field("opacityMode", &ShaderEnableFunctorSourceData::m_opacityMode)
                    ->Field("parallaxEnable", &ShaderEnableFunctorSourceData::m_parallaxEnable)
                    ->Field("parallaxPdoEnable", &ShaderEnableFunctorSourceData::m_parallaxPdoEnable)
                    ->Field("shadowShaderNoPSIndex", &ShaderEnableFunctorSourceData::m_shadowShaderNoPSIndex)
                    ->Field("shadowShaderWithPSIndex", &ShaderEnableFunctorSourceData::m_shadowShaderWithPSIndex)
                    ->Field("depthShaderNoPSIndex", &ShaderEnableFunctorSourceData::m_depthShaderNoPSIndex)
                    ->Field("depthShaderWithPSIndex", &ShaderEnableFunctorSourceData::m_depthShaderWithPSIndex)
                    ->Field("pbrShaderNoEdsIndex", &ShaderEnableFunctorSourceData::m_pbrShaderNoEdsIndex)
                    ->Field("pbrShaderWithEdsIndex", &ShaderEnableFunctorSourceData::m_pbrShaderWithEdsIndex)
                    ->Field("depthShaderTransparentMin", &ShaderEnableFunctorSourceData::m_depthShaderTransparentMin)
                    ->Field("depthShaderTransparentMax", &ShaderEnableFunctorSourceData::m_depthShaderTransparentMax)
                ;
            }
        }

        RPI::MaterialFunctorSourceData::FunctorResult ShaderEnableFunctorSourceData::CreateFunctor(const RuntimeContext& context) const
        {
            RPI::Ptr<ShaderEnableFunctor> functor = aznew ShaderEnableFunctor;

            functor->m_opacityModeIndex = context.FindMaterialPropertyIndex(Name{ m_opacityMode });
            if (functor->m_opacityModeIndex.IsNull())
            {
                return Failure();
            }
            AddMaterialPropertyDependency(functor, functor->m_opacityModeIndex);

            functor->m_parallaxEnableIndex = context.FindMaterialPropertyIndex(Name{ m_parallaxEnable });
            if (functor->m_parallaxEnableIndex.IsNull())
            {
                return Failure();
            }
            AddMaterialPropertyDependency(functor, functor->m_parallaxEnableIndex);

            functor->m_parallaxPdoEnableIndex = context.FindMaterialPropertyIndex(Name{ m_parallaxPdoEnable });
            if (functor->m_parallaxPdoEnableIndex.IsNull())
            {
                return Failure();
            }
            AddMaterialPropertyDependency(functor, functor->m_parallaxPdoEnableIndex);

            if (!context.CheckShaderIndexValid(m_shadowShaderWithPSIndex))
            {
                return Failure();
            }
            functor->m_shadowShaderWithPSIndex = m_shadowShaderWithPSIndex;

            if (!context.CheckShaderIndexValid(m_shadowShaderNoPSIndex))
            {
                return Failure();
            }
            functor->m_shadowShaderNoPSIndex = m_shadowShaderNoPSIndex;

            if (!context.CheckShaderIndexValid(m_depthShaderWithPSIndex))
            {
                return Failure();
            }
            functor->m_depthShaderWithPSIndex = m_depthShaderWithPSIndex;

            if (!context.CheckShaderIndexValid(m_depthShaderNoPSIndex))
            {
                return Failure();
            }
            functor->m_depthShaderNoPSIndex = m_depthShaderNoPSIndex;

            if (!context.CheckShaderIndexValid(m_pbrShaderNoEdsIndex))
            {
                return Failure();
            }
            functor->m_pbrShaderNoEdsIndex = m_pbrShaderNoEdsIndex;

            if (!context.CheckShaderIndexValid(m_pbrShaderWithEdsIndex))
            {
                return Failure();
            }
            functor->m_pbrShaderWithEdsIndex = m_pbrShaderWithEdsIndex;
            if (!context.CheckShaderIndexValid(m_depthShaderTransparentMin))
            {
                return Failure();
            }
            functor->m_depthShaderTransparentMin = m_depthShaderTransparentMin;
            if (!context.CheckShaderIndexValid(m_depthShaderTransparentMax))
            {
                return Failure();
            }
            functor->m_depthShaderTransparentMax = m_depthShaderTransparentMax;

            return Success(RPI::Ptr<RPI::MaterialFunctor>(functor));
        }
    }
}
