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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/PlanarManipulator.h>
#include <AzToolsFramework/Manipulators/SurfaceManipulator.h>

namespace AzToolsFramework
{
    /// TranslationManipulators is an aggregation of 3 linear manipulators, 3 planar manipulators
    /// and one surface manipulator who share the same transform.
    class TranslationManipulators
        : public Manipulators
    {
    public:
        AZ_RTTI(TranslationManipulators, "{D5E49EA2-30E0-42BC-A51D-6A7F87818260}")
        AZ_CLASS_ALLOCATOR(TranslationManipulators, AZ::SystemAllocator, 0)

        /// How many dimensions does this translation manipulator have
        enum class Dimensions
        {
            Two,
            Three
        };

        TranslationManipulators(Dimensions dimensions, const AZ::Transform& worldFromLocal);

        void InstallLinearManipulatorMouseDownCallback(const LinearManipulator::MouseActionCallback& onMouseDownCallback);
        void InstallLinearManipulatorMouseMoveCallback(const LinearManipulator::MouseActionCallback& onMouseMoveCallback);
        void InstallLinearManipulatorMouseUpCallback(const LinearManipulator::MouseActionCallback& onMouseUpCallback);

        void InstallPlanarManipulatorMouseDownCallback(const PlanarManipulator::MouseActionCallback& onMouseDownCallback);
        void InstallPlanarManipulatorMouseMoveCallback(const PlanarManipulator::MouseActionCallback& onMouseMoveCallback);
        void InstallPlanarManipulatorMouseUpCallback(const PlanarManipulator::MouseActionCallback& onMouseUpCallback);

        void InstallSurfaceManipulatorMouseDownCallback(const SurfaceManipulator::MouseActionCallback& onMouseDownCallback);
        void InstallSurfaceManipulatorMouseMoveCallback(const SurfaceManipulator::MouseActionCallback& onMouseMoveCallback);
        void InstallSurfaceManipulatorMouseUpCallback(const SurfaceManipulator::MouseActionCallback& onMouseUpCallback);

        void SetSpace(const AZ::Transform& worldFromLocal) override;
        void SetLocalTransform(const AZ::Transform& localTransform) override;
        void SetLocalPosition(const AZ::Vector3& localPosition) override;
        void SetLocalOrientation(const AZ::Quaternion& localOrientation) override;

        void SetAxes(
            const AZ::Vector3& axis1, const AZ::Vector3& axis2,
            const AZ::Vector3& axis3 = AZ::Vector3::CreateAxisZ());

        void ConfigurePlanarView(
            const AZ::Color& plane1Color,
            const AZ::Color& plane2Color = AZ::Color(0.0f, 1.0f, 0.0f, 0.5f),
            const AZ::Color& plane3Color = AZ::Color(0.0f, 0.0f, 1.0f, 0.5f));

        void ConfigureLinearView(
            float axisLength,
            const AZ::Color& axis1Color, const AZ::Color& axis2Color,
            const AZ::Color& axis3Color = AZ::Color(0.0f, 0.0f, 1.0f, 0.5f));

        void ConfigureSurfaceView(
            float radius, const AZ::Color& color);

    private:
        AZ_DISABLE_COPY_MOVE(TranslationManipulators)

        // Manipulators
        void ProcessManipulators(const AZStd::function<void(BaseManipulator*)>&) override;

        const Dimensions m_dimensions; ///< How many dimensions of freedom does this manipulator have.

        AZStd::vector<AZStd::shared_ptr<LinearManipulator>> m_linearManipulators;
        AZStd::vector<AZStd::shared_ptr<PlanarManipulator>> m_planarManipulators;
        AZStd::shared_ptr<SurfaceManipulator> m_surfaceManipulator = nullptr;
    };

    /// IndexedTranslationManipulator wraps a standard TranslationManipulators and allows it to be linked
    /// to a particular index in a list of vertices/points.
    template<typename Vertex>
    struct IndexedTranslationManipulator
    {
        explicit IndexedTranslationManipulator(
            TranslationManipulators::Dimensions dimensions, AZ::u64 vertIndex,
            const Vertex& position, const AZ::Transform& worldFromLocal)
                : m_manipulator(dimensions, worldFromLocal)
        {
            m_vertices.push_back({ position, Vertex::CreateZero(), vertIndex });
        }

        /// Store vertex start position as manipulator event occurs, index refers to location in container.
        struct VertexLookup
        {
            Vertex m_start;
            Vertex m_offset;
            AZ::u64 m_index;

            Vertex CurrentPosition() const { return m_start + m_offset; }
        };

        /// Helper to iterate over all vertices stored by the manipulator.
        void Process(AZStd::function<void(VertexLookup&)> fn)
        {
            for (VertexLookup& vertex : m_vertices)
            {
                fn(vertex);
            }
        }

        AZStd::vector<VertexLookup> m_vertices; ///< List of vertices currently associated with this translation manipulator.
        TranslationManipulators m_manipulator;
    };

    /// Function pointer to configure how a translation manipulator should look and behave (dimensions/axes/views).
    using TranslationManipulatorConfiguratorFn = void(*)(TranslationManipulators*);

    void ConfigureTranslationManipulatorAppearance3d(
        TranslationManipulators* translationManipulators);
    void ConfigureTranslationManipulatorAppearance2d(
        TranslationManipulators* translationManipulators);

} // namespace AzToolsFramework