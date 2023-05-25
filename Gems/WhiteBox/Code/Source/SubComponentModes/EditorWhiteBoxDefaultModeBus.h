/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Request bus for White Box ComponentMode operations while in 'default' mode.
    class EditorWhiteBoxDefaultModeRequests : public AZ::EntityComponentBus
    {
    public:
        //! Create a polygon scale modifier for the given polygon handles.
        virtual void CreatePolygonScaleModifier(const Api::PolygonHandle& polygonHandle) = 0;
        //! Create an edge scale modifier for the given edge handle.
        virtual void CreateEdgeScaleModifier(Api::EdgeHandle edgeHandle) = 0;
        //! Assign whatever polygon is currently hovered to the selected polygon translation modifier.
        virtual void AssignSelectedPolygonTranslationModifier() = 0;
        //! Assign whatever edge is currently hovered to the selected edge translation modifier.
        virtual void AssignSelectedEdgeTranslationModifier() = 0;
        //! Assign whatever vertex is currently hovered to the vertex selection modifier.
        virtual void AssignSelectedVertexSelectionModifier() = 0;
        //! Refresh (rebuild) the polygon scale modifier.
        //! The internal manipulator will be rebuilt from the current white box handles stored in the modifier.
        virtual void RefreshPolygonScaleModifier() = 0;
        //! Refresh (rebuild) the edge scale modifier.
        //! The internal manipulator will be rebuilt from the current white box handles stored in the modifier.
        virtual void RefreshEdgeScaleModifier() = 0;
        //! Refresh (rebuild) the polygon translation modifier.
        //! The internal manipulator will be rebuilt from the current white box handles stored in the modifier.
        virtual void RefreshPolygonTranslationModifier() = 0;
        //! Refresh (rebuild) the edge translation modifier.
        //! The internal manipulator will be rebuilt from the current white box handles stored in the modifier.
        virtual void RefreshEdgeTranslationModifier() = 0;
        //! Refresh (rebuild) the vertex selection modifier.
        //! The internal manipulator will be rebuilt from the current white box handles stored in the modifier.
        virtual void RefreshVertexSelectionModifier() = 0;
        //! Returns the currently selected vertex handles.
        //! @note If no vertex handles are selected an empty vector is returned.
        virtual Api::VertexHandles SelectedVertexHandles() const = 0;
        //! Returns the currently selected edge handles.
        //! @note If no edge handles are selected an empty vector is returned.
        virtual Api::EdgeHandles SelectedEdgeHandles() const = 0;
        //! Returns the currently selected polygon handles.
        //! @note If no polygon handles are selected an empty vector is returned.
        virtual Api::PolygonHandles SelectedPolygonHandles() const = 0;
        //! Returns the hovered vertex handle.
        //! @note If no vertex is currently being hovered then an invalid VertexHandle is returned.
        virtual Api::VertexHandle HoveredVertexHandle() const = 0;
        //! Returns the hovered edge handle.
        //! @note If no edge is currently being hovered then an invalid EdgeHandle is returned.
        virtual Api::EdgeHandle HoveredEdgeHandle() const = 0;
        //! Returns the hovered polygon handle.
        //! @note If no polygon is currently being hovered then an empty PolygonHandle is returned.
        virtual Api::PolygonHandle HoveredPolygonHandle() const = 0;
        //! Hide the selected Edge.
        virtual void HideSelectedEdge() = 0;
        //! Hide the selected Vertex.
        virtual void HideSelectedVertex() = 0;

    protected:
        ~EditorWhiteBoxDefaultModeRequests() = default;
    };

    using EditorWhiteBoxDefaultModeRequestBus = AZ::EBus<EditorWhiteBoxDefaultModeRequests>;
} // namespace WhiteBox
