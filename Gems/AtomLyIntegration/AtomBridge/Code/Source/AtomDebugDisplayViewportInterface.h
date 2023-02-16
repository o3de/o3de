

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/Math/Matrix3x4.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

namespace AZ::AtomBridge
{
    struct RenderState
    {
        AZ::Color   m_color = AZ::Color(0.0f, 0.0f, 0.0f, 1.0f);
        uint8_t     m_lineWidth = 1u;

        uint16_t    m_currentTransform = 0;
        enum { TransformStackSize = 32 };
        AZ::Matrix3x4 m_transformStack[TransformStackSize];

        AZ::RPI::AuxGeomDraw::OpacityType m_opacityType = AZ::RPI::AuxGeomDraw::OpacityType::Opaque;
        AZ::RPI::AuxGeomDraw::DepthTest m_depthTest = AZ::RPI::AuxGeomDraw::DepthTest::On;
        AZ::RPI::AuxGeomDraw::DepthWrite m_depthWrite = AZ::RPI::AuxGeomDraw::DepthWrite::On;
        AZ::RPI::AuxGeomDraw::FaceCullMode m_faceCullMode = AZ::RPI::AuxGeomDraw::FaceCullMode::Back;
        int32_t m_viewProjOverrideIndex = -1; // will be used to implement SetDrawInFrontMode & 2D mode

        // separate tracking for Cry only state
        bool m_drawInFront = false;
        bool m_2dMode = false;
    };

    //! Utility class to collect line segments when the number of segments is known at compile time.
    template <int MaxNumLines> 
    struct SingleColorStaticSizeLineHelper
    {
        bool AddLineSegment(const AZ::Vector3& lineStart, const AZ::Vector3& lineEnd)
        {
            if ((m_points.size()+2) < m_points.capacity())
            {
                m_points.push_back(lineStart);
                m_points.push_back(lineEnd);
                return true;
            }
            return false;
        }

        void Draw(AZ::RPI::AuxGeomDrawPtr auxGeomDrawPtr, const RenderState& rendState) const
        {
            if (auxGeomDrawPtr && !m_points.empty())
            {
                AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
                drawArgs.m_verts = m_points.data();
                drawArgs.m_vertCount = aznumeric_cast<uint32_t>(m_points.size());
                drawArgs.m_colors = &rendState.m_color;
                drawArgs.m_colorCount = 1;
                drawArgs.m_size = rendState.m_lineWidth;
                drawArgs.m_opacityType = rendState.m_opacityType;
                drawArgs.m_depthTest = rendState.m_depthTest;
                drawArgs.m_depthWrite = rendState.m_depthWrite;
                drawArgs.m_viewProjectionOverrideIndex = rendState.m_viewProjOverrideIndex;
                auxGeomDrawPtr->DrawLines( drawArgs );
            }
        }

        void Draw2d(AZ::RPI::AuxGeomDrawPtr auxGeomDrawPtr, const RenderState& rendState) const
        {
            if (auxGeomDrawPtr && !m_points.empty())
            {
                AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
                drawArgs.m_verts = m_points.data();
                drawArgs.m_vertCount = aznumeric_cast<uint32_t>(m_points.size());
                drawArgs.m_colors = &rendState.m_color;
                drawArgs.m_colorCount = 1;
                drawArgs.m_size = rendState.m_lineWidth;
                drawArgs.m_opacityType = rendState.m_opacityType;
                drawArgs.m_depthTest = rendState.m_depthTest;
                drawArgs.m_depthWrite = rendState.m_depthWrite;
                drawArgs.m_viewProjectionOverrideIndex = auxGeomDrawPtr->GetOrAdd2DViewProjOverride();
                auxGeomDrawPtr->DrawLines( drawArgs );
            }
        }

        void Reset()
        {
            m_points.clear();
        }

        AZStd::fixed_vector<AZ::Vector3, 2 * MaxNumLines> m_points;
    };

    //! Utility class to collect line segments
    struct SingleColorDynamicSizeLineHelper final
    {
        SingleColorDynamicSizeLineHelper(int estimatedNumLineSegments);
        void AddLineSegment(const AZ::Vector3& lineStart, const AZ::Vector3& lineEnd);
        void Draw(AZ::RPI::AuxGeomDrawPtr auxGeomDrawPtr, const RenderState& rendState) const;
        void Draw2d(AZ::RPI::AuxGeomDrawPtr auxGeomDrawPtr, const RenderState& rendState) const;
        void Reset();

        AZStd::vector<AZ::Vector3> m_points;
    };

