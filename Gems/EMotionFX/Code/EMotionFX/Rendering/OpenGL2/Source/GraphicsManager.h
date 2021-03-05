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

#ifndef __RENDERGL_GRAPHICSMANAGER__H
#define __RENDERGL_GRAPHICSMANAGER__H

#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Vector.h>
#include <MCore/Source/Color.h>
#include "RenderGLConfig.h"
#include "../../Common/Camera.h"
#include "shadercache.h"
#include "TextureCache.h"
#include "GLRenderUtil.h"
#include "GBuffer.h"



namespace RenderGL
{
    // foward declarations
    class GLFont;
    class VertexBuffer;
    class Shader;
    class PostProcessShader;
    class GLSLShader;
    class RenderTexture;
    class Texture;
    class TextureCache;


    class RENDERGL_API GraphicsManager
    {
        MCORE_MEMORYOBJECTCATEGORY(GraphicsManager, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        GraphicsManager();
        ~GraphicsManager();

        bool BeginRender();
        void EndRender();

        MCORE_INLINE MCommon::Camera* GetCamera() const                                         { return mCamera; }
        MCORE_INLINE GLRenderUtil* GetRenderUtil()                                              { return mRenderUtil; }
        const char* GetDeviceName();
        const char* GetDeviceVendor();
        MCORE_INLINE RenderTexture* GetRenderTexture()                                          { return mRenderTexture; }
        MCORE_INLINE const char* GetShaderPath() const                                          { return mShaderPath.c_str(); }
        MCORE_INLINE TextureCache* GetTextureCache()                                            { return &mTextureCache; }

        bool Init(const char* shaderPath = "Shaders/");

        bool GetIsPostProcessingEnabled() const                                                 { return mPostProcessing; }
        PostProcessShader* LoadPostProcessShader(const char* filename);
        GLSLShader* LoadShader(const char* vertexFileName, const char* pixelFileName);
        GLSLShader* LoadShader(const char* vertexFileName, const char* pixelFileName, MCore::Array<AZStd::string>& defines);

        MCORE_INLINE void SetGBuffer(GBuffer* gBuffer)                                          { mGBuffer = gBuffer; }
        MCORE_INLINE GBuffer* GetGBuffer()                                                      { return mGBuffer; }

        Texture* LoadTexture(const char* filename, bool createMipMaps);
        Texture* LoadTexture(const char* filename);

        void SetCreateMipMaps(bool createMipMaps)                                               { mCreateMipMaps = createMipMaps; }
        MCORE_INLINE bool GetCreateMipMaps() const                                              { return mCreateMipMaps; }

        void SetSkipLoadingTextures(bool skipTextures)                                          { mSkipLoadingTextures = skipTextures; }
        MCORE_INLINE bool GetSkipLoadingTextures() const                                        { return mSkipLoadingTextures; }

        void Resize(uint32 width, uint32 height);

        MCORE_INLINE void SetCamera(MCommon::Camera* camera)                                    { mCamera = camera; }

        // background rendering and colors
        MCORE_INLINE void SetClearColor(const MCore::RGBAColor& color)                          { mClearColor = color; }
        MCORE_INLINE void SetGradientSourceColor(const MCore::RGBAColor& color)                 { mGradientSourceColor = color; }
        MCORE_INLINE void SetGradientTargetColor(const MCore::RGBAColor& color)                 { mGradientTargetColor = color; }
        MCORE_INLINE void SetUseGradientBackground(bool enabled)                                { mUseGradientBackground = enabled; }
        MCORE_INLINE MCore::RGBAColor GetClearColor() const                                     { return mClearColor; }
        MCORE_INLINE MCore::RGBAColor GetGradientSourceColor() const                            { return mGradientSourceColor; }
        MCORE_INLINE MCore::RGBAColor GetGradientTargetColor() const                            { return mGradientTargetColor; }
        void RenderGradientBackground(const MCore::RGBAColor& topColor, const MCore::RGBAColor& bottomColor);


        void SetShader(Shader* shader);
        MCORE_INLINE void SetRenderTexture(RenderTexture* texture)                              { mRenderTexture = texture; }
        MCORE_INLINE void SetShaderPath(const char* shaderPath)                                 { mShaderPath = shaderPath; }

        MCORE_INLINE void SetAdvancedRendering(bool enabled)                                    { mAdvancedRendering = enabled; }
        MCORE_INLINE void SetBloomEnabled(bool enabled)                                         { mBloomEnabled     = enabled; }
        MCORE_INLINE void SetBloomThreshold(float threshold)                                    { mBloomThreshold   = threshold; }
        MCORE_INLINE void SetBloomIntensity(float intensity)                                    { mBloomIntensity   = intensity; }
        MCORE_INLINE void SetBloomRadius(float radius)                                          { mBloomRadius      = radius; }
        MCORE_INLINE void SetDOFEnabled(bool enabled)                                           { mDOFEnabled       = enabled; }
        MCORE_INLINE void SetDOFFocalDistance(float dist)                                       { mDOFFocalDistance = dist; }
        MCORE_INLINE void SetDOFNear(float dist)                                                { mDOFNear          = dist; }
        MCORE_INLINE void SetDOFFar(float dist)                                                 { mDOFFar           = dist; }
        MCORE_INLINE void SetDOFBlurRadius(float radius)                                        { mDOFBlurRadius    = radius; }

        MCORE_INLINE void SetRimColor(const MCore::RGBAColor& color)                            { mRimColor         = color; }
        MCORE_INLINE void SetRimIntensity(float intensity)                                      { mRimIntensity     = intensity; }
        MCORE_INLINE void SetRimWidth(float width)                                              { mRimWidth         = width; }
        MCORE_INLINE void SetRimAngle(float angleInDegrees)                                     { mRimAngle         = angleInDegrees; }

        MCORE_INLINE void SetMainLightIntensity(float intensity)                                { mMainLightIntensity = intensity; }
        MCORE_INLINE void SetMainLightAngleA(float angleInDegrees)                              { mMainLightAngleA  = angleInDegrees; }
        MCORE_INLINE void SetMainLightAngleB(float angleInDegrees)                              { mMainLightAngleB  = angleInDegrees; }
        MCORE_INLINE void SetSpecularIntensity(float intensity)                                 { mSpecularIntensity = intensity; }

        MCORE_INLINE bool GetAdvancedRendering() const                                          { return mAdvancedRendering; }
        MCORE_INLINE bool GetBloomEnabled() const                                               { return mBloomEnabled; }
        MCORE_INLINE float GetBloomThreshold() const                                            { return mBloomThreshold; }
        MCORE_INLINE float GetBloomIntensity() const                                            { return mBloomIntensity; }
        MCORE_INLINE float GetBloomRadius() const                                               { return mBloomRadius; }
        MCORE_INLINE bool GetDOFEnabled() const                                                 { return mDOFEnabled; }
        MCORE_INLINE float GetDOFBlurRadius() const                                             { return mDOFBlurRadius; }
        MCORE_INLINE float GetDOFFocalDistance() const                                          { return mDOFFocalDistance; }
        MCORE_INLINE float GetDOFNear() const                                                   { return mDOFNear; }
        MCORE_INLINE float GetDOFFar() const                                                    { return mDOFFar; }

        MCORE_INLINE const MCore::RGBAColor& GetRimColor() const                                { return mRimColor; }
        MCORE_INLINE float GetRimIntensity() const                                              { return mRimIntensity; }
        MCORE_INLINE float GetRimWidth() const                                                  { return mRimWidth; }
        MCORE_INLINE float GetRimAngle() const                                                  { return mRimAngle; }

        MCORE_INLINE float GetMainLightIntensity() const                                        { return mMainLightIntensity; }
        MCORE_INLINE float GetMainLightAngleA() const                                           { return mMainLightAngleA; }
        MCORE_INLINE float GetMainLightAngleB() const                                           { return mMainLightAngleB; }
        MCORE_INLINE float GetSpecularIntensity() const                                         { return mSpecularIntensity; }

        void SetupSunsetRim();
        void SetupBlueRim();

    private:
        bool InitPostProcessing();
        bool ResizeTextures(uint32 screenWidth, uint32 screenHeight);
        bool CreateRandomVectorTexture(uint32 width, uint32 height);

        bool                mPostProcessing;
        RenderTexture*      mRenderTexture;                 // Active RT

        GBuffer*            mGBuffer;       /**< The g-buffer. */

        MCommon::Camera*    mCamera;        /**< The camera used for rendering. */

        ShaderCache         mShaderCache;   /**< The shader manager used to load and manage vertex and pixel shaders. */
        AZStd::string       mShaderPath;    /**< The absolute path to the directory where the shaders are located. This string will be added as prefix to each shader file the user tries to load. */
        MCore::RGBAColor    mClearColor;    /**< The scene background color. */
        MCore::RGBAColor    mGradientSourceColor;   /**< The background gradient source color. */
        MCore::RGBAColor    mGradientTargetColor;   /**< The background gradient target color. */
        bool                mUseGradientBackground;
        Shader*             mActiveShader;  /**< The currently used shader. */

        // post process shaders
        PostProcessShader*  mHBloom;
        PostProcessShader*  mVBloom;
        PostProcessShader*  mDownSample;
        PostProcessShader*  mHBlur;
        PostProcessShader*  mVBlur;
        PostProcessShader*  mDOF;
        PostProcessShader*  mSSDO;
        PostProcessShader*  mHSmartBlur;
        PostProcessShader*  mVSmartBlur;

        Texture*            mRandomVectorTexture;
        AZStd::vector<AZ::Vector3> mRandomOffsets;
        static size_t       mNumRandomOffsets;

        GLRenderUtil*       mRenderUtil;    /**< The rendering utility. */
        TextureCache        mTextureCache;  /**< The texture manager used to load and manage textures. */

        bool                mAdvancedRendering;
        bool                mBloomEnabled;
        float               mBloomThreshold;
        float               mBloomIntensity;
        float               mBloomRadius;
        bool                mDOFEnabled;
        float               mDOFFocalDistance;
        float               mDOFNear;
        float               mDOFFar;
        float               mDOFBlurRadius;
        float               mRimAngle;
        float               mRimWidth;
        float               mRimIntensity;
        MCore::RGBAColor    mRimColor;
        float               mMainLightIntensity;
        float               mMainLightAngleA;
        float               mMainLightAngleB;
        float               mSpecularIntensity;
        bool                mCreateMipMaps;
        bool                mSkipLoadingTextures;
    };

    GraphicsManager* GetGraphicsManager();
}

#endif
