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

#include "Viewport/WhiteBoxModifierUtil.h"

#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Provides manipulators for scaling a face on a white box mesh.
    class PolygonScaleModifier
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        PolygonScaleModifier(
            const Api::PolygonHandle& polygonHandle, const AZ::EntityComponentIdPair& entityComponentIdPair);
        ~PolygonScaleModifier();

        void Refresh();

        Api::PolygonHandle GetPolygonHandle() const;
        void SetPolygonHandle(const Api::PolygonHandle& polygonHandle);

    private:
        void CreateManipulators();
        void DestroyManipulators();

        void OnMouseMove(const AzToolsFramework::LinearManipulator::Action& action);

        AZ::EntityComponentIdPair
            m_entityComponentIdPair; //!< The entity and component id this modifier is associated with.
        Api::PolygonHandle m_polygonHandle; //!< The polygon handle this modifier is responsible for.
        AZStd::vector<AZStd::shared_ptr<AzToolsFramework::LinearManipulator>>
            m_scaleManipulators; //!< Manipulators to handle corners of face to scale.
        AZStd::vector<AZ::Vector3>
            m_initialVertexPositions; //!< The initial position of all vertices when scaling begins.
        AZ::Vector3 m_midPoint; //!< The midpoint of the polygon when scaling first begins.
        AppendStage m_appendStage = AppendStage::None; //!< Are we in the process of appending a new polygon.
        float m_startingDistance =
            0.0f; //!< The distance a manipulator is from the midpoint when an interaction first begins.
        float m_offsetWhenExtruded = 0.0f; //!< How far the manipulator has moved before an extrusion starts.
    };

    inline Api::PolygonHandle PolygonScaleModifier::GetPolygonHandle() const
    {
        return m_polygonHandle;
    }
} // namespace WhiteBox
