/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Spline.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    const float Spline::s_splineEpsilon = 0.00001f;
    const float SplineAddress::s_segmentFractionEpsilon = 0.001f;
    static const float s_projectRayLength = 1000.0f;
    static const u16 s_minGranularity = 2;
    static const u16 s_maxGranularity = 64;

    namespace Internal
    {
        /**
         * Script Wrapper for SplineAddress default constructor.
         */
        void SplineAddressDefaultConstructor(SplineAddress* thisPtr)
        {
            new (thisPtr) SplineAddress();
        }

        /**
         * Script Wrapper for SplineAddress constructor overloads.
         */
        void SplineAddressScriptConstructor(SplineAddress* thisPtr, ScriptDataContext& dc)
        {
            const int numArgs = dc.GetNumArguments();
            switch (numArgs)
            {
            case 1:
                if (dc.IsNumber(0))
                {
                    u64 index = 0;
                    dc.ReadArg(0, index);
                    *thisPtr = SplineAddress(index);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 1 argument to SplineAddress(), it must be a number (index)");
                }
                break;
            case 2:
                if (dc.IsNumber(0) && dc.IsNumber(1))
                {
                    u64 index = 0;
                    float fraction = 0.0f;
                    dc.ReadArg(0, index);
                    dc.ReadArg(1, fraction);
                    *thisPtr = SplineAddress(index, fraction);
                }
                else
                {
                    dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 2 arguments to SplineAddress(), both must be a number (index, fraction)");
                }
                break;
            default:
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "SplineAddress() accepts only 1 or 2 arguments, not %d!", numArgs);
                break;
            }
        }
    }

    void SplineReflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            Spline::Reflect(*serializeContext);
            LinearSpline::Reflect(*serializeContext);
            BezierSpline::Reflect(*serializeContext);
            CatmullRomSpline::Reflect(*serializeContext);

            serializeContext->Class<SplineAddress>()->
                Field("SegmentIndex", &SplineAddress::m_segmentIndex)->
                Field("SegmentFraction", &SplineAddress::m_segmentFraction);

            serializeContext->Class<PositionSplineQueryResult>()->
                Field("SplineAddress", &PositionSplineQueryResult::m_splineAddress)->
                Field("DistanceSq", &PositionSplineQueryResult::m_distanceSq);

            serializeContext->Class<RaySplineQueryResult, PositionSplineQueryResult>()->
                Field("RayDistance", &RaySplineQueryResult::m_rayDistance);
        }

        if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<SplineAddress>()->
                Constructor<u64, float>()->
                Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::Value)->
                Attribute(Script::Attributes::ConstructorOverride, &Internal::SplineAddressScriptConstructor)->
                Attribute(Script::Attributes::GenericConstructorOverride, &Internal::SplineAddressDefaultConstructor)->
                Property("segmentIndex", BehaviorValueProperty(&SplineAddress::m_segmentIndex))->
                Property("segmentFraction", BehaviorValueProperty(&SplineAddress::m_segmentFraction))->
                Method("GetSegmentIndexAndFraction", [](SplineAddress& thisPtr)
                    {
                        return AZStd::make_tuple(thisPtr.m_segmentIndex, thisPtr.m_segmentFraction);
                    })
                ;

            behaviorContext->Class<PositionSplineQueryResult>()->
                Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::Value)->
                Property("splineAddress", [](PositionSplineQueryResult& thisPtr) { return thisPtr.m_splineAddress; }, nullptr)->
                Property("distanceSq", [](PositionSplineQueryResult& thisPtr) { return thisPtr.m_distanceSq; }, nullptr);

            behaviorContext->Class<RaySplineQueryResult>()->
                Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::Value)->
                Property("splineAddress", [](RaySplineQueryResult& thisPtr) { return thisPtr.m_splineAddress; }, nullptr)->
                Property("distanceSq", [](RaySplineQueryResult& thisPtr) { return thisPtr.m_distanceSq; }, nullptr)->
                Property("rayDistance", [](RaySplineQueryResult& thisPtr) { return thisPtr.m_rayDistance; }, nullptr);

            behaviorContext->Class<Spline>()->
                Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::RuntimeOwn)->
                Method("GetNearestAddressRay", &Spline::GetNearestAddressRay)->
                Method("GetNearestAddressPosition", &Spline::GetNearestAddressPosition)->
                Method("GetAddressByDistance", &Spline::GetAddressByDistance)->
                Method("GetAddressByFraction", &Spline::GetAddressByFraction)->
                Method("GetPosition", &Spline::GetPosition)->
                Method("GetNormal", &Spline::GetNormal)->
                Method("GetTangent", &Spline::GetTangent)->
                Method("GetLength", &Spline::GetLength)->
                Method("GetSplineLength", &Spline::GetSplineLength)->
                Method("GetSegmentLength", &Spline::GetSegmentLength)->
                Method("GetSegmentCount", &Spline::GetSegmentCount)->
                Method("GetSegmentGranularity", &Spline::GetSegmentGranularity)->
                Method("GetAabb", [](Spline& thisPtr, const Transform& transform) { Aabb aabb; thisPtr.GetAabb(aabb, transform); return aabb; })->
                Method("IsClosed", &Spline::IsClosed)->
                Property("vertexContainer", BehaviorValueGetter(&Spline::m_vertexContainer), nullptr);
        }
    }

    /**
     * Helper to calculate segment length for generic spline (bezier/catmull-rom etc) using piecewise interpolation.
     * (Break spline into linear segments and sum length of each).
     */
    static float CalculateSegmentLengthPiecewise(const Spline& spline, size_t index)
    {
        const size_t granularity = spline.GetSegmentGranularity();

        float length = 0.0f;
        Vector3 pos = spline.GetPosition(SplineAddress(index, 0.0f));
        for (size_t i = 1; i <= granularity; ++i)
        {
            Vector3 nextPos = spline.GetPosition(SplineAddress(index, i / static_cast<float>(granularity)));
            length += (nextPos - pos).GetLength();
            pos = nextPos;
        }

        return length;
    }

    /**
     * Helper to calculate aabb for generic spline (bezier/catmull-rom etc) using piecewise interpolation.
     */
    static void CalculateAabbPiecewise(const Spline& spline, size_t begin, size_t end, Aabb& aabb, const Transform& transform /*= Transform::CreateIdentity()*/)
    {
        aabb.SetNull();

        const size_t granularity = spline.GetSegmentGranularity();
        for (size_t index = begin; index < end; ++index)
        {
            for (size_t granularStep = 0; granularStep <= granularity; ++granularStep)
            {
                aabb.AddPoint(transform.TransformPoint(spline.GetPosition(SplineAddress(index, granularStep / static_cast<float>(granularity)))));
            }
        }
    }

    /**
     * Build a SplineAddress given how far along a segment in a spline we are.
     * A segment is composed of 1-* steps (number of steps is defined by the granularity).
     * Each step is a line segment, taken in aggregate they approximate the curve.
     * @param step What step through the segment are we.
     * @param granularity How many steps are there in this segment
     * @param proportion How far along the the individual step are we
     */
    AZ_FORCE_INLINE SplineAddress CalculateSplineAdddress(size_t currentVertex, size_t step, float granularity, float proportion)
    {
        return SplineAddress(currentVertex, // the segment/section along the spline
            (step - 1) / granularity + // starting step as percentage/proportion
            proportion / granularity); // proportion along individual step (line segment)
    }

    /**
     * Intermediate/temporary result computed by CalculateDistanceFunc in GetNearestAddressInternal.
     * IntermediateQueryResult is used by RayIntermediateQueryResult and PosIntermediateQueryResult
     * depending on what we're testing against the spline.
     */
    struct IntermediateQueryResult
    {
        IntermediateQueryResult(float distanceSq, float stepProportion)
            : m_distanceSq(distanceSq)
            , m_stepProportion(stepProportion) {}

        float m_distanceSq; ///< Distance Squared from the closest point of the test to the spline (point or ray).
        float m_stepProportion; ///< Proportion along individual step (line segment).
    };

    /**
     * Used to store minimum distance values of closest position to spline for position.
     */
    struct PosMinResult
    {
        float m_minDistanceSq = std::numeric_limits<float>::max();
    };

    /**
     * Used to store minimum distance values of closest position to spline for ray.
     */
    struct RayMinResult : PosMinResult
    {
        float m_rayDistance = std::numeric_limits<float>::max();
    };

    /**
     * Intermediate result of ray spline query - used to store initial result of query
     * before being combined with current state of spline query to build a spline address.
     * (use if we know distanceSq is less than current best known minDistanceSq)
     */
    struct RayIntermediateQueryResult : IntermediateQueryResult
    {
        RayIntermediateQueryResult(float distanceSq, float stepProportion, float rayClosestDistance)
            : IntermediateQueryResult(distanceSq, stepProportion)
            , m_rayDistance(rayClosestDistance) {}

        float m_rayDistance; ///< The distance along the ray the point closest to the spline is.

        /**
         * Create final spline query result by combining intermediate query results with additional information
         * about the state of spline to create a spline address.
         */
        RaySplineQueryResult Build(size_t currentVertex, size_t step, float granularity) const
        {
            return RaySplineQueryResult(
                CalculateSplineAdddress(currentVertex, step, granularity, m_stepProportion),
                m_distanceSq,
                m_rayDistance);
        }

        /**
         * Update minimum distance struct bases on current distance from spline.
         * For ray, favor shortest distance along spline, as well as closest point on ray to spline.
         * Note: This helps give expected results with parallel lines/splines and rays.
         */
        bool CompareLess(RayMinResult& rayMinResult) const
        {
            const float delta = m_distanceSq - rayMinResult.m_minDistanceSq;
            const bool zero = AZ::IsCloseMag(delta, 0.0f, 0.0001f);
            const bool lessThan = m_distanceSq < rayMinResult.m_minDistanceSq;

            if (lessThan || ((zero || lessThan) && m_rayDistance < rayMinResult.m_rayDistance))
            {
                rayMinResult.m_rayDistance = m_rayDistance;
                rayMinResult.m_minDistanceSq = m_distanceSq;
                return true;
            }

            return false;
        }
    };

    /**
     * Functor to wrap ray query against a line segment - used in conjunction with GetNearestAddressInternal.
     */
    struct RayQuery
    {
        explicit RayQuery(const Vector3& localRaySrc, const Vector3& localRayDir)
            : m_localRaySrc(localRaySrc)
            , m_localRayDir(localRayDir) {}

        /**
         * Using the ray src/origin and direction, calculate the closest point on a step (line segment)
         * between stepBegin and stepEnd. Return an intermediate result object with the shortest distance
         * between the ray and the step, the proportion along the step and the distance along the
         * ray the closest point to the spline is.
         */
        RayIntermediateQueryResult operator()(const Vector3& stepBegin, const Vector3& stepEnd) const
        {
            const Vector3 localRayEnd = m_localRaySrc + m_localRayDir * s_projectRayLength;
            Vector3 closestPosRay, closestPosSegmentStep;
            float rayProportion, segmentStepProportion;
            Intersect::ClosestSegmentSegment(
                m_localRaySrc, localRayEnd, stepBegin, stepEnd,
                rayProportion, segmentStepProportion, closestPosRay, closestPosSegmentStep);
            return RayIntermediateQueryResult(
                (closestPosRay - closestPosSegmentStep).GetLengthSq(),
                segmentStepProportion,
                rayProportion * s_projectRayLength);
        }

        Vector3 m_localRaySrc;
        Vector3 m_localRayDir;
    };

    /**
     * Intermediate result of position spline query - used to store initial result of query
     * before being combined with current state of spline query to build a spline address.
     * (use if we know distanceSq is less than current best known minDistanceSq)
     */
    struct PosIntermediateQueryResult : IntermediateQueryResult
    {
        PosIntermediateQueryResult(float distanceSq, float stepProportion)
            : IntermediateQueryResult(distanceSq, stepProportion) {}

        /**
         * Create final spline query result by combining intermediate query results with additional information
         * about the state of spline to create a spline address.
         */
        PositionSplineQueryResult Build(size_t currentVertex, size_t step, float granularity) const
        {
            return PositionSplineQueryResult(
                CalculateSplineAdddress(currentVertex, step, granularity, m_stepProportion),
                m_distanceSq);
        }

        /**
         * Update minimum distance struct bases on current distance from spline.
         */
        bool CompareLess(PosMinResult& posMinResult) const
        {
            if (m_distanceSq < posMinResult.m_minDistanceSq)
            {
                posMinResult.m_minDistanceSq = m_distanceSq;
                return true;
            }

            return false;
        }
    };

    /**
     * Functor to wrap position query against a line segment - used in conjunction with GetNearestAddressInternal.
     */
    struct PosQuery
    {
        explicit PosQuery(const Vector3& localPos)
            : m_localPos(localPos) {}

        /**
         * Using the position, calculate the closest point on a step (line segment)
         * between stepBegin and stepEnd. Return an intermediate result object with the shortest distance
         * between the point and the step and the proportion along the step.
         */
        PosIntermediateQueryResult operator()(const Vector3& stepBegin, const Vector3& stepEnd) const
        {
            Vector3 closestPosSegmentStep;
            float segmentStepProportion;
            Intersect::ClosestPointSegment(m_localPos, stepBegin, stepEnd, segmentStepProportion, closestPosSegmentStep);
            return PosIntermediateQueryResult((m_localPos - closestPosSegmentStep).GetLengthSq(), segmentStepProportion);
        }

        Vector3 m_localPos;
    };

    /**
     * Template function to perform a generic distance query against line segments composing a spline.
     * Iterate through each segment in the spline (section between vertices) and within each segment
     * iterate through each step (line). Return the address corresponding to the closest point on the spline.
     *
     * @param begin Vertex to start iteration on.
     * @param end Vertex to end iteration on.
     * @param granularity Number of line segments making up each segment (tessellation).
     * @param spline Current spline the query is being conducted on - GetPosition call required.
     * @param calcDistfunc The functor responsible for doing the distance query - returning min distance and proportion along segment.
     * @return SplineAddress closest to given query on spline.
     */
    template<typename CalculateDistanceFunc, typename IntermediateResult, typename QueryResult, typename MinResult>
    static QueryResult GetNearestAddressInternal(
        const Spline& spline, size_t begin, size_t end, size_t granularity, CalculateDistanceFunc calcDistfunc)
    {
        MinResult minResult;
        QueryResult queryResult;
        for (size_t currentVertex = begin; currentVertex < end; ++currentVertex)
        {
            Vector3 segmentStepBegin = spline.GetPosition(SplineAddress(currentVertex, 0.0f));
            for (size_t granularStep = 1; granularStep <= granularity; ++granularStep)
            {
                const Vector3 segmentStepEnd = spline.GetPosition(
                    SplineAddress(currentVertex, granularStep / static_cast<float>(granularity)));

                IntermediateResult intermediateResult = calcDistfunc(segmentStepBegin, segmentStepEnd);

                if (intermediateResult.CompareLess(minResult))
                {
                    queryResult = intermediateResult.Build(currentVertex, granularStep, static_cast<float>(granularity));
                }

                segmentStepBegin = segmentStepEnd;
            }
        }

        return queryResult;
    }

    /**
     * Util function to calculate SplineAddress from a given distance along the spline.
     * Negative distances will return first address.
     * @param begin Vertex to start iteration on.
     * @param end Vertex to end iteration on.
     * @param spline Current spline the query is being conducted on.
     * @param distance Distance along the spline.
     * @return SplineAddress closest to given distance.
     */
    static SplineAddress GetAddressByDistanceInternal(const Spline& spline, size_t begin, size_t end, float distance)
    {
        if (distance < 0.0f)
        {
            return SplineAddress(begin);
        }

        size_t index = begin;
        float segmentLength = 0.0f;
        float currentSplineLength = 0.0f;
        for (; index < end; ++index)
        {
            segmentLength = spline.GetSegmentLength(index);
            if (currentSplineLength + segmentLength > distance)
            {
                break;
            }

            currentSplineLength += segmentLength;
        }

        if (index == end)
        {
            return SplineAddress(index - 1, 1.0f);
        }

        return SplineAddress(index, segmentLength > 0.0f ? ((distance - currentSplineLength) / segmentLength) : 0.0f);
    }

    /**
     * Util function to calculate SplineAddress from a given percentage along the spline.
     * Implemented in terms of GetAddressByDistanceInternal.
     * @param begin Vertex to start iteration on.
     * @param end Vertex to end iteration on.
     * @param spline Current spline the query is being conducted on.
     * @param fraction Fraction/percentage along the spline.
     * @return SplineAddress closest to given fraction/percentage.
     */
    static SplineAddress GetAddressByFractionInternal(const Spline& spline, size_t begin, size_t end, float fraction)
    {
        AZ_MATH_ASSERT(end > 0, "Invalid end index passed");
        const size_t segmentCount = spline.GetSegmentCount();
        if (fraction <= 0.0f || segmentCount == 0)
        {
            return SplineAddress(begin);
        }

        if (fraction >= 1.0f)
        {
            return SplineAddress(end - 1, 1.0f);
        }

        return GetAddressByDistanceInternal(spline, begin, end, spline.GetSplineLength() * fraction);
    }

    /**
     * Util function to calculate total spline length.
     * @param begin Vertex to start iteration on.
     * @param end Vertex to end iteration on.
     * @param spline Current spline the query is being conducted on.
     * @return Spline length.
     */
    static float GetSplineLengthInternal(const Spline& spline, size_t begin, size_t end)
    {
        float splineLength = 0.0f;
        for (size_t i = begin; i < end; ++i)
        {
            splineLength += spline.GetSegmentLength(i);
        }

        return splineLength;
    }

    /**
     * Util function to calculate the spline length from the beginning to the specified point.
     * @param begin Vertex to start iteration on.
     * @param splineAddress Address of the point to get the distance to.
     * @param spline Current spline the query is being conducted on.
     * @return Distance to the point specified along the spline
     */
    static float GetSplineLengthAtAddressInternal(const Spline& spline, size_t begin, const SplineAddress& splineAddress)
    {
        float splineLength = GetSplineLengthInternal(spline, begin, splineAddress.m_segmentIndex);
        splineLength += spline.GetSegmentLength(splineAddress.m_segmentIndex) * splineAddress.m_segmentFraction;
        return splineLength;
    }

    /**
     * Util function to ensure range wraps correctly when stepping backwards.
     */
    static size_t GetPrevIndexWrapped(size_t index, size_t backwardStep, size_t size)
    {
        AZ_MATH_ASSERT(backwardStep < size, "Do not attempt to step back by the size or more");
        return (index - backwardStep + size) % size;
    }

    /**
     * Util function to get segment count of spline based on vertex count and open/closed state.
     */
    static size_t GetSegmentCountInternal(const Spline& spline)
    {
        size_t vertexCount = spline.GetVertexCount();
        size_t additionalSegmentCount = vertexCount >= 2 ? (spline.IsClosed() ? 1 : 0) : 0;
        return vertexCount > 1 ? vertexCount - 1 + additionalSegmentCount : 0;
    }

    /**
     * Util function to access the index of the last vertex in the spline.
     */
    static size_t GetLastVertexDefault(bool closed, size_t vertexCount)
    {
        return closed ? vertexCount : vertexCount - 1;
    }

    /**
     * Assign granularity if it is valid - only assign granularity if we know the
     * granularity value is large enough to matter (if it is 1, the spline will have
     * been linear - should keep default granularity)
     */
    static void TryAssignGranularity(u16& granularityOut, u16 granularity)
    {
        if (granularity > 1)
        {
            granularityOut = granularity;
        }
    }

    void Spline::Reflect(SerializeContext& context)
    {
        context.Class<Spline>()
            ->Field("Vertices", &Spline::m_vertexContainer)
            ->Field("Closed", &Spline::m_closed)
            ;

        if (EditContext* editContext = context.GetEditContext())
        {
            editContext->Class<Spline>("Spline", "Spline Data")
                ->ClassElement(Edit::ClassElements::EditorData, "")
                    //->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly) // disabled - prevents ChangeNotify attribute firing correctly
                    ->Attribute(Edit::Attributes::AutoExpand, true)
                    ->Attribute(Edit::Attributes::ContainerCanBeModified, false)
                ->DataElement(Edit::UIHandlers::Default, &Spline::m_vertexContainer, "Vertices", "Data representing the spline, in the entity's local coordinate space")
                    ->Attribute(Edit::Attributes::AutoExpand, true)
                ->DataElement(Edit::UIHandlers::CheckBox, &Spline::m_closed, "Closed", "Determine whether a spline is self closing (looping) or not")
                    ->Attribute(Edit::Attributes::ChangeNotify, &Spline::OnOpenCloseChanged)
                ;
        }
    }

    Spline::Spline()
        : m_vertexContainer(
            [this](size_t index) { OnVertexAdded(index); },
            [this](size_t index) { OnVertexRemoved(index); },
            [this](size_t) { OnSplineChanged(); },
            [this]() { OnVerticesSet(); },
            [this]() { OnVerticesCleared(); })
    {
    }

    Spline::Spline(const Spline& spline)
        : m_vertexContainer(
            [this](size_t index) { OnVertexAdded(index); },
            [this](size_t index) { OnVertexRemoved(index); },
            [this](size_t) { OnSplineChanged(); },
            [this]() { OnVerticesSet(); },
            [this]() { OnVerticesCleared(); })
        , m_closed(spline.m_closed)
        , m_onOpenCloseCallback(nullptr) // note: do not copy the callback, just spline data
    {
        m_vertexContainer.SetVertices(spline.GetVertices());
    }

    void Spline::SetCallbacks(
        const VoidFunction& onChangeElement, const VoidFunction& onChangeContainer,
        const BoolFunction& onOpenClose)
    {
        m_vertexContainer.SetCallbacks(
            [this, onChangeContainer](const size_t index) { OnVertexAdded(index); if (onChangeContainer) { onChangeContainer(); } },
            [this, onChangeContainer](const size_t index) { OnVertexRemoved(index); if (onChangeContainer) { onChangeContainer(); } },
            [this, onChangeElement](const size_t) { OnSplineChanged(); if (onChangeElement) { onChangeElement(); } },
            [this, onChangeContainer]() { OnVerticesSet(); if (onChangeContainer) { onChangeContainer(); } },
            [this, onChangeContainer]() { OnVerticesCleared(); if (onChangeContainer) { onChangeContainer(); } });

        m_onOpenCloseCallback = onOpenClose;
    }

    void Spline::SetCallbacks(
        const IndexFunction& onAddVertex, const IndexFunction& onRemoveVertex,
        const IndexFunction& onUpdateVertex, const VoidFunction& onSetVertices,
        const VoidFunction& onClearVertices, const BoolFunction& onOpenClose)
    {
        m_vertexContainer.SetCallbacks(
            [this, onAddVertex](const size_t index) { OnVertexAdded(index); if (onAddVertex) { onAddVertex(index); } },
            [this, onRemoveVertex](const size_t index) { OnVertexRemoved(index); if (onRemoveVertex) { onRemoveVertex(index); } },
            [this, onUpdateVertex](const size_t index) { OnSplineChanged(); if (onUpdateVertex) { onUpdateVertex(index); } },
            [this, onSetVertices]() { OnVerticesSet(); if (onSetVertices) { onSetVertices(); } },
            [this, onClearVertices]() { OnVerticesCleared(); if (onClearVertices) { onClearVertices(); } });

        m_onOpenCloseCallback = onOpenClose;
    }

    void Spline::SetClosed(const bool closed)
    {
        if (closed != m_closed)
        {
            m_closed = closed;

            if (m_onOpenCloseCallback)
            {
                m_onOpenCloseCallback(closed);
            }
        }
    }

    void Spline::OnVertexAdded(size_t /*index*/)
    {
    }

    void Spline::OnVerticesSet()
    {
    }

    void Spline::OnVertexRemoved(size_t /*index*/)
    {
    }

    void Spline::OnVerticesCleared()
    {
    }

    void Spline::OnSplineChanged()
    {
    }

    void Spline::OnOpenCloseChanged()
    {
        if (m_onOpenCloseCallback)
        {
            m_onOpenCloseCallback(m_closed);
        }

        OnSplineChanged();
    }

    RaySplineQueryResult LinearSpline::GetNearestAddressRay(const Vector3& localRaySrc, const Vector3& localRayDir) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 1
            ? GetNearestAddressInternal<RayQuery, RayIntermediateQueryResult, RaySplineQueryResult, RayMinResult>(
                *this, 0, GetLastVertexDefault(m_closed, vertexCount), GetSegmentGranularity(), RayQuery(localRaySrc, localRayDir))
            : RaySplineQueryResult(SplineAddress(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    }

    PositionSplineQueryResult LinearSpline::GetNearestAddressPosition(const Vector3& localPos) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 1
            ? GetNearestAddressInternal<PosQuery, PosIntermediateQueryResult, PositionSplineQueryResult, PosMinResult>(
                *this, 0, GetLastVertexDefault(m_closed, vertexCount), GetSegmentGranularity(), PosQuery(localPos))
            : PositionSplineQueryResult(SplineAddress(), std::numeric_limits<float>::max());
    }

    SplineAddress LinearSpline::GetAddressByDistance(float distance) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 1
            ? GetAddressByDistanceInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount), distance)
            : SplineAddress();
    }

    SplineAddress LinearSpline::GetAddressByFraction(float fraction) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 1
            ? GetAddressByFractionInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount), fraction)
            : SplineAddress();
    }

    Vector3 LinearSpline::GetPosition(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateZero();
        }

        const size_t index = splineAddress.m_segmentIndex;
        const bool outOfBoundsIndex = index >= segmentCount;
        if (!m_closed && outOfBoundsIndex)
        {
            Vector3 lastVertex;
            if (m_vertexContainer.GetLastVertex(lastVertex))
            {
                return lastVertex;
            }

            return Vector3::CreateZero();
        }

        // ensure the index is clamped within a safe range (cannot go past the last vertex)
        const size_t safeIndex = GetMin(index, segmentCount - 1);
        const size_t nextIndex = (safeIndex + 1) % GetVertexCount();
        // if the index was out of bounds, ensure the segment fraction
        // is 1 to return the very end of the spline loop
        const float segmentFraction = outOfBoundsIndex ? 1.0f : splineAddress.m_segmentFraction;
        return GetVertex(safeIndex).Lerp(GetVertex(nextIndex), segmentFraction);
    }

    Vector3 LinearSpline::GetNormal(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateAxisX();
        }

        const size_t index = GetMin(static_cast<size_t>(splineAddress.m_segmentIndex), segmentCount - 1);
        return GetTangent(SplineAddress(index)).ZAxisCross().GetNormalizedSafe(s_splineEpsilon);
    }

    Vector3 LinearSpline::GetTangent(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateAxisX();
        }

        const size_t index = GetMin(static_cast<size_t>(splineAddress.m_segmentIndex), segmentCount - 1);
        const size_t nextIndex = (index + 1) % GetVertexCount();
        return (GetVertex(nextIndex) - GetVertex(index)).GetNormalizedSafe(s_splineEpsilon);
    }

    float LinearSpline::GetSplineLength() const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 1
               ? GetSplineLengthInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount))
               : 0.0f;
    }

    float LinearSpline::GetSegmentLength(size_t index) const
    {
        if (index >= GetSegmentCount())
        {
            return 0.0f;
        }

        const size_t nextIndex = (index + 1) % GetVertexCount();
        return (GetVertex(nextIndex) - GetVertex(index)).GetLength();
    }

    float LinearSpline::GetLength(const SplineAddress& splineAddress) const
    {
        return GetVertexCount() > 1
            ? GetSplineLengthAtAddressInternal(*this, 0, splineAddress)
            : 0.0f;
    }

    size_t LinearSpline::GetSegmentCount() const
    {
        return GetSegmentCountInternal(*this);
    }

    void LinearSpline::GetAabb(Aabb& aabb, const Transform& transform /*= Transform::CreateIdentity()*/) const
    {
        // For lines, the AABB of the vertices is sufficient
        aabb.SetNull();
        for (const auto& vertex : m_vertexContainer.GetVertices())
        {
            aabb.AddPoint(transform.TransformPoint(vertex));
        }
    }

    LinearSpline& LinearSpline::operator=(const Spline& spline)
    {
        Spline::operator=(spline);
        return *this;
    }

    void LinearSpline::Reflect(SerializeContext& context)
    {
        context.Class<LinearSpline, Spline>();

        if (EditContext* editContext = context.GetEditContext())
        {
            editContext->Class<LinearSpline>("Linear Spline", "Spline data")
                ->ClassElement(Edit::ClassElements::EditorData, "")
                ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(Edit::Attributes::AutoExpand, true)
                ->Attribute(Edit::Attributes::ContainerCanBeModified, false);
        }
    }

    BezierSpline::BezierSpline(const Spline& spline)
        : Spline(spline)
    {
        const size_t vertexCount = spline.GetVertexCount();

        BezierData bezierData;
        m_bezierData.reserve(vertexCount);
        for (size_t i = 0; i < vertexCount; ++i)
        {
            bezierData.m_forward = GetVertex(i);
            bezierData.m_back = bezierData.m_forward;
            m_bezierData.push_back(bezierData);
        }

        TryAssignGranularity(m_granularity, spline.GetSegmentGranularity());

        const size_t iterations = 2;
        CalculateBezierAngles(0, 0, iterations);
    }

    RaySplineQueryResult BezierSpline::GetNearestAddressRay(const Vector3& localRaySrc, const Vector3& localRayDir) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 1
            ? GetNearestAddressInternal<RayQuery, RayIntermediateQueryResult, RaySplineQueryResult, RayMinResult>(
                *this, 0, GetLastVertexDefault(m_closed, vertexCount), GetSegmentGranularity(), RayQuery(localRaySrc, localRayDir))
            : RaySplineQueryResult(SplineAddress(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    }

    PositionSplineQueryResult BezierSpline::GetNearestAddressPosition(const Vector3& localPos) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 1
            ? GetNearestAddressInternal<PosQuery, PosIntermediateQueryResult, PositionSplineQueryResult, PosMinResult>(
                *this, 0, GetLastVertexDefault(m_closed, vertexCount), GetSegmentGranularity(), PosQuery(localPos))
            : PositionSplineQueryResult(SplineAddress(), std::numeric_limits<float>::max());
    }

    SplineAddress BezierSpline::GetAddressByDistance(float distance) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 1
            ? GetAddressByDistanceInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount), distance)
            : SplineAddress();
    }

    SplineAddress BezierSpline::GetAddressByFraction(float fraction) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 1
            ? GetAddressByFractionInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount), fraction)
            : SplineAddress();
    }

    Vector3 BezierSpline::GetPosition(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateZero();
        }

        const size_t index = splineAddress.m_segmentIndex;
        const bool outOfBoundsIndex = index >= segmentCount;
        if (!m_closed && outOfBoundsIndex)
        {
            Vector3 lastVertex;
            if (m_vertexContainer.GetLastVertex(lastVertex))
            {
                return lastVertex;
            }

            return Vector3::CreateZero();
        }

        // ensure the index is clamped within a safe range (cannot go past the last vertex)
        const size_t safeIndex = GetMin(index, segmentCount - 1);
        const size_t nextIndex = (safeIndex + 1) % GetVertexCount();

        // if the index was out of bounds, ensure the segment fraction
        // is 1 to return the very end of the spline loop
        const float t = outOfBoundsIndex ? 1.0f : splineAddress.m_segmentFraction;
        const float invt = 1.0f - t;
        const float invtSq = invt * invt;
        const float tSq = t * t;

        const Vector3& p0 = GetVertex(safeIndex);
        const Vector3& p1 = m_bezierData[safeIndex].m_forward;
        const Vector3& p2 = m_bezierData[nextIndex].m_back;
        const Vector3& p3 = GetVertex(nextIndex);

        // B(t) from https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Cubic_B.C3.A9zier_curves
        return (p0 * invtSq * invt) +
               (p1 * 3.0f * t * invtSq) +
               (p2 * 3.0f * tSq * invt) +
               (p3 * tSq * t);
    }

    Vector3 BezierSpline::GetNormal(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateAxisX();
        }

        size_t index = splineAddress.m_segmentIndex;
        float t = splineAddress.m_segmentFraction;

        if (index >= segmentCount)
        {
            t = 1.0f;
            index = segmentCount - 1;
        }

        Vector3 tangent = GetTangent(SplineAddress(index, t));
        if (tangent.IsZero(s_splineEpsilon))
        {
            return Vector3::CreateAxisX();
        }

        const size_t nextIndex = (index + 1) % GetVertexCount();

        const float an1 = m_bezierData[index].m_angle;
        const float an2 = m_bezierData[nextIndex].m_angle;

        Vector3 normal;
        if (!IsClose(an1, 0.0f, s_splineEpsilon) || !IsClose(an2, 0.0f, s_splineEpsilon))
        {
            float af = t * 2.0f - 1.0f;
            float ed = 1.0f;
            if (af < 0.0f)
            {
                ed = -1.0f;
            }

            af = ed - af;
            af = af * af * af;
            af = ed - af;
            af = (af + 1.0f) * 0.5f;

            float angle = DegToRad((1.0f - af) * an1 + af * an2);

            tangent.Normalize();
            normal = tangent.ZAxisCross();
            Quaternion quat = Quaternion::CreateFromAxisAngle(tangent, angle);
            normal = quat.TransformVector(normal);
        }
        else
        {
            normal = tangent.ZAxisCross();
        }

        return normal.GetNormalizedSafe(s_splineEpsilon);
    }

    Vector3 BezierSpline::GetTangent(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateAxisX();
        }

        float t = splineAddress.m_segmentFraction;
        size_t index = splineAddress.m_segmentIndex;

        if (index >= segmentCount)
        {
            index = segmentCount - 1;
            t = 1.0f;
        }

        const size_t nextIndex = (index + 1) % GetVertexCount();

        // B'(t) from https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Cubic_B.C3.A9zier_curves
        const float invt = 1.0f - t;
        const float invtSq = invt * invt;
        const float tSq = t * t;

        const Vector3& p0 = GetVertex(index);
        const Vector3& p1 = m_bezierData[index].m_forward;
        const Vector3& p2 = m_bezierData[nextIndex].m_back;
        const Vector3& p3 = GetVertex(nextIndex);

        return (((p1 - p0) * 3.0f * invtSq) +
                ((p2 - p1) * 6.0f * invt * t) +
                ((p3 - p2) * 3.0f * tSq)).GetNormalizedSafe(s_splineEpsilon);
    }

    float BezierSpline::GetSplineLength() const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 1
               ? GetSplineLengthInternal(*this, 0, GetLastVertexDefault(m_closed, vertexCount))
               : 0.0f;
    }

    float BezierSpline::GetSegmentLength(size_t index) const
    {
        return CalculateSegmentLengthPiecewise(*this, index);
    }

    float BezierSpline::GetLength(const SplineAddress& splineAddress) const
    {
        return GetVertexCount() > 1
            ? GetSplineLengthAtAddressInternal(*this, 0, splineAddress)
            : 0.0f;
    }

    size_t BezierSpline::GetSegmentCount() const
    {
        return GetSegmentCountInternal(*this);
    }

    void BezierSpline::GetAabb(Aabb& aabb, const Transform& transform /*= Transform::CreateIdentity()*/) const
    {
        const size_t vertexCount = GetVertexCount();
        if (vertexCount > 1)
        {
            CalculateAabbPiecewise(*this, 0, GetLastVertexDefault(m_closed, vertexCount), aabb, transform);
        }
        else
        {
            aabb.SetNull();
        }
    }

    BezierSpline& BezierSpline::operator=(const BezierSpline& bezierSpline)
    {
        Spline::operator=(bezierSpline);
        if (this != &bezierSpline)
        {
            m_bezierData.clear();
            for (const BezierData& bezierData : bezierSpline.m_bezierData)
            {
                m_bezierData.push_back(bezierData);
            }
        }

        m_granularity = bezierSpline.GetSegmentGranularity();

        return *this;
    }

    BezierSpline& BezierSpline::operator=(const Spline& spline)
    {
        Spline::operator=(spline);
        if (this != &spline)
        {
            BezierData bezierData;
            m_bezierData.clear();

            size_t vertexCount = spline.GetVertexCount();
            m_bezierData.reserve(vertexCount);

            for (size_t i = 0; i < vertexCount; ++i)
            {
                bezierData.m_forward = GetVertex(i);
                bezierData.m_back = bezierData.m_forward;
                m_bezierData.push_back(bezierData);
            }

            TryAssignGranularity(m_granularity, spline.GetSegmentGranularity());

            const size_t iterations = 2;
            CalculateBezierAngles(0, 0, iterations);
        }

        return *this;
    }

    void BezierSpline::OnVertexAdded(const size_t index)
    {
        Spline::OnVertexAdded(index);
        AddBezierDataForIndex(index);
        OnSplineChanged();
    }

    void BezierSpline::OnVerticesSet()
    {
        Spline::OnVerticesSet();
        m_bezierData.clear();

        const size_t vertexCount = GetVertexCount();
        m_bezierData.reserve(vertexCount);

        for (size_t index = 0; index < vertexCount; ++index)
        {
            AddBezierDataForIndex(index);
        }

        const size_t iterations = 2;
        CalculateBezierAngles(0, 0, iterations);
    }

    void BezierSpline::OnVertexRemoved(size_t index)
    {
        Spline::OnVertexRemoved(index);
        m_bezierData.erase(m_bezierData.data() + index);
        OnSplineChanged();
    }

    void BezierSpline::OnVerticesCleared()
    {
        Spline::OnVerticesCleared();
        m_bezierData.clear();
    }

    void BezierSpline::OnSplineChanged()
    {
        const size_t startIndex = 1;
        const size_t range = 1;
        const size_t iterations = 2;

        CalculateBezierAngles(startIndex, range, iterations);
    }

    void BezierSpline::BezierAnglesCorrectionRange(const size_t index, const size_t range)
    {
        const size_t vertexCount = GetVertexCount();
        AZ_MATH_ASSERT(range < vertexCount, "Range should be less than vertexCount to prevent wrap around");

        if (m_closed)
        {
            const size_t fullRange = range * 2;
            size_t wrappingIndex = GetPrevIndexWrapped(index, range, vertexCount);
            for (size_t i = 0; i <= fullRange; ++i)
            {
                BezierAnglesCorrection(wrappingIndex);
                wrappingIndex = (wrappingIndex + 1) % vertexCount;
            }
        }
        else
        {
            size_t min = GetMin(index + range, vertexCount - 1);
            for (size_t i = index - range; i <= min; ++i)
            {
                BezierAnglesCorrection(i);
            }
        }
    }

    void BezierSpline::BezierAnglesCorrection(size_t index)
    {
        const size_t vertexCount = GetVertexCount();
        if (vertexCount == 0)
        {
            return;
        }

        const size_t lastVertex = vertexCount - 1;
        if (index > lastVertex)
        {
            return;
        }

        const AZStd::vector<Vector3>& vertices = m_vertexContainer.GetVertices();
        const Vector3& currentVertex = vertices[index];

        Vector3 currentVertexLeftControl = m_bezierData[index].m_back;
        Vector3 currentVertexRightControl = m_bezierData[index].m_forward;

        if (index == 0 && !m_closed)
        {
            currentVertexLeftControl = currentVertex;
            const Vector3& nextVertex = vertices[1];

            if (lastVertex == 1)
            {
                currentVertexRightControl = currentVertex + (nextVertex - currentVertex) / 3.0f;
            }
            else if (lastVertex > 0)
            {
                const Vector3& nextVertexLeftControl = m_bezierData[1].m_back;

                const float vertexToNextVertexLeftControlDistance = (nextVertexLeftControl - currentVertex).GetLength();
                const float vertexToNextVertexDistance = (nextVertex - currentVertex).GetLength();

                currentVertexRightControl = currentVertex +
                    (nextVertexLeftControl - currentVertex) /
                    (vertexToNextVertexLeftControlDistance / vertexToNextVertexDistance * 3.0f);
            }
        }
        else if (index == lastVertex && !m_closed)
        {
            currentVertexRightControl = currentVertex;
            if (index > 0)
            {
                const Vector3& previousVertex = vertices[index - 1];
                const Vector3& previousVertexRightControl = m_bezierData[index - 1].m_forward;

                const float vertexToPreviousVertexRightControlDistance = (previousVertexRightControl - currentVertex).GetLength();
                const float vertexToPreviousVertexDistance = (previousVertex - currentVertex).GetLength();

                if (!IsClose(vertexToPreviousVertexRightControlDistance, 0.0f, Constants::FloatEpsilon) &&
                    !IsClose(vertexToPreviousVertexDistance, 0.0f, Constants::FloatEpsilon))
                {
                    currentVertexLeftControl = currentVertex +
                        (previousVertexRightControl - currentVertex) /
                        (vertexToPreviousVertexRightControlDistance / vertexToPreviousVertexDistance * 3.0f);
                }
                else
                {
                    currentVertexLeftControl = currentVertex;
                }
            }
        }
        else
        {
            // If spline is closed, ensure indices wrap correctly
            const size_t prevIndex = GetPrevIndexWrapped(index, 1, vertexCount);
            const size_t nextIndex = (index + 1) % vertexCount;

            const Vector3& previousVertex = vertices[prevIndex];
            const Vector3& nextVertex = vertices[nextIndex];

            const float nextVertexToPreviousVertexDistance = (nextVertex - previousVertex).GetLength();
            const float previousVertexToCurrentVertexDistance = (previousVertex - currentVertex).GetLength();
            const float nextVertexToCurrentVertexDistance = (nextVertex - currentVertex).GetLength();

            if (!IsClose(nextVertexToPreviousVertexDistance, 0.0f, Constants::FloatEpsilon))
            {
                currentVertexLeftControl = currentVertex +
                    (previousVertex - nextVertex) *
                    (previousVertexToCurrentVertexDistance / nextVertexToPreviousVertexDistance / 3.0f);
                currentVertexRightControl = currentVertex +
                    (nextVertex - previousVertex) *
                    (nextVertexToCurrentVertexDistance / nextVertexToPreviousVertexDistance / 3.0f);
            }
            else
            {
                currentVertexLeftControl = currentVertex;
                currentVertexRightControl = currentVertex;
            }
        }

        m_bezierData[index].m_back = currentVertexLeftControl;
        m_bezierData[index].m_forward = currentVertexRightControl;
    }

    void BezierSpline::CalculateBezierAngles(size_t startIndex, size_t range, size_t iterations)
    {
        const size_t vertexCount = GetVertexCount();
        for (size_t i = 0; i < iterations; ++i)
        {
            for (size_t v = startIndex; v < vertexCount; ++v)
            {
                BezierAnglesCorrectionRange(v, range);
            }
        }
    }

    void BezierSpline::AddBezierDataForIndex(size_t index)
    {
        BezierData bezierData;
        bezierData.m_forward = GetVertex(index);
        bezierData.m_back = GetVertex(index);
        m_bezierData.insert(m_bezierData.data() + index, bezierData);
    }

    void BezierSpline::BezierData::Reflect(SerializeContext& context)
    {
        context.Class<BezierSpline::BezierData>()
            ->Field("Forward", &BezierSpline::BezierData::m_forward)
            ->Field("Back", &BezierSpline::BezierData::m_back)
            ->Field("Angle", &BezierSpline::BezierData::m_angle);
    }

    void BezierSpline::Reflect(SerializeContext& context)
    {
        BezierData::Reflect(context);

        context.Class<BezierSpline, Spline>()
            ->Field("Bezier Data", &BezierSpline::m_bezierData)
            ->Field("Granularity", &BezierSpline::m_granularity);

        if (EditContext* editContext = context.GetEditContext())
        {
            editContext->Class<BezierSpline>("Bezier Spline", "Spline data")
                ->ClassElement(Edit::ClassElements::EditorData, "")
                    //->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly) // disabled - prevents ChangeNotify attribute firing correctly
                    ->Attribute(Edit::Attributes::AutoExpand, true)
                    ->Attribute(Edit::Attributes::ContainerCanBeModified, false)
                ->DataElement(Edit::UIHandlers::Default, &BezierSpline::m_bezierData, "Bezier Data", "Data defining the bezier curve")
                    ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::Hide)
                ->DataElement(Edit::UIHandlers::Slider, &BezierSpline::m_granularity, "Granularity", "Parameter specifying the granularity of each segment in the spline")
                    ->Attribute(Edit::Attributes::Min, s_minGranularity)
                    ->Attribute(Edit::Attributes::Max, s_maxGranularity)
                    ;
        }
    }

    CatmullRomSpline::CatmullRomSpline(const Spline& spline)
        : Spline(spline)
    {
        TryAssignGranularity(m_granularity, spline.GetSegmentGranularity());
    }

    static size_t GetLastVertexCatmullRom(bool closed, size_t vertexCount)
    {
        return vertexCount - (closed ? 0 : 2);
    }

    RaySplineQueryResult CatmullRomSpline::GetNearestAddressRay(const Vector3& localRaySrc, const Vector3& localRayDir) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 3
            ? GetNearestAddressInternal<RayQuery, RayIntermediateQueryResult, RaySplineQueryResult, RayMinResult>(
                *this, m_closed ? 0 : 1, GetLastVertexCatmullRom(m_closed, vertexCount), GetSegmentGranularity(), RayQuery(localRaySrc, localRayDir))
            : RaySplineQueryResult(SplineAddress(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    }

    PositionSplineQueryResult CatmullRomSpline::GetNearestAddressPosition(const Vector3& localPos) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 3
            ? GetNearestAddressInternal<PosQuery, PosIntermediateQueryResult, PositionSplineQueryResult, PosMinResult>(
                *this, m_closed ? 0 : 1, GetLastVertexCatmullRom(m_closed, vertexCount), GetSegmentGranularity(), PosQuery(localPos))
            : PositionSplineQueryResult(SplineAddress(), std::numeric_limits<float>::max());
    }

    SplineAddress CatmullRomSpline::GetAddressByDistance(float distance) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 3
            ? GetAddressByDistanceInternal(*this, m_closed ? 0 : 1, GetLastVertexCatmullRom(m_closed, vertexCount), distance)
            : SplineAddress();
    }

    SplineAddress CatmullRomSpline::GetAddressByFraction(float fraction) const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 3
            ? GetAddressByFractionInternal(*this, m_closed ? 0 : 1, GetLastVertexCatmullRom(m_closed, vertexCount), fraction)
            : SplineAddress();
    }

    Vector3 CatmullRomSpline::GetPosition(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateZero();
        }

        const AZStd::vector<Vector3>& vertices = m_vertexContainer.GetVertices();
        const size_t index = splineAddress.m_segmentIndex;
        const size_t vertexCount = GetVertexCount();
        if (!m_closed)
        {
            if (index < 1)
            {
                return vertices[1];
            }

            const size_t lastVertex = vertexCount - 2;
            if (index >= lastVertex)
            {
                return vertices[lastVertex];
            }
        }

        // ensure the index is clamped within a safe range (cannot go past the last vertex)
        const size_t safeIndex = GetMin(index, vertexCount - 1);
        const size_t prevIndex = GetPrevIndexWrapped(safeIndex, 1, vertexCount);
        const size_t nextIndex = (safeIndex + 1) % vertexCount;
        const size_t nextNextIndex = (safeIndex + 2) % vertexCount;

        const Vector3& p0 = vertices[prevIndex];
        const Vector3& p1 = vertices[safeIndex];
        const Vector3& p2 = vertices[nextIndex];
        const Vector3& p3 = vertices[nextNextIndex];

        // float t0 = 0.0f; // for reference
        const float t1 = std::pow(p1.GetDistance(p0), m_knotParameterization) /* + t0 */;
        const float t2 = std::pow(p2.GetDistance(p1), m_knotParameterization) + t1;
        const float t3 = std::pow(p3.GetDistance(p2), m_knotParameterization) + t2;

        // if the index is out of bounds, ensure the segment fraction
        // is 1 to return the very end of the spline loop
        const float segmentFraction = index >= vertexCount ? 1.0f : splineAddress.m_segmentFraction;
        // Transform fraction from [0-1] to [t1,t2]
        const float t = Lerp(t1, t2, segmentFraction);

        // Barry and Goldman's pyramidal formulation
        const Vector3 a1 = (t1 - t) / t1 * p0 + t / t1 * p1;
        const Vector3 a2 = (t2 - t) / (t2 - t1) * p1 + (t - t1) / (t2 - t1) * p2;
        const Vector3 a3 = (t3 - t) / (t3 - t2) * p2 + (t - t2) / (t3 - t2) * p3;

        const Vector3 b1 = (t2 - t) / t2 * a1 + t / t2 * a2;
        const Vector3 b2 = (t3 - t) / (t3 - t1) * a2 + (t - t1) / (t3 - t1) * a3;

        const Vector3 c = (t2 - t) / (t2 - t1) * b1 + (t - t1) / (t2 - t1) * b2;

        return c;
    }

    Vector3 CatmullRomSpline::GetNormal(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateAxisX();
        }

        return GetTangent(splineAddress).ZAxisCross().GetNormalizedSafe(s_splineEpsilon);
    }

    Vector3 CatmullRomSpline::GetTangent(const SplineAddress& splineAddress) const
    {
        const size_t segmentCount = GetSegmentCount();
        if (segmentCount == 0)
        {
            return Vector3::CreateAxisX();
        }

        const size_t vertexCount = GetVertexCount();
        size_t index = splineAddress.m_segmentIndex;
        float fraction = index >= vertexCount ? 1.0f : splineAddress.m_segmentFraction;

        if (!m_closed)
        {
            if (index < 1)
            {
                index = 1;
                fraction = 0.0f;
            }
            else if (index >= vertexCount - 2)
            {
                index = vertexCount - 3;
                fraction = 1.0f;
            }
        }

        // ensure the index is clamped within a safe range (cannot go past the last vertex)
        const size_t safeIndex = GetMin(index, vertexCount - 1);
        const size_t prevIndex = GetPrevIndexWrapped(safeIndex, 1, vertexCount);
        const size_t nextIndex = (safeIndex + 1) % vertexCount;
        const size_t nextNextIndex = (safeIndex + 2) % vertexCount;

        const AZStd::vector<Vector3>& vertices = m_vertexContainer.GetVertices();
        const Vector3& p0 = vertices[prevIndex];
        const Vector3& p1 = vertices[safeIndex];
        const Vector3& p2 = vertices[nextIndex];
        const Vector3& p3 = vertices[nextNextIndex];

        // float t0 = 0.0f; // for reference
        const float t1 = std::pow(p1.GetDistance(p0), m_knotParameterization) /* + t0 */;
        const float t2 = std::pow(p2.GetDistance(p1), m_knotParameterization) + t1;
        const float t3 = std::pow(p3.GetDistance(p2), m_knotParameterization) + t2;

        // Transform fraction from [0-1] to [t1,t2]
        const float t = Lerp(t1, t2, fraction);

        // Barry and Goldman's pyramidal formulation
        const Vector3 a1 = (t1 - t) / t1 * p0 + t / t1 * p1;
        const Vector3 a2 = (t2 - t) / (t2 - t1) * p1 + (t - t1) / (t2 - t1) * p2;
        const Vector3 a3 = (t3 - t) / (t3 - t2) * p2 + (t - t2) / (t3 - t2) * p3;

        const Vector3 b1 = (t2 - t) / t2 * a1 + t / t2 * a2;
        const Vector3 b2 = (t3 - t) / (t3 - t1) * a2 + (t - t1) / (t3 - t1) * a3;

        // Derivative of Barry and Goldman's pyramidal formulation
        // (http://denkovacs.com/2016/02/catmull-rom-spline-derivatives/)
        const Vector3 a1p = (p1 - p0) / t1;
        const Vector3 a2p = (p2 - p1) / (t2 - t1);
        const Vector3 a3p = (p3 - p2) / (t3 - t2);

        const Vector3 b1p = ((a2 - a1) / t2) + (((t2 - t) / t2) * a1p) + ((t / t2) * a2p);
        const Vector3 b2p = ((a3 - a2) / (t3 - t1)) + (((t3 - t) / (t3 - t1)) * a2p) + (((t - t1) / (t3 - t1)) * a3p);

        const Vector3 cp = ((b2 - b1) / (t2 - t1)) + (((t2 - t) / (t2 - t1)) * b1p) + (((t - t1) / (t2 - t1)) * b2p);

        return cp.GetNormalizedSafe(s_splineEpsilon);
    }

    float CatmullRomSpline::GetSplineLength() const
    {
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 3
               ? GetSplineLengthInternal(*this, m_closed ? 0 : 1, GetLastVertexCatmullRom(m_closed, vertexCount))
               : 0.0f;
    }

    float CatmullRomSpline::GetSegmentLength(size_t index) const
    {
        return m_closed || index > 0
               ? CalculateSegmentLengthPiecewise(*this, index)
               : 0.0f;
    }

    float CatmullRomSpline::GetLength(const SplineAddress& splineAddress) const
    {
        return GetVertexCount() > 3
            ? GetSplineLengthAtAddressInternal(*this, m_closed ? 0 : 1, splineAddress)
            : 0.0f;
    }

    size_t CatmullRomSpline::GetSegmentCount() const
    {
        // Catmull-Rom spline must have at least four vertices to be valid.
        const size_t vertexCount = GetVertexCount();
        return vertexCount > 3 ? (m_closed ? vertexCount : vertexCount - 3) : 0;
    }

    // Gets the Aabb of the vertices in the spline
    void CatmullRomSpline::GetAabb(Aabb& aabb, const Transform& transform /*= Transform::CreateIdentity()*/) const
    {
        const size_t vertexCount = GetVertexCount();
        if (vertexCount > 3)
        {
            CalculateAabbPiecewise(*this, m_closed ? 0 : 1, GetLastVertexCatmullRom(m_closed, vertexCount), aabb, transform);
        }
        else
        {
            aabb.SetNull();
        }
    }

    CatmullRomSpline& CatmullRomSpline::operator=(const Spline& spline)
    {
        Spline::operator=(spline);
        TryAssignGranularity(m_granularity, spline.GetSegmentGranularity());
        return *this;
    }

    void CatmullRomSpline::Reflect(SerializeContext& context)
    {
        context.Class<CatmullRomSpline, Spline>()
            ->Field("KnotParameterization", &CatmullRomSpline::m_knotParameterization)
            ->Field("Granularity", &CatmullRomSpline::m_granularity);

        if (EditContext* editContext = context.GetEditContext())
        {
            editContext->Class<CatmullRomSpline>("Catmull Rom Spline", "Spline data")
                ->ClassElement(Edit::ClassElements::EditorData, "")
                    //->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly) // disabled - prevents ChangeNotify attribute firing correctly
                    ->Attribute(Edit::Attributes::AutoExpand, true)
                    ->Attribute(Edit::Attributes::ContainerCanBeModified, false)
                ->DataElement(Edit::UIHandlers::Slider, &CatmullRomSpline::m_knotParameterization, "Knot Parameterization", "Parameter specifying interpolation of the spline")
                    ->Attribute(Edit::Attributes::Min, 0.0f)
                    ->Attribute(Edit::Attributes::Max, 1.0f)
                ->DataElement(Edit::UIHandlers::Slider, &CatmullRomSpline::m_granularity, "Granularity", "Parameter specifying the granularity of each segment in the spline")
                    ->Attribute(Edit::Attributes::Min, s_minGranularity)
                    ->Attribute(Edit::Attributes::Max, s_maxGranularity)
                    ;
        }
    }

    AZ_CLASS_ALLOCATOR_IMPL(Spline, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(LinearSpline, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(BezierSpline, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(BezierSpline::BezierData, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(CatmullRomSpline, SystemAllocator);
}
