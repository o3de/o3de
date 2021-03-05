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

#include <IRenderer.h>
#include <IFont.h>
#include <LyShine/IRenderGraph.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/Math/Color.h>

#ifndef _RELEASE
#include "LyShineDebug.h"
#endif

class UiRenderer;

namespace LyShine
{
    enum RenderNodeType
    {
        PrimitiveList,
        Mask,
        RenderTarget
    };

    enum class AlphaMaskType
    {
        None,
        ModulateAlpha,
        ModulateAlphaAndColor
    };

    // Abstract base class for nodes in the render graph
    class RenderNode
    {
    public: // functions
        RenderNode(RenderNodeType type) : m_type(type) {}
        virtual ~RenderNode() {};

        virtual void Render(UiRenderer* uiRenderer) = 0;

        RenderNodeType GetType() const { return m_type; }

#ifndef _RELEASE
        // A debug-only function useful for debugging
        virtual void ValidateNode() = 0;
#endif

    private: // data
        RenderNodeType m_type;
    };

    // As we build the render graph we allocate a render node for each change in render state
    class PrimitiveListRenderNode : public RenderNode
    {
    public: // functions
        // We use a pool allocator to keep these allocations fast.
        AZ_CLASS_ALLOCATOR(PrimitiveListRenderNode, AZ::PoolAllocator, 0);

        PrimitiveListRenderNode(ITexture* texture, bool isClampTextureMode, bool isTextureSRGB, bool preMultiplyAlpha, int blendModeState);
        PrimitiveListRenderNode(ITexture* texture, ITexture* maskTexture,
            bool isClampTextureMode, bool isTextureSRGB, bool preMultiplyAlpha, AlphaMaskType alphaMaskType, int blendModeState);
        ~PrimitiveListRenderNode() override;
        void Render(UiRenderer* uiRenderer) override;

        void AddPrimitive(IRenderer::DynUiPrimitive* primitive);
        IRenderer::DynUiPrimitiveList& GetPrimitives() const;

        int GetOrAddTexture(ITexture* texture, bool isClampTextureMode);
        int GetNumTextures() const { return m_numTextures; }
        ITexture* GetTexture(int texIndex) const { return m_textures[texIndex].m_texture; }
        bool GetTextureIsClampMode(int texIndex) const { return m_textures[texIndex].m_isClampTextureMode; }

        bool GetIsTextureSRGB() const { return m_isTextureSRGB; }
        int  GetBlendModeState() const { return m_blendModeState; }
        bool GetIsPremultiplyAlpha() const { return m_preMultiplyAlpha; }
        AlphaMaskType GetAlphaMaskType() const { return m_alphaMaskType; }

        bool HasSpaceToAddPrimitive(IRenderer::DynUiPrimitive* primitive) const;

        // Search to see if this texture is already used by this texture unit, returns -1 if not used
        int FindTexture(ITexture* texture, bool isClampTextureMode) const;

#ifndef _RELEASE
        // A debug-only function useful for debugging
        void ValidateNode() override;
#endif

    public: // data
        static const int MaxTextures = 16;

    private: // types
        struct TextureUsage
        {
            ITexture*   m_texture;
            bool        m_isClampTextureMode;
        };

    private: // data
        TextureUsage    m_textures[MaxTextures];
        int             m_numTextures;
        bool            m_isTextureSRGB;
        bool            m_preMultiplyAlpha;
        AlphaMaskType   m_alphaMaskType;
        int             m_blendModeState;
        int             m_totalNumVertices;
        int             m_totalNumIndices;

        IRenderer::DynUiPrimitiveList   m_primitives;
    };

    // A mask render node handles using one set of render nodes to mask another set of render nodes
    class MaskRenderNode : public RenderNode
    {
    public: // functions
        // We use a pool allocator to keep these allocations fast.
        AZ_CLASS_ALLOCATOR(MaskRenderNode, AZ::PoolAllocator, 0);

        MaskRenderNode(MaskRenderNode* parentMask, bool isMaskingEnabled, bool useAlphaTest, bool drawBehind, bool drawInFront);
        ~MaskRenderNode() override;

        void Render(UiRenderer* uiRenderer) override;

