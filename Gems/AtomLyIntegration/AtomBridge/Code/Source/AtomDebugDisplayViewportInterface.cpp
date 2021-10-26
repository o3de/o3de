/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Frustum.h>
#include <AtomDebugDisplayViewportInterface.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AzCore/Interface/Interface.h>

#include <AzFramework/Font/FontInterface.h>

namespace // unnamed namespace to hold copies of Cry AuxGeom state enum's, this is to avoid creating a dependency on IRenderAuxGeom.h
{
    // Notes:
    //   Don't change the xxxShift values, they need to match the values from legacy cry rendering
    //   This also applies to the individual flags in EAuxGeomPublicRenderflags_*!
    // Remarks:
    //   Bits 0 - 22 are currently reserved for prim type and per draw call render parameters (point size, etc.)
    //   Check RenderAuxGeom.h in ../RenderDll/Common
    enum EAuxGeomPublicRenderflagBitMasks
    {
        e_Mode2D3DShift             = 31,
        e_Mode2D3DMask              = 0x1 << e_Mode2D3DShift,

        e_AlphaBlendingShift        = 29,
        e_AlphaBlendingMask         = 0x3 << e_AlphaBlendingShift,

        e_DrawInFrontShift          = 28,
        e_DrawInFrontMask           = 0x1 << e_DrawInFrontShift,

        e_FillModeShift             = 26,
        e_FillModeMask              = 0x3 << e_FillModeShift,

        e_CullModeShift             = 24,
        e_CullModeMask              = 0x3 << e_CullModeShift,

        e_DepthWriteShift           = 23,
        e_DepthWriteMask            = 0x1 << e_DepthWriteShift,

        e_DepthTestShift            = 22,
        e_DepthTestMask             = 0x1 << e_DepthTestShift,

        e_PublicParamsMask      = e_Mode2D3DMask | e_AlphaBlendingMask | e_DrawInFrontMask | e_FillModeMask |
            e_CullModeMask | e_DepthWriteMask | e_DepthTestMask
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
    enum EAuxGeomPublicRenderflags_Mode2D3D
    {
        e_Mode3D                            = 0x0 << e_Mode2D3DShift,
        e_Mode2D                            = 0x1 << e_Mode2D3DShift,
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
    enum EAuxGeomPublicRenderflags_AlphaBlendMode
    {
        e_AlphaNone                     = 0x0 << e_AlphaBlendingShift,
        e_AlphaAdditive             = 0x1 << e_AlphaBlendingShift,
        e_AlphaBlended              = 0x2 << e_AlphaBlendingShift,
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
    enum EAuxGeomPublicRenderflags_DrawInFrontMode
    {
        e_DrawInFrontOff            = 0x0 << e_DrawInFrontShift,
        e_DrawInFrontOn             = 0x1 << e_DrawInFrontShift,
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
    enum EAuxGeomPublicRenderflags_FillMode
    {
        e_FillModeSolid             = 0x0 << e_FillModeShift,
        e_FillModeWireframe     = 0x1 << e_FillModeShift,
        e_FillModePoint             = 0x2 << e_FillModeShift,
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
    enum EAuxGeomPublicRenderflags_CullMode
    {
        e_CullModeNone              = 0x0 << e_CullModeShift,
        e_CullModeFront             = 0x1 << e_CullModeShift,
        e_CullModeBack              = 0x2 << e_CullModeShift,
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
    enum EAuxGeomPublicRenderflags_DepthWrite
    {
        e_DepthWriteOn              = 0x0 << e_DepthWriteShift,
        e_DepthWriteOff             = 0x1 << e_DepthWriteShift,
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
    enum EAuxGeomPublicRenderflags_DepthTest
    {
        e_DepthTestOn                   = 0x0 << e_DepthTestShift,
        e_DepthTestOff              = 0x1 << e_DepthTestShift,
    };
};

namespace AZ::AtomBridge
{

    ////////////////////////////////////////////////////////////////////////
    SingleColorDynamicSizeLineHelper::SingleColorDynamicSizeLineHelper(
        int estimatedNumLineSegments
        )
    {
        m_points.reserve(estimatedNumLineSegments * 2);
    }

    void SingleColorDynamicSizeLineHelper::AddLineSegment(
        const AZ::Vector3& lineStart,
        const AZ::Vector3& lineEnd
        )
    {
        m_points.push_back(lineStart);
        m_points.push_back(lineEnd);
    }

    void SingleColorDynamicSizeLineHelper::Draw(
        AZ::RPI::AuxGeomDrawPtr auxGeomDrawPtr, 
        const RenderState& rendState
        ) const
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

    void SingleColorDynamicSizeLineHelper::Draw2d(
        AZ::RPI::AuxGeomDrawPtr auxGeomDrawPtr, 
        const RenderState& rendState
        ) const
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

    void SingleColorDynamicSizeLineHelper::Reset()
    {
        m_points.clear();
    }
    ////////////////////////////////////////////////////////////////////////

    AtomDebugDisplayViewportInterface::AtomDebugDisplayViewportInterface(AZ::RPI::ViewportContextPtr viewportContextPtr)
    {
        ResetRenderState();
        m_viewportId = viewportContextPtr->GetId();
        m_defaultInstance = false;
        auto setupScene = [this](RPI::ScenePtr scene)
        {
            auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
            AZ::RPI::ViewportContextPtr viewportContextPtr = viewportContextManager->GetViewportContextById(m_viewportId);
            InitInternal(scene.get(), viewportContextPtr);
        };
        setupScene(viewportContextPtr->GetRenderScene());
        m_sceneChangeHandler = AZ::RPI::ViewportContext::SceneChangedEvent::Handler(setupScene);
        viewportContextPtr->ConnectSceneChangedHandler(m_sceneChangeHandler);
    }

    AtomDebugDisplayViewportInterface::AtomDebugDisplayViewportInterface(uint32_t defaultInstanceAddress)
    {
        ResetRenderState();
        m_viewportId = defaultInstanceAddress;
        m_defaultInstance = true;
        RPI::Scene* scene = RPI::RPISystemInterface::Get()->GetDefaultScene().get();
        InitInternal(scene, nullptr);
    }

    void AtomDebugDisplayViewportInterface::UpdateAuxGeom(RPI::Scene* scene, AZ::RPI::View* view)
    {
        if (!scene)
        {
            m_auxGeomPtr = nullptr;
            return;
        }
        auto auxGeomFP = scene->GetFeatureProcessor<RPI::AuxGeomFeatureProcessorInterface>();
        if (!auxGeomFP)
        {
            m_auxGeomPtr = nullptr;
            return;
        }
        // default instance draws to all viewports in the default scene
        if (m_defaultInstance || !view)
        {
            m_auxGeomPtr = auxGeomFP->GetDrawQueue();
        }
        else
        {
            // cache the aux geom draw interface for the current view (aka camera)
            m_auxGeomPtr = auxGeomFP->GetOrCreateDrawQueueForView(view);
        }
    }

    void AtomDebugDisplayViewportInterface::InitInternal(RPI::Scene* scene, AZ::RPI::ViewportContextPtr viewportContextPtr)
    {
        AzFramework::DebugDisplayRequestBus::Handler::BusDisconnect(m_viewportId);
        UpdateAuxGeom(scene, viewportContextPtr ? viewportContextPtr->GetDefaultView().get() : nullptr);
        AzFramework::DebugDisplayRequestBus::Handler::BusConnect(m_viewportId);
        if (!m_defaultInstance) // only the per viewport instances need to listen for viewport changes
        {
            AZ::RPI::ViewportContextIdNotificationBus::Handler::BusConnect(viewportContextPtr->GetId());
        }
    }


