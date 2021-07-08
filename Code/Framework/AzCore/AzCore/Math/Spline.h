/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/VertexContainer.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/functional.h>

namespace AZ
{
    class Aabb;
    class ReflectContext;
    void SplineReflect(ReflectContext* context);

    /**
     * Interface for representing a specific position on a spline.
     */
    struct SplineAddress
    {
        AZ_TYPE_INFO(SplineAddress, "{865BA2EC-43C5-4E1F-9B6F-2D63F6DC2E70}")

        u64 m_segmentIndex = 0; ///< Which segment along the spline you are on.
        float m_segmentFraction = 0.0f; ///< Percentage along a given segment - range [0, 1].

        SplineAddress() = default;
        explicit SplineAddress(size_t segmentIndex)
            : m_segmentIndex(segmentIndex) {}
        SplineAddress(size_t segmentIndex, float segmentFraction)
            : m_segmentIndex(segmentIndex)
            , m_segmentFraction(segmentFraction) {}

        bool operator==(const SplineAddress& splineAddress) const
        {
            return m_segmentIndex == splineAddress.m_segmentIndex
                && IsClose(m_segmentFraction, splineAddress.m_segmentFraction, s_segmentFractionEpsilon);
        }

        static const float s_segmentFractionEpsilon; ///< Epsilon value for segment fraction spline address comparison.
    };

    /**
     * Result object for querying a position on a spline using a position/point.
     */
    struct PositionSplineQueryResult
    {
        AZ_TYPE_INFO(PositionSplineQueryResult, "{E35DCF28-1AC3-49E8-A0AB-2F6115348F45}")

        PositionSplineQueryResult() = default;
        explicit PositionSplineQueryResult(const SplineAddress& splineAddress, float distanceSq)
            : m_splineAddress(splineAddress), m_distanceSq(distanceSq) {}

        SplineAddress m_splineAddress; ///< The returned spline address from the query (closest point on spline to query).
        float m_distanceSq = 0.0f; ///< The shortest distance from the query to the spline.
    };

    /**
     * Result object for querying a position on a spline using a ray.
     * Includes same data as position query.
     */
    struct RaySplineQueryResult
        : public PositionSplineQueryResult
    {
        AZ_TYPE_INFO(RaySplineQueryResult, "{FE862126-C838-4999-9B7B-4AEEA5507A49}")

        RaySplineQueryResult() = default;
        explicit RaySplineQueryResult(const SplineAddress& splineAddress, float distanceSq, float rayDistance)
            : PositionSplineQueryResult(splineAddress, distanceSq)
            , m_rayDistance(rayDistance) {}

        float m_rayDistance = 0.0f; ///< The distance along the ray the point closest to the spline is.
    };

    /**
     *  Generic base spline class.
     */
    class Spline
    {
    public:
        AZ_RTTI(Spline, "{6E2D31AF-5CB0-4A50-BD68-B00E2D2FD0A4}")
        AZ_CLASS_ALLOCATOR_DECL

        Spline();
        Spline(const Spline& spline);
        virtual ~Spline() = default;

        /**
         * Return nearest address on spline from ray (local space).
         * @param localRaySrc Position of ray in local space of spline (must be transformed prior to being passed).
         * @param localRayDir Direction of ray in local space of spline (must be transformed prior to being passed).
         * @return SplineAddress closest to given ray on spline.
         */
        virtual RaySplineQueryResult GetNearestAddressRay(const Vector3& localRaySrc, const Vector3& localRayDir) const = 0;

        /**
         * Return nearest address on spline from position (local space).
         * @param localPos Position in local space of spline (must be transformed prior to being passed).
         * @return SplineAddress closest to given position on spline.
         */
        virtual PositionSplineQueryResult GetNearestAddressPosition(const Vector3& localPos) const = 0;

        /**
         * Return address at distance value - range [0, splineLength].
         * @param distance Distance along the spline.
         * @return SplineAddress corresponding to distance.
         */
        virtual SplineAddress GetAddressByDistance(float distance) const = 0;

        /**
         * Return address at fractional value - range [0, 1].
         * @param fraction fraction/proportion/percentage along the spline.
         * @return SplineAddress corresponding to fraction.
         */
        virtual SplineAddress GetAddressByFraction(float fraction) const = 0;

