/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
