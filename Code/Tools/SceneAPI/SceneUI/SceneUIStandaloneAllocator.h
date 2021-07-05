#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneUI/SceneUIConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        class SceneUIStandaloneAllocator
        {
        public:
            SCENE_UI_API static void Initialize();
            SCENE_UI_API static void TearDown();

        private:
            static bool m_allocatorInitialized;
        };
    } // namespace SceneAPI
} // namespace AZ
