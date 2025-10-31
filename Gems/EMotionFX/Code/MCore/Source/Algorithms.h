/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include "StandardHeaders.h"
#include "FastMath.h"
#include <MCore/Source/CompressedQuaternion.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class Matrix4x4;
    class Vector2;
    class Vector3;
}

namespace MCore
{
    /**
     * Project a 3D point in world space to 2D screen coordinates.
     * @param point The point in world space we want to map to the screen.
     * @param viewProjMatrix A precalculated version of viewMatrix * projectionMatrix used by the camera.
     * @param screenWidth The width of the screen in pixels.
     * @param screenHeight The height of the screen in pixels.
     * @return A three component vector containing the resulting 2D screen coordinates as well as the distance of the point to the camera.
     *         x component: The horizontal screen coordinate. Zero means the coordinate is on the left border while a negative value means the
     *                      point even further to the left and not visible by the camera at all, a bigger value than screenWidth goes analogue for the right border.
     *         y component: The vertical screen coordinate. Zero means the coordinate is on the top border while a negative value means the
     *                      point even further to the top and not visible by the camera at all, a bigger value than screenHeight goes analogue for the bottom border.
     *         z component: The distance of the point to the camera. A negative value means the point is behind the camera and not visible.
     */
    AZ::Vector3 MCORE_API Project(const AZ::Vector3& point, const AZ::Matrix4x4& viewProjMatrix, uint32 screenWidth, uint32 screenHeight);

    /**
     * Unproject screen coordinates to a 3D point in world space.
     * @param screenX The mouse position x value or another horizontal screen coordinate in range [0, screenWidth].
     * @param screenY The mouse position y value or another vertical screen coordinate in range [0, screenHeight].
     * @param screenWidth The width of the screen in pixels.
     * @param screenHeight The height of the screen in pixels.
     * @param depth .
     * @param viewMatrix The view matrix of the camera.
     * @param projectionMatrix The projection matrix of the camera.
     * @return The unprojected point in world space.
     */
    AZ::Vector3 MCORE_API UnprojectToEyeSpace(float screenX, float screenY, const AZ::Matrix4x4& invProjMat, float windowWidth, float windowHeight, float depth);
    AZ::Vector3 MCORE_API Unproject(float screenX, float screenY, float screenWidth, float screenHeight, float depth, const AZ::Matrix4x4& invProjMat, const AZ::Matrix4x4& invViewMat);
    AZ::Vector3 MCORE_API UnprojectOrtho(float screenX, float screenY, float screenWidth, float screenHeight, float depth, const AZ::Matrix4x4& projectionMatrix, const AZ::Matrix4x4& viewMatrix);

    //
    AZ::Vector2 MCORE_API OrthogonalProject(const AZ::Vector3& pos);
    AZ::Vector3 MCORE_API OrthogonalUnproject(const AZ::Vector2& uv);
    AZ::Vector2 MCORE_API StereographicProject(const AZ::Vector3& pos);
    AZ::Vector3 MCORE_API StereographicUnproject(const AZ::Vector2& uv);

    //
    bool MCORE_API PointInPoly(AZ::Vector2* verts, size_t numVerts, const AZ::Vector2& point);
    float MCORE_API DistanceToEdge(const AZ::Vector2& edgePointA, const AZ::Vector2& edgePointB, const AZ::Vector2& testPoint);
    AZ::Vector2 MCORE_API ClosestPointToPoly(const AZ::Vector2* polyPoints, size_t numPoints, const AZ::Vector2& testPoint);

    // @param data The original data that we want to apply moving-average smooth algorithm on
    // @param sampleNum The sample number we will take from each data point.
    // When sampleNum = 1, we are taking 1 sample before the data point, and 1 sample after the data point, 3 samples point in total (include the original data point).
    // And calculate the average.
    // When sampleNum = 2, we are taking 5 samples point in total and take the average (2 before, 1 original and 2 after).
    void MCORE_API MovingAverageSmooth(AZStd::vector<AZ::Vector3>& data, size_t sampleNum);
    void MCORE_API MovingAverageSmooth(AZStd::vector<MCore::Compressed16BitQuaternion>& data, size_t sampleNum);

    /**
     * Calculate the cube root, which basically is pow(x, 1/3).
     * This also allows negative and zero values.
     * @param x The number.
     * @result The cube root.
     */
    MCORE_INLINE float CubeRoot(float x);