        /**
         * Return position at SplineAddress (local space).
         * @param splineAddress Address representing a point on the spline.
         * @return Position of given SplineAddress in local space.
         */
        virtual Vector3 GetPosition(const SplineAddress& splineAddress) const = 0;

        /**
         * Return normal at SplineAddress (local space).
         * @param splineAddress Address representing a point on the spline.
         * @return Normal of given SplineAddress in local space.
         */
        virtual Vector3 GetNormal(const SplineAddress& splineAddress) const = 0;

        /**
         * Return tangent at SplineAddress (local space).
         * @param splineAddress Address representing a point on the spline.
         * @return Tangent of given SplineAddress in local space.
         */
        virtual Vector3 GetTangent(const SplineAddress& splineAddress) const = 0;

        /**
         * Returns spline length from the beginning to the specific point.
         * @param splineAddress Address of the point to get the distance to.
         * @return Distance to the address specified along the spline
         */
        virtual float GetLength(const SplineAddress& splineAddress) const = 0;

        /**
         * Returns total length of spline.
         */
        virtual float GetSplineLength() const = 0;

        /**
         * Returns length the segment between vertices - [index, index + 1].
         */
        virtual float GetSegmentLength(size_t index) const = 0;

        /**
         * Return number of Segments along spline.
         * Explicitly returns the number of valid/real segments in the spline
         * Some splines technically have invalid segments (example: that lie in the range [0 - 1]
         * and [vertexCount - 2, vertexCount -1] - these will be ignored in the segment count calculation).
         */
        virtual size_t GetSegmentCount() const = 0;

        /**
         * Return the number of parts (lines) that make up a segment (higher granularity - smoother curve).
         */
        virtual u16 GetSegmentGranularity() const = 0;

        /**
         * Gets the Aabb of the vertices in the spline.
         * @param aabb out param of filled aabb.
         */
        virtual void GetAabb(Aabb& aabb, const Transform& transform = Transform::CreateIdentity()) const = 0;

        /**
         * Set whether the spline is closed or not - should its last vertex connect to the first
         */
        void SetClosed(bool closed);

        /**
         * Return if the spline is closed (looping) or not
         */
        bool IsClosed() const { return m_closed; }

        /**
         * Return number of vertices composing the spline.
         */
        size_t GetVertexCount() const { return m_vertexContainer.Size(); }

        /**
         * Return immutable stored vertices (local space).
         */
        const AZStd::vector<Vector3>& GetVertices() const { return m_vertexContainer.GetVertices(); }

        /**
         * Return immutable position of vertex at index (local space).
         */
        const Vector3& GetVertex(size_t index) const { return m_vertexContainer.GetVertices()[index]; }

        /**
         * Override callbacks to be used when spline changes/is modified (general).
         */
        void SetCallbacks(
            const VoidFunction& onChangeElement, const VoidFunction& onChangeContainer,
            const BoolFunction& onOpenClose);

        /**
         * Override callbacks to be used when spline changes/is modified (specific).
         * (use if you need more fine grained control over modifications to the container)
         */
        void SetCallbacks(
            const IndexFunction& onAddVertex, const IndexFunction& onRemoveVertex,
            const IndexFunction& onUpdateVertex, const VoidFunction& onSetVertices,
            const VoidFunction& onClearVertices, const BoolFunction& onOpenClose);

        VertexContainer<Vector3> m_vertexContainer; ///< Vertices representing the spline.

        static void Reflect(SerializeContext& context);

        /**
         * Notification that spline has changed
         */
        virtual void OnSplineChanged();

    protected:
        static const float s_splineEpsilon; ///< Epsilon value for splines to use to check approximate results.

        virtual void OnVertexAdded(size_t index); ///< Internal function to be overridden by derived spline spline to handle custom logic when a vertex is added.
        virtual void OnVerticesSet(); ///< Internal function to be overridden by derived spline spline to handle custom logic when all vertices are set.
        virtual void OnVertexRemoved(size_t index); ///< Internal function to be overridden by derived spline to handle custom logic when a vertex is removed.
        virtual void OnVerticesCleared(); ///< Internal function to be overridden by derived spline to handle custom logic when spline is reset (vertices are cleared).