    void AtomDebugDisplayViewportInterface::OnViewportDefaultViewChanged(AZ::RPI::ViewPtr view)
    {
        ResetRenderState();
        if (!m_defaultInstance)
        {
            // handle viewport update (view change, scene change, etc
            auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
            AZ::RPI::ViewportContextPtr viewportContextPtr = viewportContextManager->GetViewportContextById(m_viewportId);
            UpdateAuxGeom(viewportContextPtr->GetRenderScene().get(), viewportContextPtr->GetDefaultView().get());
        }
    }

    AtomDebugDisplayViewportInterface::~AtomDebugDisplayViewportInterface()
    {
        AzFramework::DebugDisplayRequestBus::Handler::BusDisconnect(m_viewportId);
        AZ::RPI::ViewportContextIdNotificationBus::Handler::BusDisconnect();
        m_viewportId = AzFramework::InvalidViewportId;
        m_auxGeomPtr = nullptr;
    }

    void AtomDebugDisplayViewportInterface::ResetRenderState()
    {
        m_rendState = RenderState();
        for (int index = 0; index < RenderState::TransformStackSize; ++index)
        {
            m_rendState.m_transformStack[index] = AZ::Matrix3x4::Identity();
        }
    }

    void AtomDebugDisplayViewportInterface::SetColor(float r, float g, float b, float a)
    {
        m_rendState.m_color = AZ::Color(r, g, b, a);
    }

    void AtomDebugDisplayViewportInterface::SetColor(const AZ::Color& color)
    {
        m_rendState.m_color = color;
    }

    void AtomDebugDisplayViewportInterface::SetColor(const AZ::Vector4& color)
    {
        m_rendState.m_color = AZ::Color(color);
    }

    void AtomDebugDisplayViewportInterface::SetAlpha(float a)
    {
        m_rendState.m_color.SetA(a);
        if (a < 1.0f)
        {
            m_rendState.m_opacityType = AZ::RPI::AuxGeomDraw::OpacityType::Opaque;
        }
        else
        {
            m_rendState.m_opacityType = AZ::RPI::AuxGeomDraw::OpacityType::Translucent;
        }
    }

