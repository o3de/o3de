/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Events/GenerateEventContext.h>

namespace AZ::SceneAPI::Events
{
    /////////////
    // PreGenerateEventContext
    /////////////

    GenerateEventBaseContext::GenerateEventBaseContext(Containers::Scene& scene, const char* platformIdentifier)
        : m_scene(scene)
        , m_platformIdentifier(platformIdentifier)
    {
    }

    Containers::Scene& GenerateEventBaseContext::GetScene() const
    {
        return m_scene;
    }

    const char* GenerateEventBaseContext::GetPlatformIdentifier() const
    {
        return m_platformIdentifier;
    }
} // namespace AZ::SceneAPI::Events
