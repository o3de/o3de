/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Viewport/CameraState.h>

namespace AZ
{
    class Entity;
}

class ITexture;

namespace AzFramework
{
    inline constexpr AZ::s32 g_defaultSceneEntityDebugDisplayId = AZ_CRC_CE("MainViewportEntityDebugDisplayId"); // default id to draw to all viewports in the default scene

    // Notes:
    //   Don't change the xxxShift values, they need to match the values from legacy cry rendering
    //   This also applies to the individual flags in EAuxGeomPublicRenderflags_*!
    // Remarks:
    //   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
    //   Check RenderAuxGeom.h in ../RenderDll/Common
    enum EAuxGeomPublicRenderflagBitMasks: AZ::u32
    {
        e_Mode2D3DShift       = 31,
        e_Mode2D3DMask        = 0x1u << e_Mode2D3DShift,

        e_AlphaBlendingShift  = 29,
        e_AlphaBlendingMask   = 0x3u << e_AlphaBlendingShift,

        e_DrawInFrontShift    = 28,
        e_DrawInFrontMask     = 0x1u << e_DrawInFrontShift,

        e_FillModeShift       = 26,
        e_FillModeMask        = 0x3u << e_FillModeShift,

        e_CullModeShift       = 24,
        e_CullModeMask        = 0x3u << e_CullModeShift,

        e_DepthWriteShift     = 23,
        e_DepthWriteMask      = 0x1u << e_DepthWriteShift,

        e_DepthTestShift      = 22,
        e_DepthTestMask       = 0x1u << e_DepthTestShift,

        e_PublicParamsMask    = e_Mode2D3DMask | e_AlphaBlendingMask | e_DrawInFrontMask | e_FillModeMask | e_CullModeMask | e_DepthWriteMask | e_DepthTestMask
    };

    // Notes:
    //   e_Mode2D renders in normalized [0.. 1] screen space.
    //   Don't change the xxxShift values blindly as they affect the rendering output
    //   that is two primitives have to be rendered after 3d primitives, alpha blended
    //   geometry have to be rendered after opaque ones, etc.
    //   This also applies to the individual flags in EAuxGeomPublicRenderflagBitMasks!
    // Remarks:
    //   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
    //   Check RenderAuxGeom.h in ../RenderDll/Common
    // See also:
    //   EAuxGeomPublicRenderflagBitMasks
    enum EAuxGeomPublicRenderflags_Mode2D3D: AZ::u32
    {
        e_Mode3D = 0x0u << e_Mode2D3DShift,
        e_Mode2D = 0x1u << e_Mode2D3DShift,
    };

    // Notes:
    //   Don't change the xxxShift values blindly as they affect the rendering output
    //   that is two primitives have to be rendered after 3d primitives, alpha blended
    //   geometry have to be rendered after opaque ones, etc.
    //   This also applies to the individual flags in EAuxGeomPublicRenderflagBitMasks!
    // Remarks:
    //   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
    //   Check RenderAuxGeom.h in ../RenderDll/Common
    // See also:
    //   EAuxGeomPublicRenderflagBitMasks
    enum EAuxGeomPublicRenderflags_AlphaBlendMode: AZ::u32
    {
        e_AlphaNone     = 0x0u << e_AlphaBlendingShift,
        e_AlphaAdditive = 0x1u << e_AlphaBlendingShift,
        e_AlphaBlended  = 0x2u << e_AlphaBlendingShift,
    };

    // Notes:
    //   Don't change the xxxShift values blindly as they affect the rendering output
    //   that is two primitives have to be rendered after 3d primitives, alpha blended
    //   geometry have to be rendered after opaque ones, etc.
    //   This also applies to the individual flags in EAuxGeomPublicRenderflagBitMasks!
    // Remarks:
    //   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
    //   Check RenderAuxGeom.h in ../RenderDll/Common
    // See also:
    //   EAuxGeomPublicRenderflagBitMasks
    enum EAuxGeomPublicRenderflags_DrawInFrontMode: AZ::u32
    {
        e_DrawInFrontOff = 0x0u << e_DrawInFrontShift,
        e_DrawInFrontOn  = 0x1u << e_DrawInFrontShift,
    };

