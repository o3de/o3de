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
