/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TranslationManipulators.h"

#include <AzCore/Math/VectorConversions.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

namespace AzToolsFramework
{
    static const float SurfaceManipulatorTransparency = 0.75f;
    static const float LinearManipulatorAxisLength = 2.0f;
    static const float SurfaceManipulatorRadius = 0.1f;

    static const AZ::Color LinearManipulatorXAxisColor = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
    static const AZ::Color LinearManipulatorYAxisColor = AZ::Color(0.0f, 1.0f, 0.0f, 1.0f);
    static const AZ::Color LinearManipulatorZAxisColor = AZ::Color(0.0f, 0.0f, 1.0f, 1.0f);
    static const AZ::Color SurfaceManipulatorColor = AZ::Color(1.0f, 1.0f, 0.0f, 0.5f);

    TranslationManipulators::TranslationManipulators(
        const Dimensions dimensions, const AZ::Transform& worldFromLocal, const AZ::Vector3& nonUniformScale)
        : m_dimensions(dimensions)
    {
        switch (dimensions)
        {
        case Dimensions::Two:
            m_linearManipulators.reserve(2);
            for (size_t manipulatorIndex = 0; manipulatorIndex < 2; ++manipulatorIndex)
            {
                m_linearManipulators.emplace_back(LinearManipulator::MakeShared(worldFromLocal));
            }
            m_planarManipulators.emplace_back(PlanarManipulator::MakeShared(worldFromLocal));
            break;
        case Dimensions::Three:
            m_linearManipulators.reserve(3);
            m_planarManipulators.reserve(3);
            for (size_t manipulatorIndex = 0; manipulatorIndex < 3; ++manipulatorIndex)
            {
                m_linearManipulators.emplace_back(LinearManipulator::MakeShared(worldFromLocal));
                m_planarManipulators.emplace_back(PlanarManipulator::MakeShared(worldFromLocal));
            }
            m_surfaceManipulator = SurfaceManipulator::MakeShared(worldFromLocal);
            break;
        default:
            AZ_Assert(false, "Invalid dimensions provided");
            break;
        }

        m_manipulatorSpaceWithLocalTransform.SetSpace(worldFromLocal);
        SetNonUniformScale(nonUniformScale);
    }