        AZStd::vector<RenderNode*>& GetMaskRenderNodeList() { return m_maskRenderNodes; }
        const AZStd::vector<RenderNode*>& GetMaskRenderNodeList() const { return m_maskRenderNodes; }

        AZStd::vector<RenderNode*>& GetContentRenderNodeList() { return m_contentRenderNodes; }
        const AZStd::vector<RenderNode*>& GetContentRenderNodeList() const { return m_contentRenderNodes; }

        MaskRenderNode* GetParentMask() { return m_parentMask; }

        //! if the mask has no content elements and is not drawing the mask primitives then there is no need to add a render node
        bool IsMaskRedundant();

        bool GetIsMaskingEnabled() const { return m_isMaskingEnabled; }
        bool GetUseAlphaTest() const { return m_useAlphaTest; }
        bool GetDrawBehind() const { return m_drawBehind; }
        bool GetDrawInFront() const { return m_drawInFront; }

#ifndef _RELEASE
        // A debug-only function useful for debugging
        void ValidateNode() override;
#endif

    private: // functions
        void SetupBeforeRenderingMask(UiRenderer* uiRenderer, bool firstPass, int priorBaseState);
        void SetupAfterRenderingMask(UiRenderer* uiRenderer, bool firstPass, int priorBaseState);

    private: // data
        AZStd::vector<RenderNode*>  m_maskRenderNodes;      //!< The render nodes used to render the mask shape
        AZStd::vector<RenderNode*>  m_contentRenderNodes;   //!< The render nodes that are masked by this mask

        MaskRenderNode* m_parentMask = nullptr;             //! Used while building the render graph.

        // flags that control the render behavior of the mask
        bool m_isMaskingEnabled = true;
        bool m_useAlphaTest = false;
        bool m_drawBehind = false;
        bool m_drawInFront = false;
    };

    // A render target render node renders its child render nodes to a given render target
    class RenderTargetRenderNode : public RenderNode
    {
    public: // functions
        // We use a pool allocator to keep these allocations fast.
        AZ_CLASS_ALLOCATOR(RenderTargetRenderNode, AZ::PoolAllocator, 0);

        RenderTargetRenderNode(RenderTargetRenderNode* parentRenderTarget, int renderTargetHandle,
            SDepthTexture* renderTargetDepthSurface,
            const AZ::Vector2& viewportTopLeft,
            const AZ::Vector2& viewportSize,
            const AZ::Color& clearColor,
            int nestLevel);
        ~RenderTargetRenderNode() override;

        void Render(UiRenderer* uiRenderer) override;

        AZStd::vector<RenderNode*>& GetChildRenderNodeList() { return m_childRenderNodes; }
        const AZStd::vector<RenderNode*>& GetChildRenderNodeList() const { return m_childRenderNodes; }

        RenderTargetRenderNode* GetParentRenderTarget() { return m_parentRenderTarget; }

        float GetViewportX() const { return m_viewportX; }
        float GetViewportY() const { return m_viewportY; }
        float GetViewportWidth() const { return m_viewportWidth; }
        float GetViewportHeight() const { return m_viewportHeight; }
        AZ::Color GetClearColor() const { return m_clearColor; }

        const char* GetRenderTargetName() const;

#ifndef _RELEASE
        // A debug-only function useful for debugging
        void ValidateNode() override;
#endif

        //! Used to sort a list of RenderTargetNodes for render order
        static bool CompareNestLevelForSort(RenderTargetRenderNode* a, RenderTargetRenderNode* b);

    private: // functions

    private: // data
        AZStd::vector<RenderNode*>  m_childRenderNodes;      //!< The render nodes to render to the render target

        RenderTargetRenderNode* m_parentRenderTarget = nullptr;             //! Used while building the render graph.

        int m_renderTargetHandle = -1;
        SDepthTexture* m_renderTargetDepthSurface = nullptr;

        float       m_viewportX = 0;
        float       m_viewportY = 0;
        float       m_viewportWidth = 0;
        float       m_viewportHeight = 0;
        AZ::Color   m_clearColor;
        int         m_nestLevel = 0;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // The RenderGraph is owned by the canvas component
    class RenderGraph : public IRenderGraph
    {
    public:

        RenderGraph();
        ~RenderGraph() override;

        //! Free up all the memory and clear the lists
        void ResetGraph();
        
