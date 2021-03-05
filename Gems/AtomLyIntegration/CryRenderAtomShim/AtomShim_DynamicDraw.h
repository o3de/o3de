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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERATOMSHIM_ATOMSHIM_DYNAMICDRAW_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERATOMSHIM_ATOMSHIM_DYNAMICDRAW_H
#pragma once

#include <AzCore/Math/Matrix4x4.h>

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Scene.h>

#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/DrawList.h>

namespace AZ
{
    namespace RHI
    {
        class Image;
        class ImageView;
    }

    namespace RPI
    {
        class StreamingImage;
    }
}

class CAtomShimDynamicDraw
{
public:
    static const AZ::u32 MaxVerts = 256 * 1024;
    static const AZ::u32 MaxIndices = 256 * 1024;
    static constexpr const char* DrawList2DPass = "2dpass";

    CAtomShimDynamicDraw();
    virtual ~CAtomShimDynamicDraw() = default;

    void BeginFrame();

    void EndFrame();

    //! Create an Atom image given the source pixels and store a pointer to it in the given AtomShimTexture.
    //! Returns false if it fails to create the image.
    bool CreateFontImage(AtomShimTexture* texture, AZ::s32 width, AZ::s32 height, AZ::u8* pData, ETEX_Format format, bool genMips, const char* textureName);

    bool UpdateFontImage(AZ::RHI::Ptr<AZ::RHI::Image> image, int x, int y, int uSize, int vSize, AZ::u8* pData);
    void SetCurrentViewProj(AZ::Matrix4x4 viewProj) { m_currentViewProj = viewProj; }

    void Set2DMode(bool set2DMode) { m_2DMode = set2DMode; }

    //! Add a draw packet for when drawing a font.
    //! Fonts are currently treated specially because the font textures are not AtomShim textures but stored separately
    void AddFontDraw(const AZ::RHI::ImageView* imageView, AZ::RPI::Scene* scene,
        SVF_P3F_C4B_T2F* pBuf, uint16* pInds, int nVerts, int nInds, const PublicRenderPrimitiveType nPrimType);

    //! Add a draw packet for a given vertex buffer (used to implement DrawDynVB) for Atom using the DynamicDraw feature processor
    void AddSimpleTexturedDraw(const AZ::RHI::ImageView* imageView, AZ::RPI::Scene* scene,
        SVF_P3F_C4B_T2F* pBuf, uint16* pInds, int nVerts, int nInds,
        const PublicRenderPrimitiveType nPrimType, bool useClamp);

    //! Add a draw packet for a set of LyShine UI primitives using up to 16 textures
    void AddUiDraw(AZ::RPI::Scene* scene,
        IRenderer::DynUiPrimitiveList & primitives, int totalNumVertices, int totalNumIndices,
        const PublicRenderPrimitiveType nPrimType,
        AtomShimTexture* currentTextureForUnit[], bool clampFlagPerTextureUnit[]);

    //! Set the shader variant options for the UI shader (using flags from the IRenderer SetColorOp).
    //! See LyShine's RenderGraph.cpp for the enums used to set these parameters for LyShine.
    void SetUiOptions(byte eCo, byte eAo);

protected:

    // We store a map for each shader variant to get the PipelineState for a given Scene and Topology.
    // This is the key type for that map.
    struct PipelineStateMapKey
    {
        AZ::RHI::PrimitiveTopology m_topology;
        AZ::RPI::SceneId m_sceneId;

        bool operator<(const PipelineStateMapKey& other) const
        {
            if (m_topology == other.m_topology)
            {
                return m_sceneId < other.m_sceneId;
            }
            else
            {
                return m_topology < other.m_topology;
            }
        }
    };

    // For each ShaderVariant we cache the DrawListTag and the pipelineStates.
    struct ShaderVariantData
    {
        AZ::RPI::ShaderVariantStableId m_variantStableId;
        AZ::RPI::ShaderVariantKey m_shaderVariantKeyFallback;
        AZ::RHI::DrawListTag m_drawListTag;

        AZStd::map<PipelineStateMapKey, AZ::RHI::ConstPtr<AZ::RHI::PipelineState>> m_pipelineStates;
    };

    // This is the data that we cache for the shader
    struct ShaderData
    {
        const char* m_shaderFilepath;
        AZ::Data::Instance<AZ::RPI::Shader> m_shader;
        AZ::Data::Asset<AZ::RPI::ShaderResourceGroupAsset> m_perDrawSrgAsset;
        AZ::RHI::ShaderInputImageIndex m_imageInputIndex;
        AZ::RHI::ShaderInputConstantIndex m_viewProjInputIndex;

        AZ::RPI::ShaderVariantKey      m_fontVariantKeyFallback;
        AZ::RPI::ShaderVariantStableId m_fontVariantStableId;
        AZ::RPI::ShaderVariantKey      m_clampedImageVariantKeyFallback;
        AZ::RPI::ShaderVariantStableId m_clampedImageVariantStableId;
        AZ::RPI::ShaderVariantKey      m_wrappedImageVariantKeyFallback;
        AZ::RPI::ShaderVariantStableId m_wrappedImageVariantStableId;

