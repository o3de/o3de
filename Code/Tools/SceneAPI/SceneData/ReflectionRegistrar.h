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
    class SerializeContext;
    class BehaviorContext;
    namespace SceneAPI
    {
        SCENE_DATA_API void RegisterDataTypeReflection(AZ::SerializeContext* context);
        SCENE_DATA_API void RegisterDataTypeBehaviorReflection(AZ::BehaviorContext* context);
    }
}
