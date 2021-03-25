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

#include <ISystem.h>
#include <IGem.h>

namespace RenderToTexture
{
    /*!
    * The RenderToTextureModule coordinates with the application
    * to reflect component classes and register console variables.
    */
    class RenderToTextureModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(RenderToTextureModule, "{B92256F5-7FD8-4C3E-8E6B-F9BAA081367B}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(RenderToTextureModule, AZ::SystemAllocator, 0);

        RenderToTextureModule();

        void OnCrySystemCVarRegistry() override;

    private:
        int rtt_aa;
        int rtt_dof;
        int rtt_motionblur;
        float rtt_maxFPS;
    };
} // namespace RenderToTexture
