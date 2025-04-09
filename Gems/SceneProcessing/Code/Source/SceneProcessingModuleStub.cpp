/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(SCENE_PROCESSING_EDITOR)
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>

namespace AZ
{
    namespace SceneProcessing
    {
        class SceneProcessingModuleStub
            : public Module
        {
        public:
            AZ_RTTI(SceneProcessingModuleStub, "{23438D63-EA7F-425B-82F2-5B45C072B4E5}", Module);

            SceneProcessingModuleStub()
                : Module()
            {
            }
        };
    } // namespace SceneProcessing
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AZ::SceneProcessing::SceneProcessingModuleStub)
#else
AZ_DECLARE_MODULE_CLASS(Gem_SceneProcessing, AZ::SceneProcessing::SceneProcessingModuleStub)
#endif
#endif
