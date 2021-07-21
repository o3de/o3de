/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

class ViewportAnchor
{
public:

    ViewportAnchor();
    virtual ~ViewportAnchor();

    void Draw(Draw2dHelper& draw2d, AZ::Entity* element, bool drawUnTransformedRect, bool drawAnchorLines, bool drawLinesToParent,
        bool anchorInteractionEnabled, ViewportHelpers::SelectedAnchors highlightedAnchors) const;

private:

    //! draw distance lines from the anchor pos to the parent rectangle
    void DrawAnchorToParentLines(Draw2dHelper& draw2d, const UiTransform2dInterface::Anchors& anchors,
        const UiTransformInterface::RectPoints& parentPoints, const AZ::Matrix4x4& transform,
        ViewportHelpers::SelectedAnchors highlightedAnchors) const;

    std::unique_ptr< ViewportIcon > m_anchorWhole;
    std::unique_ptr< ViewportIcon > m_anchorLeft;
    std::unique_ptr< ViewportIcon > m_anchorLeftTop;
    std::unique_ptr< ViewportIcon > m_dottedLine;
};
