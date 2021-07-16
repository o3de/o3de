/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Vector2.h>
#include <LyShine/Draw2d.h>

//! Abstract base class for drag interactions in the UI Editor viewport window.
class ViewportDragInteraction
{
public:

    enum class EndState
    {
        Inside,
        OutsideX, //!< Outside only on the X-axis
        OutsideY, //!< Outside only on the Y-axis
        OutsideXY, //!< Outside on both the X-axis and the Y-axis
        Canceled
    };

public:

    ViewportDragInteraction(const AZ::Vector2& startMousePos);
    virtual ~ViewportDragInteraction();

    //! Update the interaction each time the mouse moves
    virtual void Update(const AZ::Vector2& mousePos) = 0;

    //! Render any display desired during the interaction
    virtual void Render(Draw2dHelper& draw2d);

    //! End the interaction
    virtual void EndInteraction(EndState endState);

protected:

    AZ::Vector2 m_startMousePos;
};
