/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TubeShape.h"

#include <AzCore/Math/Transform.h>
#include <Shape/ShapeGeometryUtil.h>

#if LMBR_CENTRAL_EDITOR
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#endif

namespace LmbrCentral
{
    float Lerpf(const float from, const float to, const float fraction)
    {
        return AZ::Lerp(from, to, fraction);
    }

    AZ::Vector3 CalculateNormal(
        const AZ::Vector3& previousNormal, const AZ::Vector3& previousTangent, const AZ::Vector3& currentTangent)
    {
        // Rotates the previous normal by the angle difference between two tangent segments. Ensures
        // The normal is continuous along the tube.
        AZ::Vector3 normal = previousNormal;
        auto cosAngleBetweenTangentSegments = currentTangent.Dot(previousTangent);
        if (std::fabs(cosAngleBetweenTangentSegments) < 1.0f)
        {
            AZ::Vector3 axis = previousTangent.Cross(currentTangent);
            if (!axis.IsZero())
            {
                axis.Normalize();
                float angle = acosf(cosAngleBetweenTangentSegments);
                AZ::Quaternion rotationTangentDelta = AZ::Quaternion::CreateFromAxisAngle(axis, angle);
                normal = rotationTangentDelta.TransformVector(normal);
                normal.Normalize();
            }
        }
        return normal;
    }

