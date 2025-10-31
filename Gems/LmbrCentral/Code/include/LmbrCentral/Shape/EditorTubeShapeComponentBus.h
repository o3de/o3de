/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    /// Editor specific TubeShapeComponent requests.
    class EditorTubeShapeComponentRequests
        : public AZ::ComponentBus
    {
    public:
         /// Request the EditorTubeShapeComponent to regenerate its vertices.
         /// Recreate vertex and index buffers to update the visual representation
         /// of the tube to match the internals state.
        virtual void GenerateVertices() = 0;

    protected:
        ~EditorTubeShapeComponentRequests() = default;
    };

    /// Type to inherit to provide EditorTubeShapeComponentRequests
    using EditorTubeShapeComponentRequestBus = AZ::EBus<EditorTubeShapeComponentRequests>;

    /// Editor specific TubeShapeComponentMode requests.
    class EditorTubeShapeComponentModeRequests
        : public AZ::EntityComponentBus
    {
    public:
         /// Request the EditorTubeShapeComponent to reset all radii.
        virtual void ResetRadii() = 0;

    protected:
        ~EditorTubeShapeComponentModeRequests() = default;
    };

    /// Type to inherit to provide EditorTubeShapeComponentRequests
    using EditorTubeShapeComponentModeRequestBus = AZ::EBus<EditorTubeShapeComponentModeRequests>;
} // namespace LmbrCentral
