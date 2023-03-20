/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Scene.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>
#include <CoreLights/PolygonLightDelegate.h>

namespace AZ::Render
{
    PolygonLightDelegate::PolygonLightDelegate(LmbrCentral::PolygonPrismShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible)
        : LightDelegateBase<PolygonLightFeatureProcessorInterface>(entityId, isVisible)
        , m_shapeBus(shapeBus)
    {
        InitBase(entityId);
        shapeBus->SetHeight(0.0f);
    }

    void PolygonLightDelegate::SetLightEmitsBothDirections(bool lightEmitsBothDirections)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetLightEmitsBothDirections(GetLightHandle(), lightEmitsBothDirections);
        }
    }

    float PolygonLightDelegate::CalculateAttenuationRadius(float lightThreshold) const
    {
        // Calculate the radius at which the irradiance will be equal to cutoffIntensity.
        const float intensity = GetPhotometricValue().GetCombinedIntensity(PhotometricUnit::Lumen);
        return Sqrt(intensity / lightThreshold);
    }

    void PolygonLightDelegate::HandleShapeChanged()
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetPosition(GetLightHandle(), GetTransform().GetTranslation());

            AZStd::vector<Vector2> vertices = m_shapeBus->GetPolygonPrism()->m_vertexContainer.GetVertices();

            Transform transform = GetTransform();
            transform.SetUniformScale(transform.GetUniformScale()); // Polygon Prism only supports uniform scale.

            AZStd::vector<Vector3> transformedVertices;
            transformedVertices.reserve(vertices.size());
            for (const Vector2& vertex : vertices)
            {
                transformedVertices.push_back(transform.TransformPoint(Vector3(vertex, 0.0f)));
            }

            GetFeatureProcessor()->SetPolygonPoints(
                GetLightHandle(),
                transformedVertices.data(),
                static_cast<uint32_t>(transformedVertices.size()),
                GetTransform().GetBasisZ());
        }
    }

    float PolygonLightDelegate::GetSurfaceArea() const
    {
        // Surprisingly simple to calculate area of arbitrary polygon assuming no self-intersection. See
        // https://en.wikipedia.org/wiki/Shoelace_formula
        float twiceArea = 0.0f;
        AZStd::vector<Vector2> vertices = m_shapeBus->GetPolygonPrism()->m_vertexContainer.GetVertices();
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            size_t j = (i + 1) % vertices.size();
            twiceArea += vertices.at(i).GetX() * vertices.at(j).GetY();
            twiceArea -= vertices.at(i).GetY() * vertices.at(j).GetX();
        }
        float scale = GetTransform().GetUniformScale();
        return GetAbs(twiceArea * 0.5f * scale * scale);
    }

    void PolygonLightDelegate::DrawDebugDisplay(
        const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
    {
        if (isSelected)
        {
            debugDisplay.SetColor(color);

            // Draw a Polygon for the attenuation radius
            debugDisplay.DrawWireSphere(transform.GetTranslation(), GetConfig()->m_attenuationRadius);
            debugDisplay.DrawArrow(transform.GetTranslation(), transform.GetTranslation() + transform.GetBasisZ());
        }
    }

    Aabb PolygonLightDelegate::GetLocalVisualizationBounds() const
    {
        return Aabb::CreateCenterRadius(Vector3::CreateZero(), GetConfig()->m_attenuationRadius);
    }
} // namespace AZ::Render
