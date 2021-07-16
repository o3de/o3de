#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneData/SceneDataConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        class SceneDataStandaloneAllocator
        {
        public:
            SCENE_DATA_API static void Initialize(AZ::EnvironmentInstance environment);
            SCENE_DATA_API static void TearDown();

        private:
            static bool m_allocatorInitialized;
        };
    } // namespace SceneAPI
} // namespace AZ
