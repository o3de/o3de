#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzTest/AzTest.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>

namespace AZ {
    namespace SceneAPI {
        namespace Containers {
            class MockScene
                : public Scene
            {
            public:
                MockScene(const char* name)
                    : Scene(name)
                {}
            };
        } // namespace Containers
    } // namespace SceneAPI
}  // namespace AZ