    // Notes:
    //   Don't change the xxxShift values blindly as they affect the rendering output
    //   that is two primitives have to be rendered after 3d primitives, alpha blended
    //   geometry have to be rendered after opaque ones, etc.
    //   This also applies to the individual flags in EAuxGeomPublicRenderflagBitMasks!
    // Remarks:
    //   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
    //   Check RenderAuxGeom.h in ../RenderDll/Common
    // See also:
    //   EAuxGeomPublicRenderflagBitMasks
    enum EAuxGeomPublicRenderflags_FillMode: AZ::u32
    {
        e_FillModeSolid     = 0x0u << e_FillModeShift,
        e_FillModeWireframe = 0x1u << e_FillModeShift,
        e_FillModePoint     = 0x2u << e_FillModeShift,
    };

    // Notes:
    //   Don't change the xxxShift values blindly as they affect the rendering output
    //   that is two primitives have to be rendered after 3d primitives, alpha blended
    //   geometry have to be rendered after opaque ones, etc.
    //   This also applies to the individual flags in EAuxGeomPublicRenderflagBitMasks!
    // Remarks:
    //   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
    //   Check RenderAuxGeom.h in ../RenderDll/Common
    // See also:
    //   EAuxGeomPublicRenderflagBitMasks
    enum EAuxGeomPublicRenderflags_CullMode: AZ::u32
    {
        e_CullModeNone  = 0x0u << e_CullModeShift,
        e_CullModeFront = 0x1u << e_CullModeShift,
        e_CullModeBack  = 0x2u << e_CullModeShift,
    };

    // Notes:
    //   Don't change the xxxShift values blindly as they affect the rendering output
    //   that is two primitives have to be rendered after 3d primitives, alpha blended
    //   geometry have to be rendered after opaque ones, etc.
    //   This also applies to the individual flags in EAuxGeomPublicRenderflagBitMasks!
    // Remarks:
    //   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
    //   Check RenderAuxGeom.h in ../RenderDll/Common
    // See also:
    //   EAuxGeomPublicRenderflagBitMasks
    enum EAuxGeomPublicRenderflags_DepthWrite: AZ::u32
    {
        e_DepthWriteOn  = 0x0u << e_DepthWriteShift,
        e_DepthWriteOff = 0x1u << e_DepthWriteShift,
    };

    // Notes:
    //   Don't change the xxxShift values blindly as they affect the rendering output
    //   that is two primitives have to be rendered after 3d primitives, alpha blended
    //   geometry have to be rendered after opaque ones, etc.
    //   This also applies to the individual flags in EAuxGeomPublicRenderflagBitMasks!
    // Remarks:
    //   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
    //   Check RenderAuxGeom.h in ../RenderDll/Common
    // See also:
    //   EAuxGeomPublicRenderflagBitMasks
    enum EAuxGeomPublicRenderflags_DepthTest: AZ::u32
    {
        e_DepthTestOn  = 0x0u << e_DepthTestShift,
        e_DepthTestOff = 0x1u << e_DepthTestShift,
    };

    /// DebugDisplayRequests provides a debug draw api to be used by components and viewport features.
    class DebugDisplayRequests
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        using BusIdType = AZ::s32;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void SetColor(const AZ::Color& color) { (void)color; }
        virtual void SetAlpha(float a) { (void)a; }
        virtual void DrawQuad(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4) { (void)p1; (void)p2; (void)p3; (void)p4; }
        virtual void DrawQuad(float width, float height, bool drawShaded = true) { (void)width; (void)height; (void)drawShaded; }
        virtual void DrawWireQuad(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4) { (void)p1; (void)p2; (void)p3; (void)p4; }
        virtual void DrawWireQuad(float width, float height) { (void)width; (void)height; }
        virtual void DrawQuadGradient(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4, const AZ::Vector4& firstColor, const AZ::Vector4& secondColor) { (void)p1; (void)p2; (void)p3; (void)p4; (void)firstColor; (void)secondColor; }

