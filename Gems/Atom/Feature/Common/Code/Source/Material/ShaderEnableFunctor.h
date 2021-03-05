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

#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RHI.Reflect/Limits.h>

namespace AZ
{
    namespace Render
    {
        enum OpacityMode
        {
            Opaque = 0,
            Cutout,
            Blended,
            TintedTransparent,
        };

        //! Select shadow and depth shader based on opacity mode and parallax state
        //! Opaque:                      Enable shader without PS
        //! Cutout or Parallax enable:   Enable shader with PS
        //! Blended:                     Disable both
        //! TintedTransparent:           Disable both
        class ShaderEnableFunctor final
            : public RPI::MaterialFunctor
        {
            friend class ShaderEnableFunctorSourceData;
        public:
            AZ_RTTI(ShaderEnableFunctor, "{2079A693-FE4F-46A7-95C0-09D88AC156D0}", RPI::MaterialFunctor);

            static void Reflect(ReflectContext* context);

            void Process(RuntimeContext& context) override;

        private:
            RPI::MaterialPropertyIndex m_opacityModeIndex;
            RPI::MaterialPropertyIndex m_parallaxEnableIndex;
            RPI::MaterialPropertyIndex m_parallaxPdoEnableIndex;

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