    /**
     * Sample an ease-in/out curve.
     * The curve is split into three sections: an ease in part, a linear constant velocity mid section, and an ease out part.
     * The ease in and out parts are parts of a sine wave.
     * @param t The normalized time value, between 0 and 1.
     * @param k1 The normalized time value where the ease-in section stops.
     * @param k2 The normalized time value where the ease-out section starts.
     * @result The interpolation fraction value. Use "startValue + SampleEaseInOutCurve(...) * (endValue-startValue)" to interpolate, or use EaseInOutInterpolate(...).
     * @see EaseInOutInterpolate.
     */
    MCORE_INLINE float SampleEaseInOutCurve(float t, float k1 = 0.5f, float k2 = 0.5f);

    /**
     * Sample an ease-in/out curve, while controlling the smoothness/linearity of both ease in and out parts.
     * You can use the smoothness values to control if this is an ease-in curve only, or ease-out or both ease-in/out, etc.
     * @param t The normalized time value, between 0 and 1.
     * @param easeInSmoothness The smoothness of the ease in part, where 0 means linear and 1 means smoothed. Values between 0 and 1 are valid.
     * @param easeOutSmoothness The smoothness of the ease out part, where 0 means linear and 1 means smoothed. Values between 0 and 1 are valid.
     * @result The interpolation fraction value. Use "startValue + SampleEaseInOutCurveWithSmoothness(...) * (endValue-startValue)" to interpolate, or use EaseInOutWithSmoothnessInterpolate(...).
     * @see EaseInOutWithSmoothnessInterpolate.
     */
    MCORE_INLINE float SampleEaseInOutCurveWithSmoothness(float t, float easeInSmoothness = 1.0f, float easeOutSmoothness = 1.0f);

    /**
     * Convert a linear interpolation weight value that is between 0 and 1 into a
     * smoothed version of that value that is also between 0 and 1.
     * This can be used to easily convert a linear interpolation into smooth looking interpolation.
     * @param linearValue The linear weight value that is between 0 and 1.
     * @result The smoothed version of the linear value, also in range of 0..1.
     */
    MCORE_INLINE float CalcCosineInterpolationWeight(float linearValue);

    /**
     * Linear interpolate from source into target.
     * @param source The source value to start interpolating from.
     * @param target The target value.
     * @param timeValue A value between 0 and 1, where 0 results in the source and 1 results in the target value.
     * @result The interpolated value.
     */
    template <class T>
    MCORE_INLINE T LinearInterpolate(const T& source, const T& target, float timeValue);

    /**
     * Cosine interpolate from source into target.
     * @param source The source value to start interpolating from.
     * @param target The target value.
     * @param timeValue A value between 0 and 1, where 0 results in the source and 1 results in the target value.
     * @result The interpolated value.
     */
    template <class T>
    MCORE_INLINE T CosineInterpolate(const T& source, const T& target, float timeValue);


    /**
     * Ease in/out interpolation.
     * @param source The source value to start interpolating from.
     * @param target The target value.
     * @param timeValue The normalized time value, between 0 and 1.
     * @param k1 The normalized time value where the ease-in section stops.
     * @param k2 The normalized time value where the ease-out section starts.
     * @result The interpolated value.
     */
    template <class T>
    MCORE_INLINE T EaseInOutInterpolate(const T& source, const T& target, float timeValue, float k1 = 0.5f, float k2 = 0.5f);

    /**
     * Ease in/out interpolation, with smoothness control on both ease in and out parts.
     * @param source The source value to start interpolating from.
     * @param target The target value.
     * @param timeValue The normalized time value, between 0 and 1.
     * @param easeInSmoothness The smoothness of the ease in part, where 0 means linear and 1 means smoothed. Values between 0 and 1 are valid.
     * @param easeOutSmoothness The smoothness of the ease out part, where 0 means linear and 1 means smoothed. Values between 0 and 1 are valid.
     * @result The interpolated value.
     */
    template <class T>
    MCORE_INLINE T EaseInOutWithSmoothnessInterpolate(const T& source, const T& target, float timeValue, float easeInSmoothness = 1.0f, float easeOutSmoothness = 1.0f);

    /**
     * Calculate an interpolated value using barycentric coordinates.
     * Given three attributes and an u and v, which range from 0..1, we can calculate the interpolated attribute value at the given u, v position.
     * @param u The barycentric u coordinate, between 0 and 1.
     * @param v The barycentric v coordinate, between 0 and 1.
     * @param pointA The first attribute value (for example the first vertex normal of a triangle).
     * @param pointB The second attribute value (for example the second vertex normal of a triangle).
     * @param pointC The third attribute value (for example the third vertex normal of the triangle).
     * @result The interpolated attribute value, at the given u and v coordinate.
     */
    template <class T>
    MCORE_INLINE T BarycentricInterpolate(float u, float v, const T& pointA, const T& pointB, const T& pointC);