        bool m_closed = false; ///< Is the spline closed - default is not.

    private:
        /**
         * Called when the 'Closed' property on the SplineComponent is checked/unchecked.
         */
        void OnOpenCloseChanged();
        
        BoolFunction m_onOpenCloseCallback = nullptr; ///< Callback for when the open/closed property of the Spline changes.
    };

    using SplinePtr = AZStd::shared_ptr<Spline>;
    using ConstSplinePtr = AZStd::shared_ptr<const Spline>;

    /**
     * Linear spline implementation
     */
    class LinearSpline
        : public Spline
    {
    public:
        AZ_RTTI(LinearSpline, "{DD80E118-12C9-4F69-848B-4EA5DAA2E0EC}", Spline)
        AZ_CLASS_ALLOCATOR_DECL

        LinearSpline()
            : Spline() {}
        explicit LinearSpline(const LinearSpline& spline)
            : Spline(spline) {}
        explicit LinearSpline(const Spline& spline)
            : Spline(spline) {}
        ~LinearSpline() override {}

        RaySplineQueryResult GetNearestAddressRay(const Vector3& localRaySrc, const Vector3& localRayDir) const override;
        PositionSplineQueryResult GetNearestAddressPosition(const Vector3& localPos) const override;
        SplineAddress GetAddressByDistance(float distance) const override;
        SplineAddress GetAddressByFraction(float fraction) const override;

        Vector3 GetPosition(const SplineAddress& splineAddress) const override;
        Vector3 GetNormal(const SplineAddress& splineAddress) const override;
        Vector3 GetTangent(const SplineAddress& splineAddress) const override;
        float GetLength(const SplineAddress& splineAddress) const override;

        float GetSplineLength() const override;
        float GetSegmentLength(size_t index) const override;
        size_t GetSegmentCount() const override;
        void GetAabb(Aabb& aabb, const Transform& transform = Transform::CreateIdentity()) const override;

        LinearSpline& operator=(const LinearSpline& spline) = default;
        LinearSpline& operator=(const Spline& spline);

        static void Reflect(SerializeContext& context);

    protected:
        u16 GetSegmentGranularity() const override { return 1; }
    };

    /**
     * Bezier spline implementation
     */
    class BezierSpline
        : public Spline
    {
    public:
        AZ_RTTI(BezierSpline, "{C1A48956-5CBC-4124-AB49-61FFEEE9139A}", Spline)
        AZ_CLASS_ALLOCATOR_DECL

        BezierSpline()
            : Spline() {}
        explicit BezierSpline(const BezierSpline& spline)
            : Spline(spline)
            , m_bezierData(spline.m_bezierData.begin(), spline.m_bezierData.end())
            , m_granularity(spline.m_granularity) {}
        explicit BezierSpline(const Spline& spline);
        ~BezierSpline() override {}

        RaySplineQueryResult GetNearestAddressRay(const Vector3& localRaySrc, const Vector3& localRayDir) const override;
        PositionSplineQueryResult GetNearestAddressPosition(const Vector3& localPos) const override;
        SplineAddress GetAddressByDistance(float distance) const override;
        SplineAddress GetAddressByFraction(float fraction) const override;

        Vector3 GetPosition(const SplineAddress& splineAddress) const override;
        Vector3 GetNormal(const SplineAddress& splineAddress) const override;
        Vector3 GetTangent(const SplineAddress& splineAddress) const override;
        float GetLength(const SplineAddress& splineAddress) const override;

        float GetSplineLength() const override;
        float GetSegmentLength(size_t index) const override;
        size_t GetSegmentCount() const override;
        void GetAabb(Aabb& aabb, const Transform& transform = Transform::CreateIdentity()) const override;

        BezierSpline& operator=(const BezierSpline& spline);
        BezierSpline& operator=(const Spline& spline);

        static void Reflect(SerializeContext& context);

        /**
         * Internal Bezier spline data
         */
        struct BezierData
        {
            AZ_TYPE_INFO(BezierData, "{6C34069E-AEA2-44A2-877F-BED9CE07DA6B}")
            AZ_CLASS_ALLOCATOR_DECL

            static void Reflect(SerializeContext& context);

            Vector3 m_back; ///< Control point before Vertex.
            Vector3 m_forward; ///< Control point after Vertex.
            float m_angle = 0.0f;
        };

        /**
         * Return immutable bezier data for each vertex
         */
        const AZStd::vector<BezierData>& GetBezierData() const { return m_bezierData; }

    protected:
        void OnVertexAdded(size_t index) override;
        void OnVerticesSet() override;
        void OnVertexRemoved(size_t index) override;
        void OnSplineChanged() override;
        void OnVerticesCleared() override;

        u16 GetSegmentGranularity() const override { return m_granularity; }

    private:
        /**
         * Functions to calculate bezier control points from input vertices
         */
        void BezierAnglesCorrection(size_t index);
        void BezierAnglesCorrectionRange(size_t index, size_t range);
        void CalculateBezierAngles(size_t startIndex, size_t range, size_t iterations);

        /**
         * Create bezier data for a given index
         */
        void AddBezierDataForIndex(size_t index);
        AZStd::vector<BezierData> m_bezierData; ///< Bezier data to control spline interpolation.
        u16 m_granularity = 8; ///< The granularity (tessellation) of each segment in the spline.
    };