    void TubeShape::Reflect(AZ::SerializeContext& context)
    {
        context.Class <TubeShape>()
            ->Version(1)
            ->Field("Radius", &TubeShape::m_radius)
            ->Field("VariableRadius", &TubeShape::m_variableRadius)
            ;

        if (auto editContext = context.GetEditContext())
        {
            editContext->Class<TubeShape>(
                "Tube Shape", "")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->DataElement(AZ::Edit::UIHandlers::Default, &TubeShape::m_radius, "Radius", "Radius of the tube")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.5f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &TubeShape::BaseRadiusChanged)
                ->DataElement(AZ::Edit::UIHandlers::Default, &TubeShape::m_variableRadius, "Variable Radius", "Variable radius along the tube")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &TubeShape::VariableRadiusChanged)
                    ;
        }
    }

    TubeShape::TubeShape(const TubeShape& rhs)
    {
        m_spline = rhs.m_spline;
        m_variableRadius = rhs.m_variableRadius;
        m_currentTransform = rhs.m_currentTransform;
        m_entityId = rhs.m_entityId;
        m_radius = rhs.m_radius;
    }


    void TubeShape::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        TubeShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        SplineComponentNotificationBus::Handler::BusConnect(m_entityId);

        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        SplineComponentRequestBus::EventResult(m_spline, m_entityId, &SplineComponentRequests::GetSpline);

        m_variableRadius.Activate(entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
    }

    void TubeShape::Deactivate()
    {
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        m_variableRadius.Deactivate();
        SplineComponentNotificationBus::Handler::BusDisconnect();
        TubeShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void TubeShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        {
            AZStd::unique_lock lock(m_mutex);
            m_currentTransform = world;
        }
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    void TubeShape::SetRadius(float radius)
    {
        {
            AZStd::unique_lock lock(m_mutex);
            m_radius = radius;
            ValidateAllVariableRadii();
        }

        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    float TubeShape::GetRadius() const
    {
        AZStd::shared_lock lock(m_mutex);
        return m_radius;
    }

    void TubeShape::SetVariableRadius(int vertIndex, float radius)
    {
        {
            AZStd::unique_lock lock(m_mutex);
            m_variableRadius.SetElement(vertIndex, radius);
            ValidateVariableRadius(vertIndex);
        }

        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void TubeShape::SetAllVariableRadii(float radius)
    {
        {
            AZStd::unique_lock lock(m_mutex);
            for (size_t vertIndex = 0; vertIndex < m_variableRadius.Size(); ++vertIndex)
            {
                m_variableRadius.SetElement(vertIndex, radius);
                ValidateVariableRadius(vertIndex);
            }
        }

        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    float TubeShape::GetVariableRadius(int vertIndex) const
    {
        AZStd::shared_lock lock(m_mutex);
        return m_variableRadius.GetElement(vertIndex);
    }

    float TubeShape::GetTotalRadius(const AZ::SplineAddress& address) const
    {
        AZStd::shared_lock lock(m_mutex);
        return m_radius + m_variableRadius.GetElementInterpolated(address, Lerpf);
    }

    const SplineAttribute<float>& TubeShape::GetRadiusAttribute() const
    {
        AZStd::shared_lock lock(m_mutex);
        return m_variableRadius;
    }

    void TubeShape::OnSplineChanged()
    {
        SplineComponentRequestBus::EventResult(m_spline, m_entityId, &SplineComponentRequests::GetSpline);
        ShapeComponentNotificationsBus::Event(m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    AZ::SplinePtr TubeShape::GetSpline()
    {
        AZStd::shared_lock lock(m_mutex);
        if (!m_spline)
        {
            SplineComponentRequestBus::EventResult(m_spline, m_entityId, &SplineComponentRequests::GetSpline);
            AZ_Assert(m_spline, "A TubeShape must have a Spline to work");
        }

        return m_spline;
    }

    AZ::ConstSplinePtr TubeShape::GetConstSpline() const
    {
        AZStd::shared_lock lock(m_mutex);
        if (!m_spline)
        {
            SplineComponentRequestBus::EventResult(m_spline, m_entityId, &SplineComponentRequests::GetSpline);
            AZ_Assert(m_spline, "A TubeShape must have a Spline to work");
        }

        return m_spline;
    }

    static AZ::Aabb CalculateTubeBounds(const TubeShape& tubeShape, const AZ::Transform& transform)
    {
        const auto maxScale = transform.GetUniformScale();
        const auto scaledRadiusFn =
            [&tubeShape, maxScale](const AZ::SplineAddress& splineAddress)
        {
            return tubeShape.GetTotalRadius(splineAddress) * maxScale;
        };

        const auto& spline = tubeShape.GetConstSpline();
        // approximate aabb - not exact but is guaranteed to encompass tube
        float maxRadius = scaledRadiusFn(AZ::SplineAddress(0, 0.0f));
        for (size_t i = 0; i < spline->GetVertexCount(); ++i)
        {
            maxRadius = AZ::GetMax(maxRadius, scaledRadiusFn(AZ::SplineAddress(i, 1.0f)));
        }

        AZ::Aabb aabb;
        spline->GetAabb(aabb, transform);
        aabb.Expand(AZ::Vector3(maxRadius));

        return aabb;
    }

    AZ::Aabb TubeShape::GetEncompassingAabb() const
    {
        AZStd::shared_lock lock(m_mutex);
        if (m_spline == nullptr)
        {
            return AZ::Aabb::CreateNull();
        }

        AZ::Transform worldFromLocalUniformScale = m_currentTransform;
        worldFromLocalUniformScale.SetUniformScale(worldFromLocalUniformScale.GetUniformScale());

        return CalculateTubeBounds(*this, worldFromLocalUniformScale);
    }

    void TubeShape::GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) const
    {
        AZStd::shared_lock lock(m_mutex);
        bounds = CalculateTubeBounds(*this, AZ::Transform::CreateIdentity());
        transform = m_currentTransform;
    }

    bool TubeShape::IsPointInside(const AZ::Vector3& point) const
    {
        AZStd::shared_lock lock(m_mutex);
        if (m_spline == nullptr)
        {
            return false;
        }

        AZ::Transform worldFromLocalNormalized = m_currentTransform;
        const float scale = worldFromLocalNormalized.ExtractUniformScale();
        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverse();
        const AZ::Vector3 localPoint = localFromWorldNormalized.TransformPoint(point) / scale;

        const auto address = m_spline->GetNearestAddressPosition(localPoint).m_splineAddress;
        const float radiusSq = powf(m_radius, 2.0f);
        const float variableRadiusSq =
            powf(m_variableRadius.GetElementInterpolated(address, Lerpf), 2.0f);

        return (m_spline->GetPosition(address) - localPoint).GetLengthSq() < (radiusSq + variableRadiusSq) * scale;
    }

    float TubeShape::DistanceFromPoint(const AZ::Vector3& point) const
    {
        AZStd::shared_lock lock(m_mutex);
        AZ::Transform worldFromLocalNormalized = m_currentTransform;
        const float uniformScale = worldFromLocalNormalized.ExtractUniformScale();
        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverse();
        const AZ::Vector3 localPoint = localFromWorldNormalized.TransformPoint(point) / uniformScale;

        const auto splineQueryResult = m_spline->GetNearestAddressPosition(localPoint);
        const float variableRadius =
            m_variableRadius.GetElementInterpolated(splineQueryResult.m_splineAddress, Lerpf);

        // Make sure the distance is clamped to 0 for all points that exist within the tube.
        return AZStd::max(0.0f, (sqrtf(splineQueryResult.m_distanceSq) - (m_radius + variableRadius)) * uniformScale);
    }

    float TubeShape::DistanceSquaredFromPoint(const AZ::Vector3& point) const
    {
        float distance = DistanceFromPoint(point);
        return powf(distance, 2.0f);
    }

    bool TubeShape::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) const
    {
        AZStd::shared_lock lock(m_mutex);
        const auto splineQueryResult = IntersectSpline(m_currentTransform, src, dir, *m_spline);
        const float variableRadius = m_variableRadius.GetElementInterpolated(
            splineQueryResult.m_splineAddress, Lerpf);

        const float totalRadius = m_radius + variableRadius;
        distance = (splineQueryResult.m_rayDistance - totalRadius) * m_currentTransform.GetUniformScale();

        return static_cast<float>(sqrtf(splineQueryResult.m_distanceSq)) < totalRadius;
    }

    /// Generates all vertex positions. Assumes the vertex pointer is valid
    static void GenerateSolidTubeMeshVertices(
        const AZ::SplinePtr& spline, const SplineAttribute<float>& variableRadius,
        const float radius, const AZ::u32 sides, const AZ::u32 capSegments, AZ::Vector3* vertices)
    {
        // start cap
        auto address = spline->GetAddressByFraction(0.0f);
        AZ::Vector3 normal = spline->GetNormal(address);
        AZ::Vector3 previousTangent = spline->GetTangent(address);
        if (capSegments > 0)
        {
            vertices = CapsuleTubeUtil::GenerateSolidStartCap(
                spline->GetPosition(address),
                previousTangent,
                normal,
                radius + variableRadius.GetElementInterpolated(address, Lerpf),
                sides, capSegments, vertices);
        }

        // middle segments (body)
        const float stepDelta = 1.0f / static_cast<float>(spline->GetSegmentGranularity());
        const auto endIndex = address.m_segmentIndex + spline->GetSegmentCount();

        while (address.m_segmentIndex < endIndex)
        {
            for (auto step = 0; step <= spline->GetSegmentGranularity(); ++step)
            {
                const AZ::Vector3 currentTangent = spline->GetTangent(address);
                normal = CalculateNormal(normal, previousTangent, currentTangent);

                vertices = CapsuleTubeUtil::GenerateSegmentVertices(
                    spline->GetPosition(address),
                    currentTangent,
                    normal,
                    radius + variableRadius.GetElementInterpolated(address, Lerpf),
                    sides, vertices);

                address.m_segmentFraction += stepDelta;
                previousTangent = currentTangent;
            }
            address.m_segmentIndex++;
            address.m_segmentFraction = 0.f;
        }

        // end cap
        if (capSegments > 0)
        {
            const auto endAddress = spline->GetAddressByFraction(1.0f);
            CapsuleTubeUtil::GenerateSolidEndCap(
                spline->GetPosition(endAddress),
                spline->GetTangent(endAddress),
                normal,
                radius + variableRadius.GetElementInterpolated(endAddress, Lerpf),
                sides, capSegments, vertices);
        }
    }
    /*
        Generates vertices and indices for a tube shape
        Split into two stages:
        - Generate vertex positions
        - Generate indices (faces)
          Heres a rough diagram of how it is built:
           ____________
          /_|__|__|__|_\
          \_|__|__|__|_/
         - A single vertex at each end of the tube
         - Angled end cap segments
         - Middle segments
    */
    void GenerateSolidTubeMesh(
        const AZ::SplinePtr& spline, const SplineAttribute<float>& variableRadius,
        const float radius, const AZ::u32 capSegments, const AZ::u32 sides,
        AZStd::vector<AZ::Vector3>& vertexBufferOut,
        AZStd::vector<AZ::u32>& indexBufferOut)
    {
        const size_t segmentCount = spline->GetSegmentCount();
        if (segmentCount == 0)
        {
            // clear the buffers so we no longer draw anything
            vertexBufferOut.clear();
            indexBufferOut.clear();
            return;
        }

        const AZ::u32 segments = static_cast<AZ::u32>(segmentCount * spline->GetSegmentGranularity() + segmentCount - 1);
        const AZ::u32 totalSegments = segments + capSegments * 2;
        const AZ::u32 capSegmentTipVerts = capSegments > 0 ? 2 : 0;
        const size_t numVerts = sides * (totalSegments + 1) + capSegmentTipVerts;
        const size_t numTriangles = (sides * totalSegments) * 2 + (sides * capSegmentTipVerts);

        vertexBufferOut.resize(numVerts);
        indexBufferOut.resize(numTriangles * 3);

        GenerateSolidTubeMeshVertices(
            spline, variableRadius, radius,
            sides, capSegments, &vertexBufferOut[0]);

        CapsuleTubeUtil::GenerateSolidMeshIndices(
            sides, segments, capSegments, &indexBufferOut[0]);
    }

    /// Compose Caps, Lines and Loops to produce a final wire mesh matching the style of other
    /// debug draw components.
    void GenerateWireTubeMesh(
        const AZ::SplinePtr& spline, const SplineAttribute<float>& variableRadius,
        const float radius, const AZ::u32 capSegments, const AZ::u32 sides,
        AZStd::vector<AZ::Vector3>& vertexBufferOut)
    {
        const size_t segmentCount = spline->GetSegmentCount();
        if (segmentCount == 0)
        {
            // clear the buffer so we no longer draw anything
            vertexBufferOut.clear();
            return;
        }

        // notes on vert buffer size
        // total end segments
        // 2 verts for each segment
        //  2 * capSegments for one full half arc
        //   2 arcs per end
        //    2 ends
        // total segments
        // 2 verts for each segment
        //  2 lines - top and bottom
        //   2 lines - left and right
        // total loops
        // 2 verts for each segment
        //  loops == sides
        //   2 loops per segment
        const AZ::u32 segments = static_cast<AZ::u32>(segmentCount * spline->GetSegmentGranularity());
        const AZ::u32 totalEndSegments = capSegments * 2 * 2 * 2 * 2;
        const AZ::u32 totalSegments = segments * 2 * 2 * 2;
        const AZ::u32 totalLoops = 2 * sides * segments * 2;
        const bool hasEnds = capSegments > 0;
        const size_t numVerts = totalEndSegments + totalSegments + totalLoops;
        vertexBufferOut.resize(numVerts);

        AZ::Vector3* vertices = vertexBufferOut.data();

        // start cap
        auto address = spline->GetAddressByFraction(0.0f);
        AZ::Vector3 side = spline->GetNormal(address);
        AZ::Vector3 nextSide = spline->GetNormal(address);
        AZ::Vector3 previousDirection = spline->GetTangent(address);
        if (hasEnds)
        {
            vertices = CapsuleTubeUtil::GenerateWireCap(
                spline->GetPosition(address),
                -previousDirection,
                side,
                radius + variableRadius.GetElementInterpolated(address, Lerpf),
                capSegments,
                vertices);
        }

        // body
        const float stepDelta = 1.0f / static_cast<float>(spline->GetSegmentGranularity());
        auto nextAddress = address;
        const auto endIndex = address.m_segmentIndex + segmentCount;
        while (address.m_segmentIndex < endIndex)
        {
            address.m_segmentFraction = 0.f;
            nextAddress.m_segmentFraction = stepDelta;

            for (auto step = 0; step < spline->GetSegmentGranularity(); ++step)
            {
                const auto position = spline->GetPosition(address);
                const auto nextPosition = spline->GetPosition(nextAddress);
                const auto direction = spline->GetTangent(address);
                const auto nextDirection = spline->GetTangent(nextAddress);
                side = spline->GetNormal(address);
                nextSide = spline->GetNormal(nextAddress);
                const auto up = side.Cross(direction);
                const auto nextUp = nextSide.Cross(nextDirection);
                const auto finalRadius = radius + variableRadius.GetElementInterpolated(address, Lerpf);
                const auto nextFinalRadius = radius + variableRadius.GetElementInterpolated(nextAddress, Lerpf);

                // left line
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        position, direction, side, finalRadius, 0.0f),
                    vertices);
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        nextPosition, nextDirection, nextSide, nextFinalRadius, 0.0f),
                    vertices);
                // right line
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        position, -direction, side, finalRadius, AZ::Constants::Pi),
                    vertices);
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        nextPosition, -nextDirection, nextSide, nextFinalRadius, AZ::Constants::Pi),
                    vertices);
                // top line
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        position, direction, up, finalRadius, 0.0f),
                    vertices);
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        nextPosition, nextDirection, nextUp, nextFinalRadius, 0.0f),
                    vertices);
                // bottom line
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        position, -direction, up, finalRadius, AZ::Constants::Pi),
                    vertices);
                vertices = WriteVertex(
                    CapsuleTubeUtil::CalculatePositionOnSphere(
                        nextPosition, -nextDirection, nextUp, nextFinalRadius, AZ::Constants::Pi),
                    vertices);

                // loops along each segment
                vertices = CapsuleTubeUtil::GenerateWireLoop(
                    position, direction, side, sides, finalRadius, vertices);

                vertices = CapsuleTubeUtil::GenerateWireLoop(
                    nextPosition, nextDirection, nextSide, sides, nextFinalRadius, vertices);

                address.m_segmentFraction += stepDelta;
                nextAddress.m_segmentFraction += stepDelta;
                previousDirection = direction;
            }

            address.m_segmentIndex++;
            nextAddress.m_segmentIndex++;
        }

        if (hasEnds)
        {
            const auto endAddress = spline->GetAddressByFraction(1.0f);
            const auto endPosition = spline->GetPosition(endAddress);
            const auto endDirection = spline->GetTangent(endAddress);
            const auto endRadius =
                radius + variableRadius.GetElementInterpolated(endAddress, Lerpf);

            // end cap
            CapsuleTubeUtil::GenerateWireCap(
                endPosition, endDirection,
                nextSide, endRadius, capSegments,
                vertices);
        }
    }

    void GenerateTubeMesh(
        const AZ::SplinePtr& spline, const SplineAttribute<float>& variableRadius,
        const float radius, const AZ::u32 capSegments, const AZ::u32 sides,
        AZStd::vector<AZ::Vector3>& vertexBufferOut, AZStd::vector<AZ::u32>& indexBufferOut,
        AZStd::vector<AZ::Vector3>& lineBufferOut)
    {
        GenerateSolidTubeMesh(
            spline, variableRadius, radius,
            capSegments, sides, vertexBufferOut,
            indexBufferOut);

        GenerateWireTubeMesh(
            spline, variableRadius, radius,
            capSegments, sides, lineBufferOut);
    }

    void TubeShapeMeshConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TubeShapeMeshConfig>()
                ->Version(2)
                ->Field("EndSegments", &TubeShapeMeshConfig::m_endSegments)
                ->Field("Sides", &TubeShapeMeshConfig::m_sides)
                ->Field("ShapeConfig", &TubeShapeMeshConfig::m_shapeComponentConfig)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TubeShapeMeshConfig>("Configuration", "Tube Shape Mesh Configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &TubeShapeMeshConfig::m_endSegments, "End Segments",
                        "Number Of segments at each end of the tube in the editor")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 10)
                        ->Attribute(AZ::Edit::Attributes::Step, 1)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &TubeShapeMeshConfig::m_sides, "Sides",
                        "Number of Sides of the tube in the editor")
                        ->Attribute(AZ::Edit::Attributes::Min, 3)
                        ->Attribute(AZ::Edit::Attributes::Max, 32)
                        ->Attribute(AZ::Edit::Attributes::Step, 1)
                        ;
            }
        }
    }

    void TubeShape::BaseRadiusChanged()
    {
        // ensure all variable radii stay in bounds should the base radius
        // change and cause the resulting total radius to be negative
        ValidateAllVariableRadii();
    }

    void TubeShape::VariableRadiusChanged(size_t vertIndex)
    {
        ValidateVariableRadius(vertIndex);
    }

    void TubeShape::ValidateVariableRadius(const size_t vertIndex)
    {
        // if the total radius is less than 0, adjust the variable radius
        // to ensure the total radius stays positive
        float totalRadius = m_radius + m_variableRadius.GetElementInterpolated(AZ::SplineAddress(vertIndex), Lerpf);
        if (totalRadius < 0.0f)
        {
            m_variableRadius.SetElement(vertIndex, -m_radius);
        }
    }

    void TubeShape::ValidateAllVariableRadii()
    {
        for (size_t vertIndex = 0; vertIndex < m_spline->GetVertexCount(); ++vertIndex)
        {
            ValidateVariableRadius(vertIndex);
        }
    }
} // namespace LmbrCentral
