/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzFramework/Input/Channels/InputChannel.h>
#include <Cry_Math.h>

struct ray_hit;

namespace AzFramework
{
    namespace RenderGeometry
    {
        struct RayResult;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// UiCanvasOnMeshBus
class UiCanvasOnMeshInterface
    : public AZ::ComponentBus
{
public:
    virtual ~UiCanvasOnMeshInterface() {}


    //! Convert the input ray collision into a canvas space position and pass the event and that position
    //! to the UI canvas to handle as a positional input event
    //! \return true if the event was handled
    virtual bool ProcessHitInputEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot, const AzFramework::RenderGeometry::RayResult& rayResult) = 0;
};

using UiCanvasOnMeshBus = AZ::EBus<UiCanvasOnMeshInterface>;