        AZStd::map<AZ::RPI::ShaderVariantStableId, ShaderVariantData> m_shaderVariants;
    };

    static constexpr int MaxUiTextures = 16;

    // This is the data that we cache for the shader
    struct ShaderDataUi
    {
        const char* m_shaderFilepath;
        AZ::Data::Instance<AZ::RPI::Shader> m_shader;
        AZ::Data::Asset<AZ::RPI::ShaderResourceGroupAsset> m_perDrawSrgAsset;

        AZ::RHI::ShaderInputImageIndex m_imageInputIndex;
        AZ::RHI::ShaderInputSamplerIndex m_samplerInputIndex;
        AZ::RHI::ShaderInputConstantIndex m_viewProjInputIndex;

        AZ::RPI::ShaderVariantKey m_shaderVariantKeyFallback;
        AZ::RPI::ShaderVariantStableId m_defaultVariantStableId;

        AZStd::map<AZ::RPI::ShaderVariantStableId, ShaderVariantData> m_shaderVariants;
    };

    //! Load a shader from the given path and store data for using it in the given structure
    void LoadShader(const char* shaderFilepath, ShaderData& outShaderData);

    //! Load a shader from the given path and store data for using it in the given structure
    void LoadUiShader(const char* shaderFilepath, ShaderDataUi& outShaderData);

    //! Get or create the cached data for a shader variant. This cached data includes any pipeline state data that is created.
    ShaderVariantData& GetShaderVariantData(AZ::RPI::ShaderVariantStableId variantStableId, ShaderData& outShaderData);

    //! Get or create the cached data for a shader variant. This cached data includes any pipeline state data that is created.
    ShaderVariantData& GetUiShaderVariantData(AZ::RPI::ShaderVariantStableId variantStableId, ShaderDataUi& outShaderData);

    //! Get or create the cached pipeline state for the given combination of scene, shaderVariant and toplogyType
    AZ::RHI::ConstPtr<AZ::RHI::PipelineState> GetPipelineState(const AZ::RPI::Scene* scene, const ShaderData& shaderData, ShaderVariantData& shaderVariantData,
        AZ::RHI::PrimitiveTopology topology);

    //! Get or create the cached pipeline state for the given combination of scene, shaderVariant and toplogyType
    AZ::RHI::ConstPtr<AZ::RHI::PipelineState> GetUiPipelineState(const AZ::RPI::Scene* scene, const ShaderDataUi& shaderData, ShaderVariantData& shaderVariantData,
        AZ::RHI::PrimitiveTopology topology);

    //! Add a DrawPacket for the given vertices and (optionally) indices.
    //! @param shaderData   Specifies the shader to use plus the cached pipeline states, input indices etc
    //! @param imageView    The texture to use
    //! @param scene        The scene being rendered to (we need a different pipeline state for each scene since the output attachment is different)
    //! @param variantStableId The shader variant to use
    //! @param pBuf         The source vertices
    //! @param pInds        The source indices (can be nullptr)
    //! @param nVerts       The number of source vertices
    //! @param nInds        The number of source indices (can be zero)
    //! @param nPrimType    The primitive type
    void AddDraw(ShaderData& shaderData, const AZ::RHI::ImageView* imageView, AZ::RPI::Scene* scene, AZ::RPI::ShaderVariantStableId variantStableId,
        SVF_P3F_C4B_T2F* pBuf, uint16* pInds, int nVerts, int nInds, const PublicRenderPrimitiveType nPrimType);

    AZ::RHI::Ptr<AZ::RHI::Buffer> m_vertexBuffer;
    AZStd::array<AZ::RHI::StreamBufferView, 1> m_streamBufferViews;

    AZ::RHI::Ptr<AZ::RHI::Buffer> m_indexBuffer;
    AZ::RHI::IndexBufferView m_indexBufferView;
    AZ::RHI::Ptr<AZ::RHI::BufferPool> m_inputAssemblyPool;

    ShaderData m_simpleTexturedShader;
    ShaderDataUi m_uiShader;

    SVF_P3F_C4B_T2F* m_mappedVertexPtr = nullptr;
    AZ::u32 m_vertexCount = 0;
    AZ::u16* m_mappedIndexPtr = nullptr;
    AZ::u32 m_indexCount = 0;

    AZStd::vector<AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>> m_processSrgs;

    AZ::Matrix4x4 m_currentViewProj;
    AZ::s32 m_imageIdGuidGenerator = 0;
    AZ::u32 m_drawCount = 0;

    bool m_2DMode = false;
    AZ::RHI::DrawListTag m_2DPassDrawListTag;

    uint64_t m_lastRenderTick = uint64_t(-1);
    bool m_inFrame = false;

    bool m_uiUsePreMultipliedAlpha = false;

    enum class UiModulate
    {
        None,
        ModulateAlpha,
        ModulateAlphaAndColor
    };

    UiModulate m_uiModulateOption = UiModulate::None;

    private:
        static constexpr char LogName[] = "CAtomShimDynamicDraw";
};

#endif  //CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERATOMSHIM_ATOMSHIM_DYNAMICDRAW_H
