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

#include <Atom/Utils/StableDynamicArray.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    class Transform;

    namespace Render
    {
        class SkinnedMeshRenderProxyInterface
        {
        public:
            AZ_RTTI(AZ::Render::SkinnedMeshRenderProxyInterface, "{8A850EE0-6F08-446D-92A0-B83714D541D1}");

            virtual void SetTransform(const AZ::Transform& transform) = 0;
            virtual void SetSkinningMatrices(const AZStd::vector<float>& data) = 0;
            virtual void SetMorphTargetWeights(uint32_t lodIndex, const AZStd::vector<float>& weights) = 0;
        };
        using SkinnedMeshRenderProxyInterfaceHandle = StableDynamicArrayHandle<SkinnedMeshRenderProxyInterface>;

    } // namespace Render
} // namespace AZ