    void AtomDebugDisplayViewportInterface::DrawQuad(
        const AZ::Vector3& p1, 
        const AZ::Vector3& p2, 
        const AZ::Vector3& p3, 
        const AZ::Vector3& p4)
    {
        if (m_auxGeomPtr)
        {
            AZ::Vector3 wsPoints[4] = { ToWorldSpacePosition(p1), ToWorldSpacePosition(p2), ToWorldSpacePosition(p3), ToWorldSpacePosition(p4) };
            AZ::Vector3 triangles[6];
            triangles[0] = wsPoints[0];
            triangles[1] = wsPoints[1];
            triangles[2] = wsPoints[2];
            triangles[3] = wsPoints[2];
            triangles[4] = wsPoints[3];
            triangles[5] = wsPoints[0];
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
            drawArgs.m_verts = triangles;
            drawArgs.m_vertCount = 6;
            drawArgs.m_colors = &m_rendState.m_color;
            drawArgs.m_colorCount = 1;
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_rendState.m_viewProjOverrideIndex;
            m_auxGeomPtr->DrawTriangles(drawArgs);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawQuad(float width, float height)
    {
        if (!m_auxGeomPtr || width <= 0.0f || height <= 0.0f)
        {
            return;
        }

        m_auxGeomPtr->DrawQuad(
            width, 
            height, 
            GetCurrentTransform(), 
            m_rendState.m_color,
            AZ::RPI::AuxGeomDraw::DrawStyle::Shaded,
            m_rendState.m_depthTest,
            m_rendState.m_depthWrite,
            m_rendState.m_faceCullMode,
            m_rendState.m_viewProjOverrideIndex);
    }

    void AtomDebugDisplayViewportInterface::DrawWireQuad(
        const AZ::Vector3& p1, 
        const AZ::Vector3& p2, 
        const AZ::Vector3& p3, 
        const AZ::Vector3& p4)
    {
        if (m_auxGeomPtr)
        {
            AZ::Vector3 wsPoints[4] = { ToWorldSpacePosition(p1), ToWorldSpacePosition(p2), ToWorldSpacePosition(p3), ToWorldSpacePosition(p4) };
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
            drawArgs.m_verts = wsPoints;
            drawArgs.m_vertCount = 4;
            drawArgs.m_colors = &m_rendState.m_color;
            drawArgs.m_colorCount = 1;
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_rendState.m_viewProjOverrideIndex;
            m_auxGeomPtr->DrawPolylines(drawArgs, AZ::RPI::AuxGeomDraw::PolylineEnd::Closed);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawWireQuad(float width, float height)
    {
        if (!m_auxGeomPtr || width <= 0.0f || height <= 0.0f)
        {
            return;
        }

        m_auxGeomPtr->DrawQuad(
            width, 
            height, 
            GetCurrentTransform(), 
            m_rendState.m_color, 
            AZ::RPI::AuxGeomDraw::DrawStyle::Line,
            m_rendState.m_depthTest,
            m_rendState.m_depthWrite,
            m_rendState.m_faceCullMode,
            m_rendState.m_viewProjOverrideIndex);
    }

    void AtomDebugDisplayViewportInterface::DrawQuadGradient(
        const AZ::Vector3& p1, 
        const AZ::Vector3& p2, 
        const AZ::Vector3& p3, 
        const AZ::Vector3& p4, 
        const AZ::Vector4& firstColor, 
        const AZ::Vector4& secondColor)
    {
        if (m_auxGeomPtr)
        {
            AZ::Vector3 wsPoints[4] = { ToWorldSpacePosition(p1), ToWorldSpacePosition(p2), ToWorldSpacePosition(p3), ToWorldSpacePosition(p4) };
            AZ::Vector3 triangles[6];
            AZ::Color colors[6];
            triangles[0] = wsPoints[0];         colors[0]    = firstColor;
            triangles[1] = wsPoints[1];         colors[1]    = firstColor;
            triangles[2] = wsPoints[2];         colors[2]    = secondColor;
            triangles[3] = wsPoints[2];         colors[3]    = secondColor;
            triangles[4] = wsPoints[3];         colors[4]    = secondColor;
            triangles[5] = wsPoints[0];         colors[5]    = firstColor;
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
            drawArgs.m_verts = triangles;
            drawArgs.m_vertCount = 6;
            drawArgs.m_colors = colors;
            drawArgs.m_colorCount = 6;
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_rendState.m_viewProjOverrideIndex;
            m_auxGeomPtr->DrawTriangles(drawArgs);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawTri(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3)
    {
        if (m_auxGeomPtr)
        {
            AZ::Vector3 verts[3] = {ToWorldSpacePosition(p1), ToWorldSpacePosition(p2), ToWorldSpacePosition(p3)};
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
            drawArgs.m_verts = verts;
            drawArgs.m_vertCount = 3;
            drawArgs.m_colors = &m_rendState.m_color;
            drawArgs.m_colorCount = 1;
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_rendState.m_viewProjOverrideIndex;
            m_auxGeomPtr->DrawTriangles(drawArgs);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawTriangles(const AZStd::vector<AZ::Vector3>& vertices, const AZ::Color& color)
    {
        if (m_auxGeomPtr)
        {
            AZStd::vector<AZ::Vector3> transformedVertices = ToWorldSpacePosition(vertices);
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
            drawArgs.m_verts = transformedVertices.data();
            drawArgs.m_vertCount = aznumeric_cast<uint32_t>(transformedVertices.size());
            drawArgs.m_colors = &color;
            drawArgs.m_colorCount = 1;
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_rendState.m_viewProjOverrideIndex;
            m_auxGeomPtr->DrawTriangles(drawArgs);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawTrianglesIndexed(
        const AZStd::vector<AZ::Vector3>& vertices, 
        const AZStd::vector<AZ::u32>& indices, 
        const AZ::Color& color)
    {
        if (m_auxGeomPtr)
        {
            AZStd::vector<AZ::Vector3> transformedVertices = ToWorldSpacePosition(vertices);
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicIndexedDrawArguments drawArgs;
            drawArgs.m_verts = transformedVertices.data();
            drawArgs.m_vertCount = aznumeric_cast<uint32_t>(transformedVertices.size());
            drawArgs.m_indices = indices.data();
            drawArgs.m_indexCount = aznumeric_cast<uint32_t>(indices.size());
            drawArgs.m_colors = &color;
            drawArgs.m_colorCount = 1;
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_rendState.m_viewProjOverrideIndex;
            m_auxGeomPtr->DrawTriangles(drawArgs);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawWireBox(const AZ::Vector3& min, const AZ::Vector3& max)
    {
        if (m_auxGeomPtr)
        {
            m_auxGeomPtr->DrawAabb(
                AZ::Aabb::CreateFromMinMax(min, max), 
                GetCurrentTransform(), 
                m_rendState.m_color, 
                AZ::RPI::AuxGeomDraw::DrawStyle::Line,
                m_rendState.m_depthTest,
                m_rendState.m_depthWrite,
                m_rendState.m_faceCullMode,
                m_rendState.m_viewProjOverrideIndex
                );
        }
    }

    void AtomDebugDisplayViewportInterface::DrawSolidBox(const AZ::Vector3& min, const AZ::Vector3& max)
    {
        if (m_auxGeomPtr)
        {
            m_auxGeomPtr->DrawAabb(
                AZ::Aabb::CreateFromMinMax(min, max), 
                GetCurrentTransform(), 
                m_rendState.m_color, 
                AZ::RPI::AuxGeomDraw::DrawStyle::Solid,
                m_rendState.m_depthTest,
                m_rendState.m_depthWrite,
                m_rendState.m_faceCullMode,
                m_rendState.m_viewProjOverrideIndex);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawWireOBB(
        const AZ::Vector3& center, 
        const AZ::Vector3& axisX, 
        const AZ::Vector3& axisY, 
        const AZ::Vector3& axisZ, 
        const AZ::Vector3& halfExtents)
    {
        if (m_auxGeomPtr)
        {
            AZ::Quaternion rotation = AZ::Quaternion::CreateFromMatrix3x3(AZ::Matrix3x3::CreateFromColumns(axisX, axisY, axisZ));
            AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(center, rotation, halfExtents);
            m_auxGeomPtr->DrawObb(
                obb,
                AZ::Vector3::CreateZero(), 
                m_rendState.m_color, 
                AZ::RPI::AuxGeomDraw::DrawStyle::Line,
                m_rendState.m_depthTest,
                m_rendState.m_depthWrite,
                m_rendState.m_faceCullMode,
                m_rendState.m_viewProjOverrideIndex);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawSolidOBB(
        const AZ::Vector3& center, 
        const AZ::Vector3& axisX, 
        const AZ::Vector3& axisY, 
        const AZ::Vector3& axisZ, 
        const AZ::Vector3& halfExtents)
    {
        if (m_auxGeomPtr)
        {
            AZ::Quaternion rotation = AZ::Quaternion::CreateFromMatrix3x3(AZ::Matrix3x3::CreateFromColumns(axisX, axisY, axisZ));
            AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(center, rotation, halfExtents);
            m_auxGeomPtr->DrawObb(
                obb,
                AZ::Vector3::CreateZero(), 
                m_rendState.m_color, 
                AZ::RPI::AuxGeomDraw::DrawStyle::Solid,
                m_rendState.m_depthTest,
                m_rendState.m_depthWrite,
                m_rendState.m_faceCullMode,
                m_rendState.m_viewProjOverrideIndex);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawPoint(const AZ::Vector3& p, int nSize)
    {
        if (m_auxGeomPtr)
        {
            AZ::Vector3 wsPoint = ToWorldSpacePosition(p);
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
            drawArgs.m_verts = &wsPoint;
            drawArgs.m_vertCount = 1;
            drawArgs.m_colors = &m_rendState.m_color;
            drawArgs.m_colorCount = 1;
            drawArgs.m_size = aznumeric_cast<uint8_t>(nSize);
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_rendState.m_viewProjOverrideIndex;
            m_auxGeomPtr->DrawPoints(drawArgs);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2)
    {
        if (m_auxGeomPtr)
        {
            AZ::Vector3 verts[2] = {ToWorldSpacePosition(p1), ToWorldSpacePosition(p2)};
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
            drawArgs.m_verts = verts;
            drawArgs.m_vertCount = 2;
            drawArgs.m_colors = &m_rendState.m_color;
            drawArgs.m_colorCount = 1;
            drawArgs.m_size = m_rendState.m_lineWidth;
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_rendState.m_viewProjOverrideIndex;
            m_auxGeomPtr->DrawLines(drawArgs);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector4& col1, const AZ::Vector4& col2)
    {
        if (m_auxGeomPtr)
        {
            AZ::Vector3 verts[2] = {ToWorldSpacePosition(p1), ToWorldSpacePosition(p2)};
            AZ::Color colors[2] = {col1, col2};
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
            drawArgs.m_verts = verts;
            drawArgs.m_vertCount = 2;
            drawArgs.m_colors = colors;
            drawArgs.m_colorCount = 2;
            drawArgs.m_size = m_rendState.m_lineWidth;
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_rendState.m_viewProjOverrideIndex;
            m_auxGeomPtr->DrawLines(drawArgs);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawLines(const AZStd::vector<AZ::Vector3>& lines, const AZ::Color& color)
    {
        if (m_auxGeomPtr)
        {
            AZStd::vector<AZ::Vector3> transformedLines = ToWorldSpacePosition(lines);
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
            drawArgs.m_verts = transformedLines.data();
            drawArgs.m_vertCount = aznumeric_cast<uint32_t>(transformedLines.size());
            drawArgs.m_colors = &color;
            drawArgs.m_colorCount = 1;
            drawArgs.m_size = m_rendState.m_lineWidth;
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_rendState.m_viewProjOverrideIndex;
            m_auxGeomPtr->DrawLines(drawArgs);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawPolyLine(const AZ::Vector3* pnts, int numPoints, bool cycled)
    {
        if (m_auxGeomPtr)
        {
            AZStd::vector<AZ::Vector3> wsPoints(static_cast<size_t>(numPoints));
            for (int index = 0; index < numPoints; ++index)
            {
                wsPoints[index] = ToWorldSpacePosition(pnts[index]);
            }
            AZ::RPI::AuxGeomDraw::PolylineEnd polylineEnd = cycled ? AZ::RPI::AuxGeomDraw::PolylineEnd::Closed : AZ::RPI::AuxGeomDraw::PolylineEnd::Open;
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
            drawArgs.m_verts = wsPoints.data();
            drawArgs.m_vertCount = aznumeric_cast<uint32_t>(numPoints);
            drawArgs.m_colors = &m_rendState.m_color;
            drawArgs.m_colorCount = 1;
            drawArgs.m_size = m_rendState.m_lineWidth;
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_rendState.m_viewProjOverrideIndex;
            m_auxGeomPtr->DrawPolylines(drawArgs, polylineEnd);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawWireQuad2d(const AZ::Vector2& p1, const AZ::Vector2& p2, float z)
    {
        if (m_auxGeomPtr)
        {
            AZ::Vector3 points[4];
            points[0] = AZ::Vector3(p1.GetX(), p1.GetY(), z);
            points[1] = AZ::Vector3(p2.GetX(), p1.GetY(), z);
            points[2] = AZ::Vector3(p2.GetX(), p2.GetY(), z);
            points[3] = AZ::Vector3(p1.GetX(), p2.GetY(), z);
            
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
            drawArgs.m_verts = points;
            drawArgs.m_vertCount = 4;
            drawArgs.m_colors = &m_rendState.m_color;
            drawArgs.m_colorCount = 1;
            drawArgs.m_size = m_rendState.m_lineWidth;
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_auxGeomPtr->GetOrAdd2DViewProjOverride();
            m_auxGeomPtr->DrawPolylines(drawArgs, AZ::RPI::AuxGeomDraw::PolylineEnd::Closed);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawLine2d(const AZ::Vector2& p1, const AZ::Vector2& p2, float z)
    {
        if (m_auxGeomPtr)
        {
            AZ::Vector3 points[2];
            points[0] = AZ::Vector3(p1.GetX(), p1.GetY(), z);
            points[1] = AZ::Vector3(p2.GetX(), p2.GetY(), z);
            
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
            drawArgs.m_verts = points;
            drawArgs.m_vertCount = 2;
            drawArgs.m_colors = &m_rendState.m_color;
            drawArgs.m_colorCount = 1;
            drawArgs.m_size = m_rendState.m_lineWidth;
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_auxGeomPtr->GetOrAdd2DViewProjOverride();
            m_auxGeomPtr->DrawLines(drawArgs);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawLine2dGradient(const AZ::Vector2& p1, const AZ::Vector2& p2, float z, const AZ::Vector4& firstColor, const AZ::Vector4& secondColor)
    {
        if (m_auxGeomPtr)
        {
            AZ::Vector3 points[2];
            points[0] = AZ::Vector3(p1.GetX(), p1.GetY(), z);
            points[1] = AZ::Vector3(p2.GetX(), p2.GetY(), z);
            AZ::Color colors[2] = {firstColor, secondColor};
            
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
            drawArgs.m_verts = points;
            drawArgs.m_vertCount = 2;
            drawArgs.m_colors = colors;
            drawArgs.m_colorCount = 2;
            drawArgs.m_size = m_rendState.m_lineWidth;
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_auxGeomPtr->GetOrAdd2DViewProjOverride();
            m_auxGeomPtr->DrawLines(drawArgs);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawWireCircle2d(const AZ::Vector2& center, float radius, float z)
    {
        if (m_auxGeomPtr)
        {
            // Draw axis aligned arc
            constexpr float angularStepDegrees = 10.0f;
            constexpr float startAngleDegrees = 0.0f;
            constexpr float sweepAngleDegrees = 360.0f;
            const float stepAngle = DegToRad(angularStepDegrees);
            const float startAngle = DegToRad(startAngleDegrees);
            const float stopAngle = DegToRad(sweepAngleDegrees) + startAngle;
            SingleColorDynamicSizeLineHelper lines(1+static_cast<int>(sweepAngleDegrees/angularStepDegrees));
            AZ::Vector3 radiusV3 = AZ::Vector3(radius);
            AZ::Vector3 pos = AZ::Vector3(center.GetX(), center.GetY(), z);
            CreateAxisAlignedArc(
                lines, 
                stepAngle, 
                startAngle, 
                stopAngle,
                pos,
                radiusV3, 
                CircleAxis::CircleAxisZ
            );
            lines.Draw2d(m_auxGeomPtr, m_rendState);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawArc(
        const AZ::Vector3& pos, 
        float radius, 
        float startAngleDegrees, 
        float sweepAngleDegrees, 
        float angularStepDegrees, 
        int referenceAxis)
    {
        if (m_auxGeomPtr)
        {
            // Draw axis aligned arc
            const float stepAngle = DegToRad(angularStepDegrees);
            const float startAngle = DegToRad(startAngleDegrees);
            const float stopAngle = DegToRad(sweepAngleDegrees) + startAngle;
            SingleColorDynamicSizeLineHelper lines(1+static_cast<int>(sweepAngleDegrees/angularStepDegrees));
            AZ::Vector3 radiusV3 = AZ::Vector3(radius);
            CreateAxisAlignedArc(
                lines, 
                stepAngle, 
                startAngle, 
                stopAngle,
                pos,
                radiusV3, 
                static_cast<CircleAxis>(referenceAxis)
            );
            lines.Draw(m_auxGeomPtr, m_rendState);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawArc(
        const AZ::Vector3& pos, 
        float radius, 
        float startAngleDegrees, 
        float sweepAngleDegrees, 
        float angularStepDegrees, 
        const AZ::Vector3& fixedAxis)
    {
        if (m_auxGeomPtr)
        {
            // Draw arbitraty axis arc
            const float stepAngle = DegToRad(angularStepDegrees);
            const float startAngle = DegToRad(startAngleDegrees);
            const float stopAngle = DegToRad(sweepAngleDegrees) + startAngle;
            SingleColorDynamicSizeLineHelper lines(1+static_cast<int>(sweepAngleDegrees/angularStepDegrees));
            AZ::Vector3 radiusV3 = AZ::Vector3(radius);
            CreateArbitraryAxisArc(
                lines, 
                stepAngle, 
                startAngle, 
                stopAngle,
                pos,
                radiusV3,
                fixedAxis
            );
            lines.Draw(m_auxGeomPtr, m_rendState);
        }
    }
    
    void AtomDebugDisplayViewportInterface::DrawCircle(const AZ::Vector3& pos, float radius, int nUnchangedAxis)
    {
        if (m_auxGeomPtr)
        {
            // Draw circle with default radius.
            const float step = DegToRad(10.0f);
            const float maxAngle = DegToRad(360.0f) + step;
            SingleColorStaticSizeLineHelper<40> lines; // hard code 40 lines until DegToRad is constexpr.
            AZ::Vector3 radiusV3 = AZ::Vector3(radius);
            CreateAxisAlignedArc(
                lines, 
                step, 
                0.0f, 
                maxAngle,
                pos,
                radiusV3, 
                static_cast<CircleAxis>(nUnchangedAxis));
            lines.Draw(m_auxGeomPtr, m_rendState);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawHalfDottedCircle(const AZ::Vector3& pos, float radius, const AZ::Vector3& viewPos, int nUnchangedAxis)
    {
        if (m_auxGeomPtr)
        {
            // Draw circle with single radius.
            const float step = DegToRad(10.0f);
            const float maxAngle = DegToRad(360.0f);
            SingleColorStaticSizeLineHelper<40> lines; // hard code 40 lines until DegToRad is constexpr.

            AZ::Vector3 radiusV3 = AZ::Vector3(radius);
            const AZ::Vector3 worldPos = ToWorldSpacePosition(pos);
            const AZ::Vector3 worldView = ToWorldSpacePosition(viewPos);
            const AZ::Vector3 worldDir = worldView - worldPos;

            CreateAxisAlignedArc(lines, step, 0.0f, maxAngle, pos, radiusV3, static_cast<CircleAxis>(nUnchangedAxis%CircleAxisMax), 
                                    [&worldPos, &worldDir](const AZ::Vector3& lineStart, const AZ::Vector3& lineEnd, int segmentIndex)
                                    {
                                        AZ_UNUSED(lineEnd);
                                        const float dot = (lineStart - worldPos).Dot(worldDir);
                                        const bool facing = dot > 0.0f;
                                        // if so skip every other line to produce a dotted effect
                                        if (facing || segmentIndex % 2 == 0)
                                        {
                                            return true;
                                        }
                                        return false;
                                    });
            lines.Draw(m_auxGeomPtr, m_rendState);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawWireCone(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius, float height)
    {
        if (m_auxGeomPtr)
        {
            const AZ::Vector3 worldPos = ToWorldSpacePosition(pos);
            const AZ::Vector3 worldDir = ToWorldSpaceVector(dir);
            m_auxGeomPtr->DrawCone(
                worldPos, 
                worldDir, 
                radius, 
                height, 
                m_rendState.m_color, 
                AZ::RPI::AuxGeomDraw::DrawStyle::Line,
                m_rendState.m_depthTest,
                m_rendState.m_depthWrite,
                m_rendState.m_faceCullMode,
                m_rendState.m_viewProjOverrideIndex
            );
        }
    }

    void AtomDebugDisplayViewportInterface::DrawSolidCone(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius, float height, bool drawShaded)
    {
        if (m_auxGeomPtr)
        {
            const AZ::Vector3 worldPos = ToWorldSpacePosition(pos);
            const AZ::Vector3 worldDir = ToWorldSpaceVector(dir);
            m_auxGeomPtr->DrawCone(
                worldPos, 
                worldDir, 
                radius, 
                height, 
                m_rendState.m_color, 
                drawShaded ? AZ::RPI::AuxGeomDraw::DrawStyle::Shaded : AZ::RPI::AuxGeomDraw::DrawStyle::Solid,
                m_rendState.m_depthTest,
                m_rendState.m_depthWrite,
                m_rendState.m_faceCullMode,
                m_rendState.m_viewProjOverrideIndex
            );
        }
    }
    
    void AtomDebugDisplayViewportInterface::DrawWireCylinder(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height)
    {
        if (m_auxGeomPtr)
        {
            const float scale = GetCurrentTransform().RetrieveScale().GetMaxElement();
            const AZ::Vector3 worldCenter = ToWorldSpacePosition(center);
            const AZ::Vector3 worldAxis = ToWorldSpaceVector(axis);
            m_auxGeomPtr->DrawCylinder(
                worldCenter, 
                worldAxis, 
                scale * radius, 
                scale * height, 
                m_rendState.m_color, 
                AZ::RPI::AuxGeomDraw::DrawStyle::Line,
                m_rendState.m_depthTest,
                m_rendState.m_depthWrite,
                m_rendState.m_faceCullMode,
                m_rendState.m_viewProjOverrideIndex
            );
        }
    }

    void AtomDebugDisplayViewportInterface::DrawSolidCylinder(
        const AZ::Vector3& center, 
        const AZ::Vector3& axis, 
        float radius, 
        float height, 
        bool drawShaded)
    {
        if (m_auxGeomPtr)
        {
            const float scale = GetCurrentTransform().RetrieveScale().GetMaxElement();
            const AZ::Vector3 worldCenter = ToWorldSpacePosition(center);
            const AZ::Vector3 worldAxis = ToWorldSpaceVector(axis);
            m_auxGeomPtr->DrawCylinder(
                worldCenter, 
                worldAxis, 
                scale * radius, 
                scale * height, 
                m_rendState.m_color, 
                drawShaded ? AZ::RPI::AuxGeomDraw::DrawStyle::Shaded : AZ::RPI::AuxGeomDraw::DrawStyle::Solid,
                m_rendState.m_depthTest,
                m_rendState.m_depthWrite,
                m_rendState.m_faceCullMode,
                m_rendState.m_viewProjOverrideIndex
            );
        }
    }

    void AtomDebugDisplayViewportInterface::DrawWireCapsule(
        const AZ::Vector3& center, 
        const AZ::Vector3& axis, 
        float radius, 
        float heightStraightSection)
    {
        if (m_auxGeomPtr &&  radius > FLT_EPSILON &&  axis.GetLengthSq() > FLT_EPSILON)
        {
            AZ::Vector3 axisNormalized = axis.GetNormalizedEstimate();
            SingleColorStaticSizeLineHelper<(16+1) * 5> lines; // 360/22.5 = 16, 5 possible calls to CreateArbitraryAxisArc
            AZ::Vector3 radiusV3 = AZ::Vector3(radius);
            float stepAngle = DegToRad(22.5f);
            float Deg0 = DegToRad(0.0f);


            // Draw cylinder part (or just a circle around the middle)
            if (heightStraightSection > FLT_EPSILON)
            {
                DrawWireCylinder(center, axis, radius, heightStraightSection);
            }
            else
            {
                float Deg360 = DegToRad(360.0f);
                CreateArbitraryAxisArc(
                    lines,
                    stepAngle,
                    Deg0,
                    Deg360,
                    center,
                    radiusV3,
                    axisNormalized
                    );
            }

            float Deg90 = DegToRad(90.0f);
            float Deg180 = DegToRad(180.0f);

            AZ::Vector3 ortho1Normalized, ortho2Normalized;
            CalcBasisVectors(axisNormalized, ortho1Normalized, ortho2Normalized);
            AZ::Vector3 centerToTopCircleCenter = axisNormalized * heightStraightSection * 0.5f;
            AZ::Vector3 topCenter = center + centerToTopCircleCenter;
            AZ::Vector3 bottomCenter = center - centerToTopCircleCenter;

            // Draw top cap as two criss-crossing 180deg arcs
            CreateArbitraryAxisArc(
                    lines,
                    stepAngle,
                    Deg90,
                    Deg90 + Deg180,
                    topCenter,
                    radiusV3,
                    ortho1Normalized
                    );

            CreateArbitraryAxisArc(
                    lines,
                    stepAngle,
                    Deg180,
                    Deg180 + Deg180,
                    topCenter,
                    radiusV3,
                    ortho2Normalized
                    );

            // Draw bottom cap
            CreateArbitraryAxisArc(
                    lines,
                    stepAngle,
                    -Deg90,
                    -Deg90 + Deg180,
                    bottomCenter,
                    radiusV3,
                    ortho1Normalized
                    );

            CreateArbitraryAxisArc(
                    lines,
                    stepAngle,
                    Deg0,
                    Deg0 + Deg180,
                    bottomCenter,
                    radiusV3,
                    ortho2Normalized
                    );

            lines.Draw(m_auxGeomPtr, m_rendState);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawWireSphere(const AZ::Vector3& pos, float radius)
    {
        if (m_auxGeomPtr)
        {
            const float scale = GetCurrentTransform().RetrieveScale().GetMaxElement();
            m_auxGeomPtr->DrawSphere(
                ToWorldSpacePosition(pos), 
                scale * radius, 
                m_rendState.m_color, 
                AZ::RPI::AuxGeomDraw::DrawStyle::Line,
                m_rendState.m_depthTest,
                m_rendState.m_depthWrite,
                m_rendState.m_faceCullMode,
                m_rendState.m_viewProjOverrideIndex
            );
        }
    }

    void AtomDebugDisplayViewportInterface::DrawWireSphere(const AZ::Vector3& pos, const AZ::Vector3 radius)
    {
        if (m_auxGeomPtr)
        {
            // This matches Cry behavior, the DrawWireSphere above may need modifying to use the same approach.
            // Draw 3 axis aligned circles
            const float step = DegToRad(10.0f);
            const float maxAngle = DegToRad(360.0f);
            SingleColorStaticSizeLineHelper<40*3> lines; // hard code to 40 lines * 3 circles until DegToRad is constexpr.

            // Z Axis
            AZ::Vector3 axisRadius(radius.GetX(), radius.GetY(), 0.0f);
            CreateAxisAlignedArc(lines, step, 0.0f, maxAngle, pos, axisRadius, CircleAxisZ);

            // X Axis
            axisRadius = AZ::Vector3(0.0f, radius.GetY(), radius.GetZ());
            CreateAxisAlignedArc(lines, step, 0.0f, maxAngle, pos, axisRadius, CircleAxisX);

            // Y Axis
            axisRadius = AZ::Vector3(radius.GetX(), 0.0f, radius.GetZ());
            CreateAxisAlignedArc(lines, step, 0.0f, maxAngle, pos, axisRadius, CircleAxisY);
            lines.Draw(m_auxGeomPtr, m_rendState);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawWireDisk(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius)
    {
        if (m_auxGeomPtr)
        {

            // Draw 3 axis aligned circles
            const float stepAngle  = DegToRad(11.25f);
            const float startAngle = DegToRad(0.0f);
            const float stopAngle = DegToRad(360.0f);
            SingleColorDynamicSizeLineHelper lines(2 + static_cast<int>(360.0f / 11.25f)); // num disk segments + 1 for axis line + 1 for spare
            const AZ::Vector3 radiusV3 = AZ::Vector3(radius);
            CreateArbitraryAxisArc(
                lines, 
                stepAngle, 
                startAngle, 
                stopAngle,
                pos,
                radiusV3,
                dir
            );

            lines.AddLineSegment(ToWorldSpacePosition(pos), ToWorldSpacePosition(pos + dir * (radius * 0.2f))); // 0.2f comes from Code\Editor\Objects\DisplayContextShared.inl DisplayContext::DrawWireDisk
            lines.Draw(m_auxGeomPtr, m_rendState);
        }
    }

    void AtomDebugDisplayViewportInterface::DrawBall(const AZ::Vector3& pos, float radius, bool drawShaded)
    {
        if (m_auxGeomPtr)
        {
            // get the max scaled radius in case the transform on the stack is scaled non-uniformly
            const float transformedRadiusX = ToWorldSpaceVector(AZ::Vector3(radius, 0.0f, 0.0f)).GetLengthEstimate();
            const float transformedRadiusY = ToWorldSpaceVector(AZ::Vector3(0.0f, radius, 0.0f)).GetLengthEstimate();
            const float transformedRadiusZ = ToWorldSpaceVector(AZ::Vector3(0.0f, 0.0f, radius)).GetLengthEstimate();
            const float maxTransformedRadius =
                AZ::GetMax(transformedRadiusX, AZ::GetMax(transformedRadiusY, transformedRadiusZ));

            AZ::RPI::AuxGeomDraw::DrawStyle drawStyle = drawShaded ? AZ::RPI::AuxGeomDraw::DrawStyle::Shaded : AZ::RPI::AuxGeomDraw::DrawStyle::Solid;
            m_auxGeomPtr->DrawSphere(
                ToWorldSpacePosition(pos), 
                maxTransformedRadius, 
                m_rendState.m_color, 
                drawStyle,
                m_rendState.m_depthTest,
                m_rendState.m_depthWrite,
                m_rendState.m_faceCullMode,
                m_rendState.m_viewProjOverrideIndex
                );
        }
    }

    void AtomDebugDisplayViewportInterface::DrawDisk(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius)
    {
        if (m_auxGeomPtr)
        {
            const float scale = GetCurrentTransform().RetrieveScale().GetMaxElement();
            const AZ::Vector3 worldPos = ToWorldSpacePosition(pos);
            const AZ::Vector3 worldDir = ToWorldSpaceVector(dir);
            m_auxGeomPtr->DrawDisk(
                worldPos, 
                worldDir, 
                scale * radius, 
                m_rendState.m_color,
                AZ::RPI::AuxGeomDraw::DrawStyle::Shaded,
                m_rendState.m_depthTest,
                m_rendState.m_depthWrite,
                m_rendState.m_faceCullMode,
                m_rendState.m_viewProjOverrideIndex
                );
        }
    }

    void AtomDebugDisplayViewportInterface::DrawArrow(const AZ::Vector3& src, const AZ::Vector3& trg, float headScale, bool dualEndedArrow)
    {
        if (m_auxGeomPtr)
        {
            float f2dScale = 1.0f;
            float arrowLen = 0.4f * headScale;
            float arrowRadius = 0.1f * headScale;
            // if (flags & DISPLAY_2D)
            // {
            //     f2dScale = 1.2f * ToWorldSpaceVector(Vec3(1, 0, 0)).GetLength();
            // }
            AZ::Vector3 dir = trg - src;
            dir = ToWorldSpaceVector(dir.GetNormalized());
            AZ::Vector3 verts[2] = {ToWorldSpacePosition(src), ToWorldSpacePosition(trg)};
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
            drawArgs.m_verts = verts;
            drawArgs.m_vertCount = 2;
            drawArgs.m_colors = &m_rendState.m_color;
            drawArgs.m_colorCount = 1;
            drawArgs.m_size = m_rendState.m_lineWidth;
            drawArgs.m_opacityType = m_rendState.m_opacityType;
            drawArgs.m_depthTest = m_rendState.m_depthTest;
            drawArgs.m_depthWrite = m_rendState.m_depthWrite;
            drawArgs.m_viewProjectionOverrideIndex = m_rendState.m_viewProjOverrideIndex;
            if (!dualEndedArrow)
            {
                verts[1] -= dir * arrowLen;
                m_auxGeomPtr->DrawLines(drawArgs);
                m_auxGeomPtr->DrawCone(
                    verts[1], 
                    dir, 
                    arrowRadius * f2dScale, 
                    arrowLen * f2dScale, 
                    m_rendState.m_color,
                    AZ::RPI::AuxGeomDraw::DrawStyle::Shaded,
                    m_rendState.m_depthTest,
                    m_rendState.m_depthWrite,
                    m_rendState.m_faceCullMode,
                    m_rendState.m_viewProjOverrideIndex
                    );
            }
            else
            {
                verts[0] += dir * arrowLen;
                verts[1] -= dir * arrowLen;
                m_auxGeomPtr->DrawLines(drawArgs);
                m_auxGeomPtr->DrawCone(
                    verts[0], 
                    -dir, 
                    arrowRadius * f2dScale, 
                    arrowLen * f2dScale, 
                    m_rendState.m_color,
                    AZ::RPI::AuxGeomDraw::DrawStyle::Shaded,
                    m_rendState.m_depthTest,
                    m_rendState.m_depthWrite,
                    m_rendState.m_faceCullMode,
                    m_rendState.m_viewProjOverrideIndex
                    );
                m_auxGeomPtr->DrawCone(
                    verts[1], 
                    dir, 
                    arrowRadius * f2dScale, 
                    arrowLen * f2dScale, 
                    m_rendState.m_color,
                    AZ::RPI::AuxGeomDraw::DrawStyle::Shaded,
                    m_rendState.m_depthTest,
                    m_rendState.m_depthWrite,
                    m_rendState.m_faceCullMode,
                    m_rendState.m_viewProjOverrideIndex
                    );
            }
        }
    }

    void AtomDebugDisplayViewportInterface::DrawTextLabel(
        const AZ::Vector3& pos, 
        float size, 
        const char* text, 
        const bool center, 
        int srcOffsetX [[maybe_unused]], 
        int srcOffsetY [[maybe_unused]])
    {
        // abort draw if draw is invalid or font query interface is missing.
        if (!text || size == 0.0f || !AZ::Interface<AzFramework::FontQueryInterface>::Get())
        {
            return;
        }

        AzFramework::FontDrawInterface* fontDrawInterface = AZ::Interface<AzFramework::FontQueryInterface>::Get()->GetDefaultFontDrawInterface();
        // abort draw if font draw interface is missing
        if (!fontDrawInterface)
        {
            return;
        }
        // if 2d draw need to project pos to screen first
        AzFramework::TextDrawParameters params;
        AZ::RPI::ViewportContextPtr viewportContext = GetViewportContext();
        params.m_drawViewportId = viewportContext->GetId(); // get the viewport ID so default viewport works
        params.m_position = pos;
        params.m_color = m_rendState.m_color;
        params.m_scale = AZ::Vector2(size);
        params.m_hAlign = center ? AzFramework::TextHorizontalAlignment::Center : AzFramework::TextHorizontalAlignment::Left; //! Horizontal text alignment
        params.m_monospace = false; //! disable character proportional spacing
        params.m_depthTest = false; //! Test character against the depth buffer
        params.m_virtual800x600ScreenSize = false; //! Text placement and size are scaled in viewport pixel coordinates
        params.m_scaleWithWindow = false; //! Font gets bigger as the window gets bigger
        params.m_multiline = true; //! text respects ascii newline characters

        fontDrawInterface->DrawScreenAlignedText3d(params, text);
    }

    void AtomDebugDisplayViewportInterface::Draw2dTextLabel(
        float x, 
        float y, 
        float size, 
        const char* text, 
        bool center)
    {
        // abort draw if draw is invalid or font query interface is missing.
        if (!text || size == 0.0f || !AZ::Interface<AzFramework::FontQueryInterface>::Get())
        {
            return;
        }

        AzFramework::FontDrawInterface* fontDrawInterface = AZ::Interface<AzFramework::FontQueryInterface>::Get()->GetDefaultFontDrawInterface();
        // abort draw if font draw interface is missing
        if (!fontDrawInterface)
        {
            return;
        }
        // if 2d draw need to project pos to screen first
        AzFramework::TextDrawParameters params;
        AZ::RPI::ViewportContextPtr viewportContext = GetViewportContext();
        const auto dpiScaleFactor = viewportContext->GetDpiScalingFactor();
        params.m_drawViewportId = viewportContext->GetId(); // get the viewport ID so default viewport works
        params.m_position = AZ::Vector3(x * dpiScaleFactor, y * dpiScaleFactor, 1.0f);
        params.m_color = m_rendState.m_color;
        params.m_scale = AZ::Vector2(size);
        params.m_hAlign = center ? AzFramework::TextHorizontalAlignment::Center : AzFramework::TextHorizontalAlignment::Left; //! Horizontal text alignment
        params.m_monospace = false; //! disable character proportional spacing
        params.m_depthTest = false; //! Test character against the depth buffer
        params.m_virtual800x600ScreenSize = false; //! Text placement and size are scaled in viewport pixel coordinates
        params.m_scaleWithWindow = false; //! Font gets bigger as the window gets bigger
        params.m_multiline = true; //! text respects ascii newline characters

        fontDrawInterface->DrawScreenAlignedText2d(params, text);
    }

    void AtomDebugDisplayViewportInterface::DrawTextOn2DBox(
        const AZ::Vector3& pos [[maybe_unused]],
        const char* text [[maybe_unused]],
        float textScale [[maybe_unused]],
        const AZ::Vector4& TextColor [[maybe_unused]],
        const AZ::Vector4& TextBackColor [[maybe_unused]])
    {
        AZ_Assert(false, "Unexpected use of legacy api, please file a feature request with the rendering team to get this implemented!");
    }

    void AtomDebugDisplayViewportInterface::SetLineWidth(float width)
    {
        AZ_Assert(width >= 0.0f && width <= 255.0f, "Width (%f) exceeds allowable range [0 - 255]", width);
        m_rendState.m_lineWidth = static_cast<AZ::u8>(width);
    }

    bool AtomDebugDisplayViewportInterface::IsVisible(const AZ::Aabb& bounds)
    {
        AZ::RPI::ViewportContextPtr viewportContext = GetViewportContext();
        const AZ::Matrix4x4& worldToClip = viewportContext->GetDefaultView()->GetWorldToClipMatrix();
        AZ::Frustum frustum = AZ::Frustum::CreateFromMatrixColumnMajor(worldToClip, Frustum::ReverseDepth::True);
        return frustum.IntersectAabb(bounds) != AZ::IntersectResult::Exterior;
    }
    // int AtomDebugDisplayViewportInterface::SetFillMode(int nFillMode) override;
    float AtomDebugDisplayViewportInterface::GetLineWidth()
    { 
        return m_rendState.m_lineWidth;
    }

    float AtomDebugDisplayViewportInterface::GetAspectRatio()
    {
        AZ::RPI::ViewportContextPtr viewportContext = GetViewportContext();
        auto windowSize = viewportContext->GetViewportSize();
        return aznumeric_cast<float>(windowSize.m_width)/aznumeric_cast<float>(windowSize.m_height);
    }

    void AtomDebugDisplayViewportInterface::DepthTestOff()
    {
        m_rendState.m_depthTest = AZ::RPI::AuxGeomDraw::DepthTest::Off;
    }

    void AtomDebugDisplayViewportInterface::DepthTestOn()
    {
        m_rendState.m_depthTest = AZ::RPI::AuxGeomDraw::DepthTest::On;
    }
    
    void AtomDebugDisplayViewportInterface::DepthWriteOff()
    {
        m_rendState.m_depthWrite = AZ::RPI::AuxGeomDraw::DepthWrite::Off;
    }
    
    void AtomDebugDisplayViewportInterface::DepthWriteOn()
    {
        m_rendState.m_depthWrite = AZ::RPI::AuxGeomDraw::DepthWrite::On;
    }
    
    void AtomDebugDisplayViewportInterface::CullOff()
    {
        m_rendState.m_faceCullMode = AZ::RPI::AuxGeomDraw::FaceCullMode::None;
    }
    
    void AtomDebugDisplayViewportInterface::CullOn()
    {
        m_rendState.m_faceCullMode = AZ::RPI::AuxGeomDraw::FaceCullMode::Back;
    }
    
    bool AtomDebugDisplayViewportInterface::SetDrawInFrontMode(bool on)
    {
        AZ_UNUSED(on);
        return false;
    }
    
    AZ::u32 AtomDebugDisplayViewportInterface::GetState()
    {
        return ConvertRenderStateToCry();
    }

    AZ::u32 AtomDebugDisplayViewportInterface::SetState(AZ::u32 state)
    {
        uint32_t currentState = ConvertRenderStateToCry();
        uint32_t changedState = (state & e_PublicParamsMask) ^ currentState;

        if (changedState & e_Mode2D3DMask)
        {
            // this is the only way to turn on 2d Mode under Atom
            if (state & e_Mode2D)
            {
                AZ_Assert((currentState & e_DrawInFrontOn) == 0 && (changedState & e_DrawInFrontOn) == 0, "Atom doesnt support Draw In Front and 2d at the same time");
                m_rendState.m_viewProjOverrideIndex = m_auxGeomPtr->GetOrAdd2DViewProjOverride();
                m_rendState.m_2dMode = true;
            }
            else // switch back to mode 3d
            {
                m_rendState.m_viewProjOverrideIndex = -1;
                m_rendState.m_2dMode = false;
            }
        }

        if (changedState & e_AlphaBlendingMask)
        {
            switch (state&e_AlphaBlendingMask)
            {
                case e_AlphaNone:
                    m_rendState.m_opacityType = AZ::RPI::AuxGeomDraw::OpacityType::Opaque;
                    break;
                case e_AlphaAdditive:
                    [[fallthrough]]; // Additive not currently supported in Atom AuxGeom implementation
                case e_AlphaBlended:
                    m_rendState.m_opacityType = AZ::RPI::AuxGeomDraw::OpacityType::Translucent;
                    break;
            }
        }

        if (changedState & e_DrawInFrontMask)
        {
            AZ_Assert( // either state is turning DrawInFront off or Mode 2D has to be off
                (state & e_DrawInFrontOn) == 0 ||
                ((currentState & e_Mode2D) == 0 && (changedState & e_Mode2D) == 0), 
                "Atom doesnt support Draw In Front and 2d at the same time");
            SetDrawInFrontMode(changedState & e_DrawInFrontOn);
        }

        if (changedState & e_CullModeMask)
        {
            switch (state & e_CullModeMask)
            {
                case e_CullModeNone:
                    CullOff();
                    break;
                case e_CullModeFront:
                    // Currently no other way to set front face culling in DebugDisplayRequestBus
                    m_rendState.m_faceCullMode = AZ::RPI::AuxGeomDraw::FaceCullMode::Front;
                    break;
                case e_CullModeBack:
                    CullOn();
                    break;
            }
        }

        if (changedState & e_DepthWriteMask)
        {
            if (state & e_DepthWriteOff)
            {
                DepthWriteOff();
            }
            else
            {
                DepthWriteOn();
            }
        }

        if (changedState & e_DepthTestMask)
        {
            if (state & e_DepthTestOff)
            {
                DepthTestOff();
            }
            else
            {
                DepthTestOn();
            }
        }

        return currentState;
    }

    void AtomDebugDisplayViewportInterface::PushMatrix(const AZ::Transform& tm)
    {
        AZ_Assert(m_rendState.m_currentTransform < RenderState::TransformStackSize, "Exceeded AtomDebugDisplayViewportInterface matrix stack size");
        if (m_rendState.m_currentTransform < RenderState::TransformStackSize)
        {
            m_rendState.m_currentTransform++;
            m_rendState.m_transformStack[m_rendState.m_currentTransform] = m_rendState.m_transformStack[m_rendState.m_currentTransform - 1] * AZ::Matrix3x4::CreateFromTransform(tm);
        }
    }

    void AtomDebugDisplayViewportInterface::PopMatrix()
    {
        AZ_Assert(m_rendState.m_currentTransform > 0, "Underflowed AtomDebugDisplayViewportInterface matrix stack");
        if (m_rendState.m_currentTransform > 0)
        {
            m_rendState.m_currentTransform--;
        }
    }

    void AtomDebugDisplayViewportInterface::PushPremultipliedMatrix(const AZ::Matrix3x4& matrix)
    {
        AZ_Assert(m_rendState.m_currentTransform < RenderState::TransformStackSize, "Exceeded AtomDebugDisplayViewportInterface matrix stack size");
        if (m_rendState.m_currentTransform < RenderState::TransformStackSize)
        {
            m_rendState.m_currentTransform++;
            m_rendState.m_transformStack[m_rendState.m_currentTransform] = matrix;
        }
    }

    AZ::Matrix3x4 AtomDebugDisplayViewportInterface::PopPremultipliedMatrix()
    {
        AZ_Assert(m_rendState.m_currentTransform > 0, "Underflowed AtomDebugDisplayViewportInterface matrix stack");
        if (m_rendState.m_currentTransform > 0)
        {
            m_rendState.m_currentTransform--;
        }
        return m_rendState.m_transformStack[m_rendState.m_currentTransform + 1];
    }

    const AZ::Matrix3x4& AtomDebugDisplayViewportInterface::GetCurrentTransform() const
    {
        return m_rendState.m_transformStack[m_rendState.m_currentTransform];
    }

    AZStd::vector<AZ::Vector3> AtomDebugDisplayViewportInterface::ToWorldSpacePosition(const AZStd::vector<AZ::Vector3>& positions) const
    {
        AZStd::vector<AZ::Vector3> transformedPositions;
        transformedPositions.resize_no_construct(positions.size());
        AZStd::transform(positions.begin(), positions.end(), transformedPositions.begin(), [this](const AZ::Vector3& position) {
            return ToWorldSpacePosition(position);
        });
        return transformedPositions;
    }

    AZStd::vector<AZ::Vector3> AtomDebugDisplayViewportInterface::ToWorldSpaceVector(const AZStd::vector<AZ::Vector3>& vectors) const
    {
        AZStd::vector<AZ::Vector3> transformedVectors;
        transformedVectors.resize_no_construct(vectors.size());
        AZStd::transform(vectors.begin(), vectors.end(), transformedVectors.begin(), [this](const AZ::Vector3& vector) {
            return ToWorldSpaceVector(vector);
        });
        return transformedVectors;
    }

    AZ::RPI::ViewportContextPtr AtomDebugDisplayViewportInterface::GetViewportContext() const
    {
        auto viewContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (m_defaultInstance)
        {
            return viewContextManager->GetViewportContextByName(viewContextManager->GetDefaultViewportContextName());
        }
        else
        {
            return viewContextManager->GetViewportContextById(m_viewportId);
        }
    }

    uint32_t AtomDebugDisplayViewportInterface::ConvertRenderStateToCry() const
    {
        uint32_t result = 0;

        result |= m_rendState.m_2dMode ? e_Mode2D : e_Mode3D;
        result |= m_rendState.m_opacityType == AZ::RPI::AuxGeomDraw::OpacityType::Opaque ? e_AlphaNone : e_AlphaBlended;
        result |= m_rendState.m_drawInFront ? e_DrawInFrontOn : e_DrawInFrontOff;
        result |= m_rendState.m_depthTest == AZ::RPI::AuxGeomDraw::DepthTest::On ? e_DepthTestOn : e_DepthTestOff;
        result |= m_rendState.m_depthWrite == AZ::RPI::AuxGeomDraw::DepthWrite::On ? e_DepthWriteOn : e_DepthWriteOff;
        switch (m_rendState.m_faceCullMode)
        {
            case AZ::RPI::AuxGeomDraw::FaceCullMode::None:
                result |= e_CullModeNone;
                break;
            case AZ::RPI::AuxGeomDraw::FaceCullMode::Front:
                result |= e_CullModeFront;
                break;
            case AZ::RPI::AuxGeomDraw::FaceCullMode::Back:
                result |= e_CullModeBack;
                break;
            default:
                AZ_Assert(false, "Trying to convert an unknown culling mode to cry!");
                break;
        }

        return result;
    }
}