        // IRenderGraph
        void BeginMask(bool isMaskingEnabled, bool useAlphaTest, bool drawBehind, bool drawInFront) override;
        void StartChildrenForMask() override;
        void EndMask() override;

        void BeginRenderToTexture(int renderTargetHandle, SDepthTexture* renderTargetDepthSurface,
            const AZ::Vector2& viewportTopLeft, const AZ::Vector2& viewportSize,
            const AZ::Color& clearColor) override;
        void EndRenderToTexture() override;

        void AddPrimitive(IRenderer::DynUiPrimitive* primitive, ITexture* texture,
            bool isClampTextureMode, bool isTextureSRGB, bool isTexturePremultipliedAlpha, BlendMode blendMode) override;

        void AddAlphaMaskPrimitive(IRenderer::DynUiPrimitive* primitive,
            ITexture* texture, ITexture* maskTexture,
            bool isClampTextureMode, bool isTextureSRGB, bool isTexturePremultipliedAlpha, BlendMode blendMode) override;

        IRenderer::DynUiPrimitive* GetDynamicQuadPrimitive(const AZ::Vector2* positions, uint32 packedColor) override;

        bool IsRenderingToMask() const override;
        void SetIsRenderingToMask(bool isRenderingToMask) override;

        void PushAlphaFade(float alphaFadeValue) override;
        void PushOverrideAlphaFade(float alphaFadeValue) override;
        void PopAlphaFade() override;
        float GetAlphaFade() const override;
        // ~IRenderGraph
    
        //! Render the display graph
        void Render(UiRenderer* uiRenderer, const AZ::Vector2& viewportSize);

        //! Set the dirty flag - this also resets the graph
        void SetDirtyFlag(bool isDirty);

        //! Get the dirty flag
        bool GetDirtyFlag();

        //! End the building of the graph
        void FinalizeGraph();

        //! Test whether the render graph contains any render nodes
        bool IsEmpty();

#ifndef _RELEASE
        // A debug-only function useful for debugging, not called but calls can be added during debugging
        void ValidateGraph();

        void GetDebugInfoRenderGraph(LyShineDebug::DebugInfoRenderGraph& info) const;
        void GetDebugInfoRenderNodeList(const AZStd::vector<RenderNode*>& renderNodeList, LyShineDebug::DebugInfoRenderGraph& info, AZStd::set<ITexture*>& uniqueTextures) const;

        void DebugReportDrawCalls(AZ::IO::HandleType fileHandle, LyShineDebug::DebugInfoDrawCallReport& reportInfo, void* context) const;
        void DebugReportDrawCallsRenderNodeList(const AZStd::vector<RenderNode*>& renderNodeList, AZ::IO::HandleType fileHandle,
            LyShineDebug::DebugInfoDrawCallReport& reportInfo, void* context, const AZStd::string& indent) const;
#endif

    protected: // types

        struct DynamicQuad
        {
            SVF_P2F_C4B_T2F_F4B         m_quadVerts[4];
            IRenderer::DynUiPrimitive   m_primitive;
        };

    protected: // member functions

        //! Given a blend mode and whether the shader will be outputing premultiplied alpha, return state flags
        int GetBlendModeState(LyShine::BlendMode blendMode, bool isShaderOutputPremultAlpha) const;

    protected:  // data

        AZStd::vector<RenderNode*>  m_renderNodes;
        AZStd::vector<DynamicQuad*> m_dynamicQuads; // used for drawing quads not cached in components

        MaskRenderNode*             m_currentMask = nullptr;
        RenderTargetRenderNode*     m_currentRenderTarget = nullptr;

        AZStd::stack<AZStd::vector<RenderNode*>*> m_renderNodeListStack;

        bool                        m_isDirty = true;
        int                         m_renderToRenderTargetCount = 0;

        bool                        m_isRenderingToMask = false;
        AZStd::stack<float>         m_alphaFadeStack;

        AZStd::vector<RenderTargetRenderNode*>  m_renderTargetRenderNodes;
        int                         m_renderTargetNestLevel = 0;

#ifndef _RELEASE
        // A debug-only variable used to track whether the rendergraph was rebuilt this frame
        mutable bool                m_wasBuiltThisFrame = false;
        AZ::u64                     m_timeGraphLastBuiltMs = 0;
#endif
    };
}
