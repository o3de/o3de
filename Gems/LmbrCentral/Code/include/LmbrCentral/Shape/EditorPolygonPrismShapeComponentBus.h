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
    /// Services provided by the Editor Component of Polygon Prism Shape.
    class EditorPolygonPrismShapeComponentRequests
        : public AZ::ComponentBus
    {
    public:
        /// Generates the vertices for displaying the shape in the editor.
        virtual void GenerateVertices() = 0;

    protected:
        ~EditorPolygonPrismShapeComponentRequests() = default;
    };

    /// Type to inherit to provide EditorPolygonPrismShapeComponentRequests
    using EditorPolygonPrismShapeComponentRequestsBus = AZ::EBus<EditorPolygonPrismShapeComponentRequests>;
} // namespace LmbrCentral