    void TranslationManipulators::InstallLinearManipulatorMouseDownCallback(
        const LinearManipulator::MouseActionCallback& onMouseDownCallback)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
        }
    }

    void TranslationManipulators::InstallLinearManipulatorMouseMoveCallback(
        const LinearManipulator::MouseActionCallback& onMouseMoveCallback)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulator->InstallMouseMoveCallback(onMouseMoveCallback);
        }
    }

    void TranslationManipulators::InstallLinearManipulatorMouseUpCallback(const LinearManipulator::MouseActionCallback& onMouseUpCallback)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
        }
    }

    void TranslationManipulators::InstallPlanarManipulatorMouseDownCallback(
        const PlanarManipulator::MouseActionCallback& onMouseDownCallback)
    {
        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
        }
    }

    void TranslationManipulators::InstallPlanarManipulatorMouseMoveCallback(
        const PlanarManipulator::MouseActionCallback& onMouseMoveCallback)
    {
        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulator->InstallMouseMoveCallback(onMouseMoveCallback);
        }
    }

    void TranslationManipulators::InstallPlanarManipulatorMouseUpCallback(const PlanarManipulator::MouseActionCallback& onMouseUpCallback)
    {
        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
        }
    }

    void TranslationManipulators::InstallSurfaceManipulatorMouseDownCallback(
        const SurfaceManipulator::MouseActionCallback& onMouseDownCallback)
    {
        if (m_surfaceManipulator)
        {
            m_surfaceManipulator->InstallLeftMouseDownCallback(onMouseDownCallback);
        }
    }

    void TranslationManipulators::InstallSurfaceManipulatorMouseUpCallback(const SurfaceManipulator::MouseActionCallback& onMouseUpCallback)
    {
        if (m_surfaceManipulator)
        {
            m_surfaceManipulator->InstallLeftMouseUpCallback(onMouseUpCallback);
        }
    }

    void TranslationManipulators::InstallSurfaceManipulatorMouseMoveCallback(
        const SurfaceManipulator::MouseActionCallback& onMouseMoveCallback)
    {
        if (m_surfaceManipulator)
        {
            m_surfaceManipulator->InstallMouseMoveCallback(onMouseMoveCallback);
        }
    }

    void TranslationManipulators::SetLocalTransformImpl(const AZ::Transform& localTransform)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulator->SetLocalTransform(localTransform);
        }

        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulator->SetLocalTransform(localTransform);
        }

        if (m_surfaceManipulator)
        {
            m_surfaceManipulator->SetLocalPosition(localTransform.GetTranslation());
        }
    }

    void TranslationManipulators::SetLocalPositionImpl(const AZ::Vector3& localPosition)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulator->SetLocalPosition(localPosition);
        }

        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulator->SetLocalPosition(localPosition);
        }

        if (m_surfaceManipulator)
        {
            m_surfaceManipulator->SetLocalPosition(localPosition);
        }
    }

    void TranslationManipulators::SetLocalOrientationImpl(const AZ::Quaternion& localOrientation)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulator->SetLocalOrientation(localOrientation);
        }

        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulator->SetLocalOrientation(localOrientation);
        }
    }

    void TranslationManipulators::SetSpaceImpl(const AZ::Transform& worldFromLocal)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulator->SetSpace(worldFromLocal);
        }

        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulator->SetSpace(worldFromLocal);
        }

        if (m_surfaceManipulator)
        {
            m_surfaceManipulator->SetSpace(worldFromLocal);
        }
    }

    void TranslationManipulators::SetNonUniformScaleImpl(const AZ::Vector3& nonUniformScale)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulator->SetNonUniformScale(nonUniformScale);
        }

        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulator->SetNonUniformScale(nonUniformScale);
        }

        if (m_surfaceManipulator)
        {
            m_surfaceManipulator->SetNonUniformScale(nonUniformScale);
        }
    }

    void TranslationManipulators::SetAxes(
        const AZ::Vector3& axis1, const AZ::Vector3& axis2, const AZ::Vector3& axis3 /*= AZ::Vector3::CreateAxisZ()*/)
    {
        AZ::Vector3 axes[] = { axis1, axis2, axis3 };

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_linearManipulators.size(); ++manipulatorIndex)
        {
            m_linearManipulators[manipulatorIndex]->SetAxis(axes[manipulatorIndex]);
        }

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_planarManipulators.size(); ++manipulatorIndex)
        {
            m_planarManipulators[manipulatorIndex]->SetAxes(axes[manipulatorIndex], axes[(manipulatorIndex + 1) % 3]);
        }
    }

    void TranslationManipulators::ConfigureLinearView(
        const float axisLength,
        const AZ::Color& axis1Color,
        const AZ::Color& axis2Color,
        const AZ::Color& axis3Color /*= AZ::Color(0.0f, 0.0f, 1.0f, 0.5f)*/)
    {
        const float coneLength = 0.28f;
        const float coneRadius = 0.07f;

        const AZ::Color axesColor[] = { axis1Color, axis2Color, axis3Color };

        const auto configureLinearView = [lineBoundWidth = m_lineBoundWidth, coneLength, axisLength,
                                          coneRadius](LinearManipulator* linearManipulator, const AZ::Color& color)
        {
            const auto lineLength = axisLength - coneLength;

            ManipulatorViews views;
            views.emplace_back(CreateManipulatorViewLine(*linearManipulator, color, lineLength, lineBoundWidth));
            views.emplace_back(
                CreateManipulatorViewCone(*linearManipulator, color, linearManipulator->GetAxis() * lineLength, coneLength, coneRadius));
            linearManipulator->SetViews(AZStd::move(views));
        };

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_linearManipulators.size(); ++manipulatorIndex)
        {
            configureLinearView(m_linearManipulators[manipulatorIndex].get(), axesColor[manipulatorIndex]);
        }
    }

    void TranslationManipulators::ConfigurePlanarView(
        const AZ::Color& plane1Color,
        const AZ::Color& plane2Color /*= AZ::Color(0.0f, 1.0f, 0.0f, 0.5f)*/,
        const AZ::Color& plane3Color /*= AZ::Color(0.0f, 0.0f, 1.0f, 0.5f)*/)
    {
        const float planeSize = 0.6f;
        const AZ::Color planesColor[] = { plane1Color, plane2Color, plane3Color };

        for (size_t manipulatorIndex = 0; manipulatorIndex < m_planarManipulators.size(); ++manipulatorIndex)
        {
            const AZStd::shared_ptr<ManipulatorViewQuad> manipulatorView = CreateManipulatorViewQuad(
                *m_planarManipulators[manipulatorIndex], planesColor[manipulatorIndex], planesColor[(manipulatorIndex + 1) % 3], planeSize);

            m_planarManipulators[manipulatorIndex]->SetViews(ManipulatorViews{ manipulatorView });
        }
    }

    void TranslationManipulators::ConfigureSurfaceView(const float radius, const AZ::Color& color)
    {
        if (m_surfaceManipulator)
        {
            m_surfaceManipulator->SetView(CreateManipulatorViewSphere(
                color, radius,
                [](const ViewportInteraction::MouseInteraction& /*mouseInteraction*/, bool mouseOver,
                   const AZ::Color& defaultColor) -> AZ::Color
                {
                    const AZ::Color color[2] = {
                        defaultColor,
                        Vector3ToVector4(BaseManipulator::s_defaultMouseOverColor.GetAsVector3(), SurfaceManipulatorTransparency)
                    };

                    return color[mouseOver];
                }));
        }
    }

    void TranslationManipulators::ProcessManipulators(const AZStd::function<void(BaseManipulator*)>& manipulatorFn)
    {
        for (AZStd::shared_ptr<LinearManipulator>& manipulator : m_linearManipulators)
        {
            manipulatorFn(manipulator.get());
        }

        for (AZStd::shared_ptr<PlanarManipulator>& manipulator : m_planarManipulators)
        {
            manipulatorFn(manipulator.get());
        }

        if (m_surfaceManipulator)
        {
            manipulatorFn(m_surfaceManipulator.get());
        }
    }

    void TranslationManipulators::SetLineBoundWidth(const float lineBoundWidth)
    {
        m_lineBoundWidth = lineBoundWidth;
    }

    void ConfigureTranslationManipulatorAppearance3d(TranslationManipulators* translationManipulators)
    {
        translationManipulators->SetAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        translationManipulators->ConfigurePlanarView(LinearManipulatorXAxisColor, LinearManipulatorYAxisColor, LinearManipulatorZAxisColor);
        translationManipulators->ConfigureLinearView(
            LinearManipulatorAxisLength, LinearManipulatorXAxisColor, LinearManipulatorYAxisColor, LinearManipulatorZAxisColor);
        translationManipulators->ConfigureSurfaceView(SurfaceManipulatorRadius, SurfaceManipulatorColor);
    }

    void ConfigureTranslationManipulatorAppearance2d(TranslationManipulators* translationManipulators)
    {
        translationManipulators->SetAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY());
        translationManipulators->ConfigurePlanarView(LinearManipulatorXAxisColor);
        translationManipulators->ConfigureLinearView(LinearManipulatorAxisLength, LinearManipulatorXAxisColor, LinearManipulatorYAxisColor);
    }
} // namespace AzToolsFramework
