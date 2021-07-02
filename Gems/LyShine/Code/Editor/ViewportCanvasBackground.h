/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <memory>

//! Responsible for drawing an image background behind any canvas.
class ViewportCanvasBackground
{
public:

    ViewportCanvasBackground();
    virtual ~ViewportCanvasBackground();

    //! Renders an image background for canvas elements to be rendered on top of.
    void Draw(Draw2dHelper& draw2d, const AZ::Vector2& canvasSize, float canvasToViewportScale, const AZ::Vector3& canvasToViewportTranslation);

private:

    std::unique_ptr< ViewportIcon > m_canvasBackground;
};
