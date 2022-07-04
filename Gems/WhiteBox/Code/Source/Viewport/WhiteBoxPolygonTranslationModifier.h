/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Viewport/WhiteBoxManipulatorBounds.h"
#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace AzToolsFramework
{
    class LinearManipulator;
}

namespace WhiteBox
{
    class ManipulatorViewPolygon;

    //! Provides manipulators for translating a polygon on a white box mesh.
    class PolygonTranslationModifier
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        using HandleType = Api::PolygonHandle;

        PolygonTranslationModifier(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const Api::PolygonHandle& polygonHandle,
            const AZ::Vector3& intersectionPoint);
        ~PolygonTranslationModifier();

        bool MouseOver() const;
        void ForwardMouseOverEvent(const AzToolsFramework::ViewportInteraction::MouseInteraction& interaction);

        Api::PolygonHandle GetHandle() const; // Generic context version
        Api::PolygonHandle GetPolygonHandle() const;
        void SetPolygonHandle(const Api::PolygonHandle& polygonHandle);
        void SetColors(const AZ::Color& fillColor, const AZ::Color& outlineColor);

        void Refresh();
        void CreateView();
        bool PerformingAction() const;

    private:
        void CreateManipulator();
        void DestroyManipulator();

        AZ::EntityComponentIdPair
            m_entityComponentIdPair; //!< The entity and component id this modifier is associated with.
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator>
            m_translationManipulator; //!< The manipulator used to modify the position of the polygon (triangles).
        AZStd::vector<Api::VertexHandle> m_vertexHandles; //!< The vertex handles associated with this polygon.
        Api::PolygonHandle m_polygonHandle; //!< The polygon handle this modifier is associated with.
        AZStd::shared_ptr<ManipulatorViewPolygon>
            m_polygonView; //!< Manipulator view used to represent a mesh polygon for translation.
        AZ::Color m_fillColor =
            ed_whiteBoxPolygonHover; //!< The color to use for the highlighted filled section of the polygon.
        AZ::Color m_outlineColor =
            ed_whiteBoxOutlineHover; //!< The color to use for the outline of the polygon.
    };

    inline Api::PolygonHandle PolygonTranslationModifier::GetHandle() const
    {
        return GetPolygonHandle();
    }

    inline Api::PolygonHandle PolygonTranslationModifier::GetPolygonHandle() const
    {
        return m_polygonHandle;
    }

    inline void PolygonTranslationModifier::SetColors(const AZ::Color& fillColor, const AZ::Color& outlineColor)
    {
        m_fillColor = fillColor;
        m_outlineColor = outlineColor;
    }
} // namespace WhiteBox
