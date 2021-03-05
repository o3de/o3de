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

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Viewport/CameraState.h>

namespace AZ
{
    class Entity;
}

struct DisplayContext;
class ITexture;

namespace AzFramework
{
    /// DebugDisplayRequests provides a debug draw api to be used by components and viewport features.
    class DebugDisplayRequests
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        using BusIdType = AZ::s32;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void SetColor(float r, float g, float b, float a = 1.f) { (void)r; (void)g; (void)b; (void)a; }
        virtual void SetColor(const AZ::Color& color) { (void)color; }
        virtual void SetColor(const AZ::Vector4& color) { (void)color; }
        virtual void SetAlpha(float a) { (void)a; }
        virtual void DrawQuad(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4) { (void)p1; (void)p2; (void)p3; (void)p4; }
        virtual void DrawQuad(float width, float height) { (void)width; (void)height; }
        virtual void DrawWireQuad(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4) { (void)p1; (void)p2; (void)p3; (void)p4; }
        virtual void DrawWireQuad(float width, float height) { (void)width; (void)height; }
        virtual void DrawQuadGradient(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4, const AZ::Vector4& firstColor, const AZ::Vector4& secondColor) { (void)p1; (void)p2; (void)p3; (void)p4; (void)firstColor; (void)secondColor; }
        virtual void DrawTri(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3) { (void)p1; (void)p2; (void)p3; }
        virtual void DrawTriangles(const AZStd::vector<AZ::Vector3>& vertices, const AZ::Color& color) { (void)vertices; (void)color; }
        virtual void DrawTrianglesIndexed(const AZStd::vector<AZ::Vector3>& vertices, const AZStd::vector<AZ::u32>& indices, const AZ::Color& color) { (void)vertices; (void)indices, (void)color; }
        virtual void DrawWireBox(const AZ::Vector3& min, const AZ::Vector3& max) { (void)min; (void)max; }
        virtual void DrawSolidBox(const AZ::Vector3& min, const AZ::Vector3& max) { (void)min; (void)max; }
        virtual void DrawSolidOBB(const AZ::Vector3& center, const AZ::Vector3& axisX, const AZ::Vector3& axisY, const AZ::Vector3& axisZ, const AZ::Vector3& halfExtents) { (void)center; (void)axisX; (void)axisY; (void)axisZ; (void)halfExtents; }
        virtual void DrawPoint(const AZ::Vector3& p, int nSize = 1) { (void)p; (void)nSize; }
        virtual void DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2) { (void)p1; (void)p2; }
        virtual void DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector4& col1, const AZ::Vector4& col2) { (void)p1; (void)p2; (void)col1; (void)col2; }
        virtual void DrawLines(const AZStd::vector<AZ::Vector3>& lines, const AZ::Color& color) { (void)lines; (void)color; }
        virtual void DrawPolyLine(const AZ::Vector3* pnts, int numPoints, bool cycled = true) { (void)pnts; (void)numPoints; (void)cycled; }
        virtual void DrawWireQuad2d(const AZ::Vector2& p1, const AZ::Vector2& p2, float z) { (void)p1; (void)p2; (void)z; }
        virtual void DrawLine2d(const AZ::Vector2& p1, const AZ::Vector2& p2, float z) { (void)p1; (void)p2; (void)z; }
        virtual void DrawLine2dGradient(const AZ::Vector2& p1, const AZ::Vector2& p2, float z, const AZ::Vector4& firstColor, const AZ::Vector4& secondColor) { (void)p1; (void)p2; (void)z; (void)firstColor; (void)secondColor; }
        virtual void DrawWireCircle2d(const AZ::Vector2& center, float radius, float z) { (void)center; (void)radius; (void)z; }
        virtual void DrawTerrainCircle(const AZ::Vector3& worldPos, float radius, float height) { (void)worldPos; (void)radius; (void)height; }
        virtual void DrawTerrainCircle(const AZ::Vector3& center, float radius, float angle1, float angle2, float height) { (void)center; (void)radius; (void)angle1; (void)angle2; (void)height; }
        virtual void DrawArc(const AZ::Vector3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, int referenceAxis = 2) { (void)pos; (void)radius; (void)startAngleDegrees; (void)sweepAngleDegrees; (void)angularStepDegrees; (void)referenceAxis; }
        virtual void DrawArc(const AZ::Vector3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, const AZ::Vector3& fixedAxis) { (void)pos; (void)radius; (void)startAngleDegrees; (void)sweepAngleDegrees; (void)angularStepDegrees; (void)fixedAxis; }
        virtual void DrawCircle(const AZ::Vector3& pos, float radius, int nUnchangedAxis = 2 /*z axis*/) { (void)pos; (void)radius; (void)nUnchangedAxis; }
        virtual void DrawHalfDottedCircle(const AZ::Vector3& pos, float radius, const AZ::Vector3& viewPos, int nUnchangedAxis = 2 /*z axis*/) { (void)pos; (void)radius; (void)viewPos; (void)nUnchangedAxis; }
        virtual void DrawCone(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius, float height, bool drawShaded = true) { (void)pos; (void)dir; (void)radius; (void)height; (void)drawShaded; }
        virtual void DrawWireCylinder(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height) { (void)center; (void)axis; (void)radius; (void)height; }
        virtual void DrawSolidCylinder(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height, bool drawShaded = true) { (void)center; (void)axis; (void)radius; (void)height; (void)drawShaded; }
        virtual void DrawWireCapsule(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float heightStraightSection) { (void)center; (void)axis; (void)radius; (void)heightStraightSection; }
        virtual void DrawTerrainRect(float x1, float y1, float x2, float y2, float height) { (void)x1; (void)y1; (void)x2; (void)y2; (void)height; }
        virtual void DrawTerrainLine(AZ::Vector3 worldPos1, AZ::Vector3 worldPos2) { (void)worldPos1; (void)worldPos2; }
        virtual void DrawWireSphere(const AZ::Vector3& pos, float radius) { (void)pos; (void)radius; }
        virtual void DrawWireSphere(const AZ::Vector3& pos, const AZ::Vector3 radius) { (void)pos; (void)radius; }
        virtual void DrawWireDisk(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius) { (void)pos; (void)dir; (void)radius; }
        virtual void DrawBall(const AZ::Vector3& pos, float radius, bool drawShaded = true) { (void)pos; (void)radius; (void)drawShaded; }
        virtual void DrawDisk(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius) { (void)pos; (void)dir; (void)radius; }
        virtual void DrawArrow(const AZ::Vector3& src, const AZ::Vector3& trg, float fHeadScale = 1, bool b2SidedArrow = false) { (void)src; (void)trg; (void)fHeadScale; (void)b2SidedArrow; }
        virtual void DrawTextLabel(const AZ::Vector3& pos, float size, const char* text, const bool bCenter = false, int srcOffsetX = 0, int srcOffsetY = 0) { (void)pos; (void)size; (void)text; (void)bCenter; (void)srcOffsetX; (void)srcOffsetY; }
        virtual void Draw2dTextLabel(float x, float y, float size, const char* text, bool bCenter = false) { (void)x; (void)y; (void)size; (void)text; (void)bCenter; }
        virtual void DrawTextOn2DBox(const AZ::Vector3& pos, const char* text, float textScale, const AZ::Vector4& TextColor, const AZ::Vector4& TextBackColor) { (void)pos; (void)text; (void)textScale; (void)TextColor; (void)TextBackColor; }
        virtual void DrawTextureLabel(ITexture* texture, const AZ::Vector3& pos, float sizeX, float sizeY, int texIconFlags) { (void)texture; (void)pos; (void)sizeX; (void)sizeY; (void)texIconFlags; }
        virtual void DrawTextureLabel(int textureId, const AZ::Vector3& pos, float sizeX, float sizeY, int texIconFlags) { (void)textureId; (void)pos; (void)sizeX; (void)sizeY; (void)texIconFlags; }
        virtual void SetLineWidth(float width) { (void)width; }
        virtual bool IsVisible(const AZ::Aabb& bounds) { (void)bounds; return false; }
        virtual int SetFillMode(int nFillMode) { (void)nFillMode; return 0; }
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
        virtual AZ::u32 SetState(AZ::u32 state) { (void)state; return 0; }
        virtual AZ::u32 SetStateFlag(AZ::u32 state) { (void)state; return 0; }
        virtual AZ::u32 ClearStateFlag(AZ::u32 state) { (void)state; return 0; }
        virtual void PushMatrix(const AZ::Transform& tm) { (void)tm; }
        virtual void PopMatrix() {}

    protected:
        ~DebugDisplayRequests() = default;
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