    class AtomDebugDisplayViewportInterface final
        : public AzFramework::DebugDisplayRequestBus::Handler
        , public AZ::RPI::ViewportContextIdNotificationBus::Handler
    {
    public:
        AZ_RTTI(AtomDebugDisplayViewportInterface, "{09AF6A46-0100-4FBF-8F94-E6B221322D14}", AzFramework::DebugDisplayRequestBus::Handler);

        explicit AtomDebugDisplayViewportInterface(AZ::RPI::ViewportContextPtr viewportContextPtr);
        explicit AtomDebugDisplayViewportInterface(uint32_t defaultInstanceAddress, RPI::Scene* scene);
        ~AtomDebugDisplayViewportInterface();

        void ResetRenderState();

        ////////////////////////////////////////////////////////////////////////////
        // AzFramework/Entity/DebugDisplayRequestBus::Handler overrides ...
        // Partial implementation of the DebugDisplayRequestBus on Atom. 
        // Commented out function prototypes are remaining part of the api
        // waiting to be implemented.
        // work tracked in [ATOM-3459]
        void SetColor(const AZ::Color& color) override;
        void SetAlpha(float a) override;
        void DrawQuad(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4) override;
        void DrawQuad(float width, float height, bool drawShaded) override;
        void DrawWireQuad(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4) override;
        void DrawWireQuad(float width, float height) override;
        void DrawQuadGradient(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4, const AZ::Vector4& firstColor, const AZ::Vector4& secondColor) override;
        void DrawQuad2dGradient(const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& p4, float z, const Color& firstColor, const Color& secondColor) override;
        void DrawTri(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3) override;
        void DrawTriangles(const AZStd::vector<AZ::Vector3>& vertices, const AZ::Color& color) override;
        void DrawTrianglesIndexed(const AZStd::vector<AZ::Vector3>& vertices, const AZStd::vector<AZ::u32>& indices, const AZ::Color& color) override;
        void DrawWireBox(const AZ::Vector3& min, const AZ::Vector3& max) override;
        void DrawSolidBox(const AZ::Vector3& min, const AZ::Vector3& max) override;
        void DrawWireOBB(const AZ::Vector3& center, const AZ::Vector3& axisX, const AZ::Vector3& axisY, const AZ::Vector3& axisZ, const AZ::Vector3& halfExtents) override;
        void DrawSolidOBB(const AZ::Vector3& center, const AZ::Vector3& axisX, const AZ::Vector3& axisY, const AZ::Vector3& axisZ, const AZ::Vector3& halfExtents) override;
        void DrawPoint(const AZ::Vector3& p, int nSize = 1) override;
        void DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2) override;
        void DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector4& col1, const AZ::Vector4& col2) override;
        void DrawLines(const AZStd::vector<AZ::Vector3>& lines, const AZ::Color& color) override;
        void DrawPolyLine(const AZ::Vector3* pnts, int numPoints, bool cycled = true) override;
        void DrawPolyLine(AZStd::span<const AZ::Vector3>, bool cycled = true) override;
        void DrawWireQuad2d(const AZ::Vector2& p1, const AZ::Vector2& p2, float z) override;
        void DrawLine2d(const AZ::Vector2& p1, const AZ::Vector2& p2, float z) override;
        void DrawLine2dGradient(const AZ::Vector2& p1, const AZ::Vector2& p2, float z, const AZ::Vector4& firstColor, const AZ::Vector4& secondColor) override;
        void DrawWireCircle2d(const AZ::Vector2& center, float radius, float z) override;
        void DrawArc(const AZ::Vector3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, int referenceAxis = 2) override;
        void DrawArc(const AZ::Vector3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, const AZ::Vector3& fixedAxis) override;
        void DrawCircle(const AZ::Vector3& pos, float radius, int nUnchangedAxis = 2 /*z axis*/) override;
        void DrawHalfDottedCircle(const AZ::Vector3& pos, float radius, const AZ::Vector3& viewPos, int nUnchangedAxis = 2 /*z axis*/) override;
        void DrawWireCone(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius, float height) override;
        void DrawSolidCone(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius, float height, bool drawShaded) override;
        void DrawWireCylinder(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height) override;
        void DrawSolidCylinder(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height, bool drawShaded) override;
        void DrawWireCylinderNoEnds(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height) override;
        void DrawSolidCylinderNoEnds(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height, bool drawShaded) override;
        void DrawWireCapsule(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float heightStraightSection) override;
        void DrawWireSphere(const AZ::Vector3& pos, float radius) override;
        void DrawWireSphere(const AZ::Vector3& pos, const AZ::Vector3 radius) override;
        void DrawWireHemisphere(const AZ::Vector3& pos, const AZ::Vector3& axis, float radius) override;
        void DrawWireDisk(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius) override;
        void DrawBall(const AZ::Vector3& pos, float radius, bool drawShaded) override;
        void DrawDisk(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius, bool drawShaded) override;
        void DrawArrow(const AZ::Vector3& src, const AZ::Vector3& trg, float headScale = 1.0f, bool dualEndedArrow = false) override;
        void DrawTextLabel(const AZ::Vector3& pos, float size, const char* text, const bool bCenter = false, int srcOffsetX = 0, int srcOffsetY = 0) override;
        void Draw2dTextLabel(float x, float y, float size, const char* text, bool bCenter = false) override;
        void DrawTextOn2DBox(const AZ::Vector3& pos, const char* text, float textScale, const AZ::Vector4& TextColor, const AZ::Vector4& TextBackColor) override;
        void SetLineWidth(float width) override;
        bool IsVisible(const AZ::Aabb& bounds) override;
        float GetLineWidth() override;
        float GetAspectRatio() override;
        void DepthTestOff() override;
        void DepthTestOn() override;
        void DepthWriteOff() override;
        void DepthWriteOn() override;
        void CullOff() override;
        void CullOn() override;
        bool SetDrawInFrontMode(bool on) override;
        AZ::u32 GetState() override;
        AZ::u32 SetState(AZ::u32 state) override;
        void PushMatrix(const AZ::Transform& tm) override;
        void PopMatrix() override;
        void PushPremultipliedMatrix(const AZ::Matrix3x4& matrix) override;
        AZ::Matrix3x4 PopPremultipliedMatrix() override;

    private:

        // ViewportContextIdNotificationBus handlers
        void OnViewportDefaultViewChanged(AZ::RPI::ViewPtr view) override;


        // internal helper functions
        using LineSegmentFilterFunc = AZStd::function<bool(const AZ::Vector3& lineStart, const AZ::Vector3& lineEnd, int segmentIndex)>;
        enum CircleAxis
        {
            CircleAxisX = 0,
            CircleAxisY = 1,
            CircleAxisZ = 2,
            CircleAxisMax = 3,
        };
        template<typename LineStorageType>
        void CreateAxisAlignedArc(
            LineStorageType& lines, 
            float segmentAngle, // radians
            float minAngle,     // radians
            float maxAngle,     // radians
            const AZ::Vector3& position, 
            const AZ::Vector3& radiusV3, 
            CircleAxis circleAxis, 
            LineSegmentFilterFunc filterFunc = 
                [](const AZ::Vector3&, const AZ::Vector3&, int)
                {return true;}
            );
        
        template<typename LineStorageType>
        void CreateArbitraryAxisArc(
            LineStorageType& lines, 
            float segmentAngle, // radians
            float minAngle,     // radians
            float maxAngle,     // radians
            const AZ::Vector3& position, 
            const AZ::Vector3& radiusV3, 
            const AZ::Vector3& axis, 
            LineSegmentFilterFunc filterFunc = 
                [](const AZ::Vector3&, const AZ::Vector3&, int)
                {return true;}
            );


        //! Convert position to world space.
        AZ::Vector3 ToWorldSpacePosition(const AZ::Vector3& v) const { return m_rendState.m_transformStack[m_rendState.m_currentTransform] * v; }

        //! Convert direction to world space (translation is not considered)
        AZ::Vector3 ToWorldSpaceVector(const AZ::Vector3& v) const { return m_rendState.m_transformStack[m_rendState.m_currentTransform].Multiply3x3(v); }

        //! Convert positions to world space.
        AZStd::vector<AZ::Vector3> ToWorldSpacePosition(const AZStd::vector<AZ::Vector3>& positions) const;

        //! Convert directions to world space (translation is not considered)
        AZStd::vector<AZ::Vector3> ToWorldSpaceVector(const AZStd::vector<AZ::Vector3>& vectors) const;

        void CalcBasisVectors(const AZ::Vector3& n, AZ::Vector3& b1, AZ::Vector3& b2) const;
        
        const AZ::Matrix3x4& GetCurrentTransform() const;

        void UpdateAuxGeom(RPI::Scene* scene, AZ::RPI::View* view);
        void InitInternal(RPI::Scene* scene, AZ::RPI::ViewportContextPtr viewportContextPtr);

        AZ::RPI::ViewportContextPtr GetViewportContext() const;

        uint32_t ConvertRenderStateToCry() const;

        RenderState m_rendState;

        AZ::RPI::AuxGeomDrawPtr m_auxGeomPtr;

        // m_defaultInstance is true for the instance that multicasts the debug draws to all viewports
        // (with an AuxGeom render pass) in the default scene.
        bool                    m_defaultInstance = false;
        AzFramework::ViewportId m_viewportId = AzFramework::InvalidViewportId; // Address this instance answers on.
        AZ::RPI::ViewportContext::SceneChangedEvent::Handler m_sceneChangeHandler;
    };

    // this is duplicated from Cry_Math.h, GetBasisVectors.
    // Need to match it's behavior to get the same orientations on curves.
    inline void AtomDebugDisplayViewportInterface::CalcBasisVectors(
        const AZ::Vector3& unitVector,
        AZ::Vector3& basis1,
        AZ::Vector3& basis2
        ) const
    {
        if (unitVector.GetZ() < FLT_EPSILON - 1.0f)
        {
            basis1 = AZ::Vector3(0.0f, -1.0f, 0.0f);
            basis2 = AZ::Vector3(-1.0f, 0.0f, 0.0f);
            return;
        }

        const float a = 1.0f / (1.0f + unitVector.GetZ());
        const float b = -unitVector.GetX() * unitVector.GetY() * a;
        basis1 = AZ::Vector3(1.0f - unitVector.GetX() * unitVector.GetX() * a, b, -unitVector.GetX());
        basis2 = AZ::Vector3(b, 1.0f - unitVector.GetY() * unitVector.GetY() * a, -unitVector.GetY());
    }
    
    template<typename LineStorageType>
    void AtomDebugDisplayViewportInterface::CreateAxisAlignedArc(
                    LineStorageType& lines,
                    float segmentAngle, // radians
                    float minAngle,     // radians
                    float maxAngle,     // radians
                    const AZ::Vector3& position,
                    const AZ::Vector3&  radiusV3,
                    CircleAxis circleAxis,
                    LineSegmentFilterFunc filterFunc)
    {
        AZ::Vector3 p1;
        AZ::Vector3 sinCos = AZ::Vector3::CreateZero();
        const uint32_t circleAxis1 = (circleAxis + 1) % CircleAxisMax;
        const uint32_t circleAxis2 = (circleAxis + 2) % CircleAxisMax;

        sinCos.SetElement(circleAxis1, sinf(minAngle));
        sinCos.SetElement(circleAxis2, cosf(minAngle));
        AZ::Vector3 p0 = position + radiusV3 * sinCos;
        p0 = ToWorldSpacePosition(p0);
        int segmentIndex = 0;
        for (float angle = minAngle + segmentAngle; angle < maxAngle; angle += segmentAngle)
        {
            float calcAngle = AZStd::clamp(angle, minAngle, maxAngle);
            sinCos.SetElement(circleAxis1, sinf(calcAngle));
            sinCos.SetElement(circleAxis2, cosf(calcAngle));
            p1 = position + radiusV3 * sinCos;
            p1 = ToWorldSpacePosition(p1);
            if (filterFunc(p0, p1, segmentIndex))
            {
                lines.AddLineSegment(p0, p1);
            }
            p0 = p1;
            ++segmentIndex;
        }
        // Complete the arc by drawing the last bit
        sinCos.SetElement(circleAxis1, sinf(maxAngle));
        sinCos.SetElement(circleAxis2, cosf(maxAngle));
        p1 = position + radiusV3 * sinCos;
        p1 = ToWorldSpacePosition(p1);
        if (filterFunc(p0, p1, segmentIndex))
        {
            lines.AddLineSegment(p0, p1);
        }
    }

    template<typename LineStorageType>
    void AtomDebugDisplayViewportInterface::CreateArbitraryAxisArc(
                    LineStorageType& lines,
                    float segmentAngle, // radians
                    float minAngle,     // radians
                    float maxAngle,     // radians
                    const AZ::Vector3& position,
                    const AZ::Vector3& radiusV3,
                    const AZ::Vector3& axis,
                    LineSegmentFilterFunc filterFunc)
    {
        AZ::Vector3 p1;
        float sinVF;
        float cosVF;
        AZ::SinCos(minAngle, sinVF, cosVF);

        AZ::Vector3 a, b;
        CalcBasisVectors(axis, a, b);

        AZ::Vector3 p0 = position + radiusV3 * (cosVF * a + sinVF * b);
        p0 = ToWorldSpacePosition(p0);
        int segmentIndex = 0;
        for (float angle = minAngle + segmentAngle; angle < maxAngle; angle += segmentAngle)
        {
            float calcAngle = AZ::GetClamp(angle, minAngle, maxAngle);
            AZ::SinCos(calcAngle, sinVF, cosVF);
            p1 = position + radiusV3 * (cosVF * a + sinVF * b);
            p1 = ToWorldSpacePosition(p1);
            if (filterFunc(p0, p1, segmentIndex))
            {
                lines.AddLineSegment(p0, p1);
            }
            p0 = p1;
            ++segmentIndex;
        }
        // Complete the arc by drawing the last bit
        AZ::SinCos(maxAngle, sinVF, cosVF);
        p1 = position + radiusV3 * (cosVF * a + sinVF * b);
        p1 = ToWorldSpacePosition(p1);
        if (filterFunc(p0, p1, segmentIndex))
        {
            lines.AddLineSegment(p0, p1);
        }
    }
} // namespace AZ::AtomBridge
