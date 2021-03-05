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

#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Provides manipulators for scaling an edge on a white box mesh.
    class EdgeScaleModifier
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        EdgeScaleModifier(const Api::EdgeHandle& edgeHandle, const AZ::EntityComponentIdPair& entityComponentIdPair);
        ~EdgeScaleModifier();

        void Refresh();

        Api::EdgeHandle GetEdgeHandle() const;
        void SetEdgeHandle(Api::EdgeHandle edgeHandle);

    private:
        //! The scaling modes supported by this modifier.
        enum class ScaleMode
        {
            Uniform,
            NonUniform
        };

        void CreateManipulators();
        void DestroyManipulators();
        void InitializeScaleModifier(
            const WhiteBoxMesh* whiteBox, const AzToolsFramework::LinearManipulator::Action& action);
        static ScaleMode GetScaleModeFromModifierKey(
            const AzToolsFramework::ViewportInteraction::KeyboardModifiers& modifiers);

        AZ::EntityComponentIdPair
            m_entityComponentIdPair; //!< The entity and component id this modifier is associated with.
        Api::EdgeHandle m_edgeHandle; //!< The edge handle this modifier is responsible for.
        AZStd::array<AZStd::shared_ptr<AzToolsFramework::LinearManipulator>, 2>
            m_scaleManipulators; //!< Manipulators to handle each end of an edge.
        AZStd::array<AZ::Vector3, 2>
            m_initialVertexPositions; //!< The initial position of all vertices when scaling begins.
        AZ::Vector3 m_pivotPoint; //!< The pivot point for scaling.
        float m_startingDistance =
            0.0f; //!< The distance a manipulator is from the pivot point when an interaction first begins.
        AZ::u32 m_selectedHandleIndex = 0; //!< The vertex that will be scaled for non-uniform scaling.
        ScaleMode m_scaleMode = ScaleMode::Uniform; //!< The current scale mode being applied.
    };

    inline Api::EdgeHandle EdgeScaleModifier::GetEdgeHandle() const
    {
        return m_edgeHandle;
    }

    inline void EdgeScaleModifier::SetEdgeHandle(Api::EdgeHandle edgeHandle)
    {
        m_edgeHandle = edgeHandle;
    }
} // namespace WhiteBox