    /**
     * Swaps the two objects.
     * @param a The first object.
     * @param b The second object.
     */
    template<class T>
    MCORE_INLINE void Swap(T& a, T& b);

    /**
     * Returns the smaller value of two objects.
     * @param a The first object.
     * @param b The second object.
     * @result The smaller value of the two.
     */
    template<class T>
    MCORE_INLINE T Min(T a, T b);

    /**
     * Returns the greater value of two objects.
     * @param a The first object.
     * @param b The second object.
     * @result The greater value of the two.
     */
    template<class T>
    MCORE_INLINE T Max(T a, T b);

    /**
     * Returns the smaller value of three objects.
     * @param a The first object.
     * @param b The second object.
     * @param c The third object.
     * @result The smaller value of the three.
     */
    template<class T>
    MCORE_INLINE T Min3(T a, T b, T c);

    /**
     * Returns the greater value of three objects.
     * @param a The first object.
     * @param b The second object.
     * @param c The third object.
     * @result The greater value of the three.
     */
    template<class T>
    MCORE_INLINE T Max3(T a, T b, T c);


    template<class T>
    MCORE_INLINE T Sgn(T A);

    /**
     * Calculates the square of the value.
     * @param x The value to square.
     * @return The square.
     */
    template<class T>
    MCORE_INLINE T Square(T x);

    /**
     * Calculates the square of the value.
     * @param x The value to square.
     * @return The square.
     */
    template<class T>
    MCORE_INLINE T Sqr(T x);

    /**
     * Returns true if the value is negative, false if not.
     * @param x The value to check.
     * @result True if the value is negative, false if not.
     */
    template<class T>
    MCORE_INLINE bool IsNegative(T x);

    /**
     * Returns true if the value is negative, false if not.
     * @param x The value to check.
     * @result True if the value is negative, false if not.
     */
    template<class T>
    MCORE_INLINE bool IsPositive(T x);

    /**
     * Modifies x if the object is lower than min or greater
     * than max and returns the object.
     * @param x The object to check.
     * @param min The minimal value.
     * @param max The maximal value.
     * @return The modified object.
     */
    template<class T>
    MCORE_INLINE T Clamp(T x, T min, T max);

    /**
     * Returns true if the value is bigger or equal than the lower limit or smaller or equal than the high limit.
     * @param x The value to check.
     * @param low The low range limit.
     * @param high The high range limit.
     * @result Returns true if the value is bigger or equal than the lower limit or smaller or equal than the high limit, otherwise false.
     */
    template<class T>
    MCORE_INLINE bool InRange(T x, T low, T high);

    /**
     * Convert from cartesian coordinates into spherical coordinates.
     * This uses the y-axis (up) and x-axis (right) as basis.
     * The input vector needs to be normalized!
     * @param normalizedVector The normalized direction vector to convert into spherical coordinates.
     * @result The spherical angles, in radians.
     */
    AZ::Vector2 MCORE_API ToSpherical(const AZ::Vector3& normalizedVector);

    /**
     * Convert from spherical coordinates back into cartesian coordinates.
     * this uses the y-axis (up) and x-axis (right) as basis.
     * @param spherical The spherical coordinates, as returned by ToSpherical(...).
     * @result The unit direction vector that was converted from the spherical coordinates.
     */
    AZ::Vector3 MCORE_API FromSpherical(const AZ::Vector2& spherical);

    /**
     * Calculate the number of samples and spacing between each samples, given a duration and number of samples per second.
     * This will make sure all samples are evenly spaced.
     * @param samplesPerSecond The desired number of samples per second. It is not guaranteed it will be exactly this amount though, because of alignment needed to
     *                         create evenly spaced samples.
     * @param duration The duration of the range in seconds.
     * @param outSampleTimeStep The spacing in seconds between the samples will be stored in this float.
     * @param outNumSamples The resulting number of samples needed to cover the range.
     */
    void MCORE_API CalcSampleRateInfo(float samplesPerSecond, float duration, float* outSampleTimeStep, uint32* outNumSamples);

    /**
     * Calculate the area of the triangle based on three points in space.
     * @param v1 First point of the triangle.
     * @param v2 Second point of the triangle.
     * @param v3 Third point of the triangle.
     * @result The area of the triangle.
     */
    double MCORE_API CalcTriangleAreaAccurate(const AZ::Vector3& v1, const AZ::Vector3& v2, const AZ::Vector3& v3);
    float MCORE_API CalcTriangleArea(const AZ::Vector3& v1, const AZ::Vector3& v2, const AZ::Vector3& v3);


    // include inline code
#include "Algorithms.inl"
}   // namespace MCore