        /// Draws a 2D quad with gradient to the viewport.
        /// @param p1 p2 p3 p4 Normalized screen coordinates of the 4 points on the quad. <0, 0> is the upper-left corner of the viewport, <1,1> is the lower-right corner of the viewport.
        /// @param z Depth value. Useful when rendering multiple 2d quads on screen and you want them to stack in a particular order.
        /// @param firstColor. Vertex color applied to p1 p2.
        /// @param secondColor. Vertex color applied to p3 p4.
        virtual void DrawQuad2dGradient(const AZ::Vector2& p1, const AZ::Vector2& p2, const AZ::Vector2& p3, const AZ::Vector2& p4, float z, const AZ::Color& firstColor, const AZ::Color& secondColor) { (void)p1; (void)p2; (void)p3; (void)p4; (void)z; (void)firstColor; (void)secondColor; }
        virtual void DrawTri(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3) { (void)p1; (void)p2; (void)p3; }
        virtual void DrawTriangles(const AZStd::vector<AZ::Vector3>& vertices, const AZ::Color& color) { (void)vertices; (void)color; }
        virtual void DrawTrianglesIndexed(const AZStd::vector<AZ::Vector3>& vertices, const AZStd::vector<AZ::u32>& indices, const AZ::Color& color) { (void)vertices; (void)indices, (void)color; }
        virtual void DrawWireBox(const AZ::Vector3& min, const AZ::Vector3& max) { (void)min; (void)max; }
        virtual void DrawSolidBox(const AZ::Vector3& min, const AZ::Vector3& max) { (void)min; (void)max; }
        virtual void DrawWireOBB(const AZ::Vector3& center, const AZ::Vector3& axisX, const AZ::Vector3& axisY, const AZ::Vector3& axisZ, const AZ::Vector3& halfExtents) { (void)center; (void)axisX; (void)axisY; (void)axisZ; (void)halfExtents; }
        virtual void DrawSolidOBB(const AZ::Vector3& center, const AZ::Vector3& axisX, const AZ::Vector3& axisY, const AZ::Vector3& axisZ, const AZ::Vector3& halfExtents) { (void)center; (void)axisX; (void)axisY; (void)axisZ; (void)halfExtents; }
        virtual void DrawPoint(const AZ::Vector3& p, int nSize = 1) { (void)p; (void)nSize; }
        virtual void DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2) { (void)p1; (void)p2; }
        virtual void DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector4& col1, const AZ::Vector4& col2) { (void)p1; (void)p2; (void)col1; (void)col2; }
        virtual void DrawLines(const AZStd::vector<AZ::Vector3>& lines, const AZ::Color& color) { (void)lines; (void)color; }
        virtual void DrawPolyLine(const AZ::Vector3* pnts, int numPoints, bool cycled = true) { (void)pnts; (void)numPoints; (void)cycled; }
        virtual void DrawPolyLine(AZStd::span<const AZ::Vector3> points, bool cycled = true) { (void)points; (void)cycled; }
        virtual void DrawWireQuad2d(const AZ::Vector2& p1, const AZ::Vector2& p2, float z) { (void)p1; (void)p2; (void)z; }
        virtual void DrawLine2d(const AZ::Vector2& p1, const AZ::Vector2& p2, float z) { (void)p1; (void)p2; (void)z; }
        virtual void DrawLine2dGradient(const AZ::Vector2& p1, const AZ::Vector2& p2, float z, const AZ::Vector4& firstColor, const AZ::Vector4& secondColor) { (void)p1; (void)p2; (void)z; (void)firstColor; (void)secondColor; }
        virtual void DrawWireCircle2d(const AZ::Vector2& center, float radius, float z) { (void)center; (void)radius; (void)z; }
        virtual void DrawArc(const AZ::Vector3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, int referenceAxis = 2) { (void)pos; (void)radius; (void)startAngleDegrees; (void)sweepAngleDegrees; (void)angularStepDegrees; (void)referenceAxis; }
        virtual void DrawArc(const AZ::Vector3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, const AZ::Vector3& fixedAxis) { (void)pos; (void)radius; (void)startAngleDegrees; (void)sweepAngleDegrees; (void)angularStepDegrees; (void)fixedAxis; }
        virtual void DrawCircle(const AZ::Vector3& pos, float radius, int nUnchangedAxis = 2 /*z axis*/) { (void)pos; (void)radius; (void)nUnchangedAxis; }
        virtual void DrawHalfDottedCircle(const AZ::Vector3& pos, float radius, const AZ::Vector3& viewPos, int nUnchangedAxis = 2 /*z axis*/) { (void)pos; (void)radius; (void)viewPos; (void)nUnchangedAxis; }
        virtual void DrawWireCone(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius, float height) { (void)pos; (void)dir; (void)radius; (void)height; }
        virtual void DrawSolidCone(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius, float height, bool drawShaded = true) { (void)pos; (void)dir; (void)radius; (void)height; (void)drawShaded; }
        virtual void DrawWireCylinder(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height) { (void)center; (void)axis; (void)radius; (void)height; }
        virtual void DrawSolidCylinder(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height, bool drawShaded = true) { (void)center; (void)axis; (void)radius; (void)height; (void)drawShaded; }
        virtual void DrawWireCylinderNoEnds(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height) { (void)center; (void)axis; (void)radius; (void)height; }
        virtual void DrawSolidCylinderNoEnds(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height, bool drawShaded = true) { (void)center; (void)axis; (void)radius; (void)height; (void)drawShaded; }
        virtual void DrawWireCapsule(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float heightStraightSection) { (void)center; (void)axis; (void)radius; (void)heightStraightSection; }
        virtual void DrawWireSphere(const AZ::Vector3& pos, float radius) { (void)pos; (void)radius; }
        virtual void DrawWireSphere(const AZ::Vector3& pos, const AZ::Vector3 radius) { (void)pos; (void)radius; }
        virtual void DrawWireHemisphere(const AZ::Vector3& pos, const AZ::Vector3& axis, float radius) { (void)pos; (void)axis; (void)radius; }
        virtual void DrawWireDisk(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius) { (void)pos; (void)dir; (void)radius; }
        virtual void DrawBall(const AZ::Vector3& pos, float radius, bool drawShaded = true) { (void)pos; (void)radius; (void)drawShaded; }
        virtual void DrawDisk(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius, bool drawShaded = true) { (void)pos; (void)dir; (void)radius; (void)drawShaded; }
        virtual void DrawArrow(const AZ::Vector3& src, const AZ::Vector3& trg, float fHeadScale = 1, bool b2SidedArrow = false) { (void)src; (void)trg; (void)fHeadScale; (void)b2SidedArrow; }
        virtual void DrawTextLabel(const AZ::Vector3& pos, float size, const char* text, const bool bCenter = false, int srcOffsetX = 0, int srcOffsetY = 0) { (void)pos; (void)size; (void)text; (void)bCenter; (void)srcOffsetX; (void)srcOffsetY; }
        virtual void Draw2dTextLabel(float x, float y, float size, const char* text, bool bCenter = false) { (void)x; (void)y; (void)size; (void)text; (void)bCenter; }
        virtual void DrawTextOn2DBox(const AZ::Vector3& pos, const char* text, float textScale, const AZ::Vector4& TextColor, const AZ::Vector4& TextBackColor) { (void)pos; (void)text; (void)textScale; (void)TextColor; (void)TextBackColor; }
        virtual void SetLineWidth(float width) { (void)width; }
        virtual bool IsVisible(const AZ::Aabb& bounds) { (void)bounds; return false; }
        virtual float GetLineWidth() { return 0.0f; }
        virtual float GetAspectRatio() { return 0.0f; }
        virtual void DepthTestOff() {}
        virtual void DepthTestOn() {}
        virtual void DepthWriteOff() {}
        virtual void DepthWriteOn() {}
        virtual void CullOff() {}
        virtual void CullOn() {}
        virtual bool SetDrawInFrontMode(bool bOn) { (void)bOn; return false; }
        virtual AZ::u32 GetState() { return 0; }

        /// Sets the render state which will affect proceeding draw calls.
        /// For example, if you want draws to happen in 2D screen-space, you can turn off the e_Mode3D flag and turn on the e_Mode2D flag.
        /// @param state is the bit field used to set render attributes. See CryCommon/IRenderAuxGeom.h for available bit flags.
        /// @return the current state
        virtual AZ::u32 SetState(AZ::u32 state) { (void)state; return 0; }
        virtual void PushMatrix(const AZ::Transform& tm) { (void)tm; }
        virtual void PopMatrix() {}
        virtual void PushPremultipliedMatrix(const AZ::Matrix3x4& matrix) { (void)matrix; }
        virtual AZ::Matrix3x4 PopPremultipliedMatrix() { return AZ::Matrix3x4::CreateIdentity(); }

    protected:
        virtual ~DebugDisplayRequests() = default;
    };

    /// Inherit from DebugDisplayRequestBus::Handler to implement the DebugDisplayRequests interface.
    using DebugDisplayRequestBus = AZ::EBus<DebugDisplayRequests>;

    /// Structure to hold information relevant to a given viewport.
    struct ViewportInfo
    {
        int m_viewportId; ///< Unique way to identify a given viewport.
    };

    /// Provide viewport drawing tied to a specific entity. Components can listen
    /// to EntityDebugDisplayEvents in order to draw debug visuals in the viewport
    /// for a given entity/component at the correct point in the frame.
    class EntityDebugDisplayEvents
        : public AZ::ComponentBus
    {
    public:
        using Bus = AZ::EBus<EntityDebugDisplayEvents>;

        /// Provide viewport drawing for a particular entity.
        /// @param viewportInfo Can be used to determine information such as the camera position.
        /// @param debugDisplay Contains interface for debug draw/display commands.
        virtual void DisplayEntityViewport(
            const ViewportInfo& /*viewportInfo*/,
            DebugDisplayRequests& /*debugDisplay*/) {}

    protected:
        ~EntityDebugDisplayEvents() = default;
    };

    // Inherit from this type to implement EntityDebugDisplayEvents.
    using EntityDebugDisplayEventBus = AZ::EBus<EntityDebugDisplayEvents>;

    /// Provide viewport drawing not tied to a specific entity. Any type can
    /// implement this bus to provide drawing commands from DebugDisplayRequests.
    class ViewportDebugDisplayEvents
        : public AZ::EBusTraits
    {
    public:
        using BusIdType = EntityContextId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        /// Display drawing in world space.
        virtual void DisplayViewport(
            const ViewportInfo& /*viewportInfo*/,
            DebugDisplayRequests& /*debugDisplay*/) {}

        /// Display drawing in screen space.
        virtual void DisplayViewport2d(
            const ViewportInfo& /*viewportInfo*/,
            DebugDisplayRequests& /*debugDisplay*/) {}

    protected:
        ~ViewportDebugDisplayEvents() = default;
    };

    // Inherit from this type to implement ViewportDebugDisplayEvents.
    using ViewportDebugDisplayEventBus = AZ::EBus<ViewportDebugDisplayEvents>;

    class DebugDisplayEvents
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<DebugDisplayEvents>;
        virtual void DrawGlobalDebugInfo() = 0;
    };

    using DebugDisplayEventBus = AZ::EBus<DebugDisplayEvents>;
} // namespace AzFramework
