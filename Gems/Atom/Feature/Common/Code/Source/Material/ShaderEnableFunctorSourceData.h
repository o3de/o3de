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

#include "./ShaderEnableFunctor.h"
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>

namespace AZ
{
    namespace Render
    {
        class ShaderEnableFunctor;

        //! Builds a ShaderEnableFunctor
        class ShaderEnableFunctorSourceData final
            : public RPI::MaterialFunctorSourceData
        {
        public:
            AZ_RTTI(ShaderEnableFunctorSourceData, "{63775ECB-5C3E-44D3-B175-4537BF76C3A7}", RPI::MaterialFunctorSourceData);

            static void Reflect(ReflectContext* context);

            FunctorResult CreateFunctor(const RuntimeContext& context) const override;

        private:

            AZStd::string m_opacityMode;
            AZStd::string m_parallaxEnable;
            AZStd::string m_parallaxPdoEnable;

            uint32_t m_shadowShaderNoPSIndex = -1;
            uint32_t m_shadowShaderWithPSIndex = -1;
            uint32_t m_depthShaderNoPSIndex = -1;
            uint32_t m_depthShaderWithPSIndex = -1;
            uint32_t m_pbrShaderWithEdsIndex = -1;
            uint32_t m_pbrShaderNoEdsIndex = -1;
            // The following are used by the light culling system to produce min/max depth bounds
            uint32_t m_depthShaderTransparentMin = -1;
            uint32_t m_depthShaderTransparentMax = -1;
        };
    }
}
