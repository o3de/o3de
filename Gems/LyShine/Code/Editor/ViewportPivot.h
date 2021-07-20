/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

class ViewportPivot
{
public:

    ViewportPivot();
    virtual ~ViewportPivot();

    void Draw(Draw2dHelper& draw2d,
        const AZ::Entity* element,
        bool isHighlighted) const;

    AZ::Vector2 GetSize() const;

private:

    std::unique_ptr< ViewportIcon > m_pivot;
};
