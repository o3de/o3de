/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class BehaviorContext;
}

namespace LmbrCentral
{
    //! Services provided by the Editor Shape Component
    class EditorShapeComponentRequests
        : public AZ::ComponentBus
    {
    public:
        //! @brief Sets the shape color
        //! @param solidColor the color to be used for drawing solid shapes
        virtual void SetShapeColor(const AZ::Color& solidColor) = 0;

        //! @brief Sets the wireframe shape color
        //! @param wireColor the color to be used for drawing shapes in wireframe
        virtual void SetShapeWireframeColor(const AZ::Color& wireColor)  = 0;

        //! @brief Sets if the shape should be visible in the editor when the object is deselected
        //! @param visible true if the shape should be visible when deselected
        virtual void SetVisibleInEditor(bool visible) = 0;

        //! @brief Sets if the shape color can be set by the user in the editor. This is useful for
        //!   components that need to control the shape's color directly.
        //! @param editable true if the shape color should be editable
        virtual void SetShapeColorIsEditable(bool editable) = 0;

        //! @brief Returns true if the shape color can be set by the user in the editor.
        virtual bool GetShapeColorIsEditable() = 0;

        //! @brief Sets if the shape is visible in game view
        //! @param visible true for shape to be visible
        virtual void SetVisibleInGame(bool visible) = 0;
    };

    // Bus to service the Shape component requests event group
    using EditorShapeComponentRequestsBus = AZ::EBus<EditorShapeComponentRequests>;
} // namespace LmbrCentral
