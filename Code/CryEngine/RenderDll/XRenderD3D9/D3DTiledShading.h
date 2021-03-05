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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef _D3DTILEDSHADING_H_
#define _D3DTILEDSHADING_H_

struct STiledLightCullInfo;
class CD3D9Renderer;

struct STiledLightShadeInfo
{
    uint32    lightType;
    uint32    resIndex;
    uint32    shadowMaskIndex;
    uint16    stencilID0;
    uint16    stencilID1;
    Vec4      posRad;
    Vec4      dirCosAngle;
    Vec2      attenuationParams;
    Vec2      shadowParams;
    Vec4      color;
    Vec4      shadowChannelIndex;
    Matrix44  projectorMatrix;
    Matrix44  shadowMatrix;
};  // 256 bytes

class CTiledShading
{
protected:
    struct AtlasItem
    {
        ITexture* texture;
        int       updateFrameID;
        int       accessFrameID;
        bool      invalid;

        AtlasItem()
            : texture(NULL)
            , updateFrameID(-1)
            , accessFrameID(0)
            , invalid(false) {}
    };

    struct TextureAtlas
    {
        CTexture* texArray;
        std::vector<AtlasItem>  items;

        TextureAtlas()
            : texArray(NULL) {}
    };

public:
    CTiledShading();

    void CreateResources();
    void DestroyResources(bool destroyResolutionIndependentResources);
    void Clear();

    void Render(TArray<SRenderLight>& envProbes, TArray<SRenderLight>& ambientLights, TArray<SRenderLight>& defLights, Vec4* clipVolumeParams);

    void BindForwardShadingResources(CShader* pShader, EHWShaderClass shType = eHWSC_Pixel);
    void UnbindForwardShadingResources(EHWShaderClass shType = eHWSC_Pixel);

    STiledLightShadeInfo* GetTiledLightShadeInfo();

    int InsertTextureToSpecularProbeAtlas(CTexture* texture, int arrayIndex) {return InsertTexture(texture, m_specularProbeAtlas, arrayIndex); }
    int InsertTextureToDiffuseProbeAtlas(CTexture* texture, int arrayIndex) {return InsertTexture(texture, m_diffuseProbeAtlas, arrayIndex); }
    int InsertTextureToSpotTexAtlas(CTexture* texture, int arrayIndex) {return InsertTexture(texture, m_spotTexAtlas, arrayIndex); }

    void NotifyCausticsVisible() { m_bApplyCaustics = true; }

protected:
    int InsertTexture(CTexture* texture, TextureAtlas& atlas, int arrayIndex);
    void PrepareLightList(TArray<SRenderLight>& envProbes, TArray<SRenderLight>& ambientLights, TArray<SRenderLight>& defLights);
    void PrepareEnvironmentProbes(bool& shouldAdd, SRenderLight& renderLight, STiledLightCullInfo& lightCullInfo, STiledLightShadeInfo& lightShadeInfo, Matrix44A& matView, const float invCameraFar, const Vec4& posVS);
    void PrepareRegularAndAmbientLights(bool& shouldAdd, SRenderLight& renderLight, STiledLightCullInfo& lightCullInfo, STiledLightShadeInfo& lightShadeInfo, Matrix44A& matView, const float invCameraFar, const Vec4& posVS,
        const bool ambientLight, CD3D9Renderer* const __restrict rd, const bool areaLightRect, const uint32 lightIdx, const uint32 firstShadowLight,const uint32 curShadowPoolLight, const int nThreadID, const int nRecurseLevel,
        uint32& numTileLights);

    void PrepareShadowCastersList(TArray<SRenderLight>& defLights);
    void PrepareClipVolumeList(Vec4* clipVolumeParams);

protected:
    uint32             m_dispatchSizeX, m_dispatchSizeY;

    WrappedDX11Buffer  m_lightCullInfoBuf;
    WrappedDX11Buffer  m_LightShadeInfoBuf;
    WrappedDX11Buffer  m_tileLightIndexBuf;

    WrappedDX11Buffer  m_clipVolumeInfoBuf;

    TextureAtlas       m_specularProbeAtlas;
    TextureAtlas       m_diffuseProbeAtlas;
    TextureAtlas       m_spotTexAtlas;

    uint32             m_nTexStateTriLinear;
    uint32             m_nTexStateCompare;

    uint32             m_numSkippedLights;
    uint32             m_numAtlasUpdates;

    TArray<uint32>     m_arrShadowCastingLights;

    bool               m_bApplyCaustics;
};

#endif