    /**
     *  Catmull-Rom spline implementation
     */
    class CatmullRomSpline
        : public Spline
    {
    public:
        AZ_RTTI(CatmullRomSpline, "{B4AD0E71-92D8-4888-AB89-5C3B4A30759A}", Spline)
        AZ_CLASS_ALLOCATOR_DECL

        CatmullRomSpline()
            : Spline() {}
        explicit CatmullRomSpline(const CatmullRomSpline& spline)
            : Spline(spline)
            , m_knotParameterization(spline.m_knotParameterization)
            , m_granularity(spline.m_granularity) {}
        explicit CatmullRomSpline(const Spline& spline);
        ~CatmullRomSpline() override {}

        RaySplineQueryResult GetNearestAddressRay(const Vector3& localRaySrc, const Vector3& localRayDir) const override;
        PositionSplineQueryResult GetNearestAddressPosition(const Vector3& localPos) const override;
        SplineAddress GetAddressByDistance(float distance) const override;
        SplineAddress GetAddressByFraction(float fraction) const override;

        Vector3 GetPosition(const SplineAddress& splineAddress) const override;
        Vector3 GetNormal(const SplineAddress& splineAddress) const override;
        Vector3 GetTangent(const SplineAddress& splineAddress) const override;
        float GetLength(const SplineAddress& splineAddress) const override;

        float GetSplineLength() const override;
        float GetSegmentLength(size_t index) const override;
        size_t GetSegmentCount() const override;
        void GetAabb(Aabb& aabb, const Transform& transform = Transform::CreateIdentity()) const override;

        CatmullRomSpline& operator=(const CatmullRomSpline& spline) = default;
        CatmullRomSpline& operator=(const Spline& spline);

        /**
         * Sets the knot parameterization for spline interpolation, ranging from [0,1].
         * 0 = uniform; 0.5 = centriperal; 1 = chordal
         * @param knotParameterization value between [0, 1] to control spline interpolation
         */
        void SetKnotParameterization(float knotParameterization) { m_knotParameterization = GetClamp(knotParameterization, 0.0f, 1.0f); }

        static void Reflect(SerializeContext& context);

    protected:
        u16 GetSegmentGranularity() const override { return m_granularity; }

        float m_knotParameterization = 0.0f; ///< Knot parameterization value to control spline interpolation - range [0, 1]
        u16 m_granularity = 8; ///< The granularity (tessellation) of each segment in the spline.
    };

    /**
     * @brief Util function to intersect a ray in world space against a spline.
     * Use the provided transform to transform the ray into the local space of the spline.
     */
    inline RaySplineQueryResult IntersectSpline(
        const Transform& worldFromLocal, const Vector3& src, const Vector3& dir, const Spline& spline)
    {
        Transform worldFromLocalNormalized = worldFromLocal;
        const float scale = worldFromLocalNormalized.ExtractUniformScale();
        const Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverse();

        const Vector3 localRayOrigin = localFromWorldNormalized.TransformPoint(src) / scale;
        const Vector3 localRayDirection = localFromWorldNormalized.TransformVector(dir);
        return spline.GetNearestAddressRay(localRayOrigin, localRayDirection);
    }
}
