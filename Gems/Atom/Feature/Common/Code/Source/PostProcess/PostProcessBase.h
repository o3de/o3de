/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AtomCore/Instance/Instance.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>


namespace AZ
{
    namespace Render
    {
        class PostProcessFeatureProcessor;

        // Base class for post process settings and sub-settings classes
        // Provides helper functions for getting the scene, default view and render pipeline
        class PostProcessBase
        {
            friend class PostProcessFeatureProcessor;
        public:
            AZ_RTTI(AZ::Render::PostProcessBase, "{DDA620D0-12AB-471A-82F8-701BCD1A00D8}");
            AZ_CLASS_ALLOCATOR(PostProcessBase, SystemAllocator);

            PostProcessBase(PostProcessFeatureProcessor* featureProcessor);

        protected:

            RPI::ShaderResourceGroup* GetSceneSrg() const;
            RPI::Scene* GetParentScene() const;
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> GetDefaultViewSrg() const;

            PostProcessFeatureProcessor* m_featureProcessor = nullptr;
        };
    }
}
