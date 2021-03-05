/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
