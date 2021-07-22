/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
