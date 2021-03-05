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

class ViewportHighlight
{
public:

    ViewportHighlight();
    virtual ~ViewportHighlight();

    //! Given the invisibleRootItem and the list of selectedItems, draw
    //! the correct highlight borders around elements, according to the
    //! given flags (defined in ViewportWidget::DrawElementBorders).
    void Draw(Draw2dHelper& draw2d,
        QTreeWidgetItem* invisibleRootItem,
        HierarchyItemRawPtrList& selectedItems,
        uint32 flags);

    void DrawHover(Draw2dHelper& draw2d, AZ::EntityId hoverElement);

private:

    std::unique_ptr< ViewportIcon > m_highlightIconSelected;
    std::unique_ptr< ViewportIcon > m_highlightIconUnselected;
};
