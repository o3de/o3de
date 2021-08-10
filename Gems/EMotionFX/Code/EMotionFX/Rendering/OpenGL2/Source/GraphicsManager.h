/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __RENDERGL_GRAPHICSMANAGER__H
#define __RENDERGL_GRAPHICSMANAGER__H

#include <AzCore/IO/Path/Path.h>
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Vector.h>
#include <MCore/Source/Color.h>
#include "RenderGLConfig.h"
#include "../../Common/Camera.h"
#include "shadercache.h"
#include "TextureCache.h"
#include "GLRenderUtil.h"
#include "GBuffer.h"

#include <AzCore/PlatformIncl.h>
#include <QOpenGLExtraFunctions>


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
        : private QOpenGLExtraFunctions
    {
        MCORE_MEMORYOBJECTCATEGORY(GraphicsManager, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        GraphicsManager();
        ~GraphicsManager();

        bool BeginRender();
        void EndRender();

        MCORE_INLINE MCommon::Camera* GetCamera() const                                         { return m_camera; }
        MCORE_INLINE GLRenderUtil* GetRenderUtil()                                              { return m_renderUtil; }
        const char* GetDeviceName();
        const char* GetDeviceVendor();
        MCORE_INLINE RenderTexture* GetRenderTexture()                                          { return m_renderTexture; }
        MCORE_INLINE AZ::IO::PathView GetShaderPath() const                                     { return m_shaderPath; }
        MCORE_INLINE TextureCache* GetTextureCache()                                            { return &m_textureCache; }

        bool Init(AZ::IO::PathView shaderPath = "Shaders");

        bool GetIsPostProcessingEnabled() const                                                 { return m_postProcessing; }
        PostProcessShader* LoadPostProcessShader(AZ::IO::PathView filename);
        GLSLShader* LoadShader(AZ::IO::PathView vertexFileName, AZ::IO::PathView pixelFileName);
        GLSLShader* LoadShader(AZ::IO::PathView vertexFileName, AZ::IO::PathView pixelFileName, AZStd::vector<AZStd::string>& defines);

        MCORE_INLINE void SetGBuffer(GBuffer* gBuffer)                                          { m_gBuffer = gBuffer; }
        MCORE_INLINE GBuffer* GetGBuffer()                                                      { return m_gBuffer; }

        Texture* LoadTexture(AZ::IO::PathView filename, bool createMipMaps);
        Texture* LoadTexture(AZ::IO::PathView filename);

        void SetCreateMipMaps(bool createMipMaps)                                               { m_createMipMaps = createMipMaps; }
        MCORE_INLINE bool GetCreateMipMaps() const                                              { return m_createMipMaps; }

        void SetSkipLoadingTextures(bool skipTextures)                                          { m_skipLoadingTextures = skipTextures; }
        MCORE_INLINE bool GetSkipLoadingTextures() const                                        { return m_skipLoadingTextures; }

        void Resize(uint32 width, uint32 height);

        MCORE_INLINE void SetCamera(MCommon::Camera* camera)                                    { m_camera = camera; }

        // background rendering and colors
        MCORE_INLINE void SetClearColor(const MCore::RGBAColor& color)                          { m_clearColor = color; }
        MCORE_INLINE void SetGradientSourceColor(const MCore::RGBAColor& color)                 { m_gradientSourceColor = color; }
        MCORE_INLINE void SetGradientTargetColor(const MCore::RGBAColor& color)                 { m_gradientTargetColor = color; }
        MCORE_INLINE void SetUseGradientBackground(bool enabled)                                { m_useGradientBackground = enabled; }
        MCORE_INLINE MCore::RGBAColor GetClearColor() const                                     { return m_clearColor; }
        MCORE_INLINE MCore::RGBAColor GetGradientSourceColor() const                            { return m_gradientSourceColor; }
        MCORE_INLINE MCore::RGBAColor GetGradientTargetColor() const                            { return m_gradientTargetColor; }
        void RenderGradientBackground(const MCore::RGBAColor& topColor, const MCore::RGBAColor& bottomColor);


        void SetShader(Shader* shader);
        MCORE_INLINE void SetRenderTexture(RenderTexture* texture)                              { m_renderTexture = texture; }
        MCORE_INLINE void SetShaderPath(AZ::IO::PathView shaderPath)                            { m_shaderPath = shaderPath; }

        MCORE_INLINE void SetBloomEnabled(bool enabled)                                         { m_bloomEnabled     = enabled; }
        MCORE_INLINE void SetBloomThreshold(float threshold)                                    { m_bloomThreshold   = threshold; }
        MCORE_INLINE void SetBloomIntensity(float intensity)                                    { m_bloomIntensity   = intensity; }
        MCORE_INLINE void SetBloomRadius(float radius)                                          { m_bloomRadius      = radius; }
        MCORE_INLINE void SetDOFEnabled(bool enabled)                                           { m_dofEnabled       = enabled; }
        MCORE_INLINE void SetDOFFocalDistance(float dist)                                       { m_dofFocalDistance = dist; }
        MCORE_INLINE void SetDOFNear(float dist)                                                { m_dofNear          = dist; }
        MCORE_INLINE void SetDOFFar(float dist)                                                 { m_dofFar           = dist; }
        MCORE_INLINE void SetDOFBlurRadius(float radius)                                        { m_dofBlurRadius    = radius; }

        MCORE_INLINE void SetRimColor(const MCore::RGBAColor& color)                            { m_rimColor         = color; }
        MCORE_INLINE void SetRimIntensity(float intensity)                                      { m_rimIntensity     = intensity; }
        MCORE_INLINE void SetRimWidth(float width)                                              { m_rimWidth         = width; }
        MCORE_INLINE void SetRimAngle(float angleInDegrees)                                     { m_rimAngle         = angleInDegrees; }

        MCORE_INLINE void SetMainLightIntensity(float intensity)                                { m_mainLightIntensity = intensity; }
        MCORE_INLINE void SetMainLightAngleA(float angleInDegrees)                              { m_mainLightAngleA  = angleInDegrees; }
        MCORE_INLINE void SetMainLightAngleB(float angleInDegrees)                              { m_mainLightAngleB  = angleInDegrees; }
        MCORE_INLINE void SetSpecularIntensity(float intensity)                                 { m_specularIntensity = intensity; }

        MCORE_INLINE bool GetBloomEnabled() const                                               { return m_bloomEnabled; }
        MCORE_INLINE float GetBloomThreshold() const                                            { return m_bloomThreshold; }
        MCORE_INLINE float GetBloomIntensity() const                                            { return m_bloomIntensity; }
        MCORE_INLINE float GetBloomRadius() const                                               { return m_bloomRadius; }
        MCORE_INLINE bool GetDOFEnabled() const                                                 { return m_dofEnabled; }
        MCORE_INLINE float GetDOFBlurRadius() const                                             { return m_dofBlurRadius; }
        MCORE_INLINE float GetDOFFocalDistance() const                                          { return m_dofFocalDistance; }
        MCORE_INLINE float GetDOFNear() const                                                   { return m_dofNear; }
        MCORE_INLINE float GetDOFFar() const                                                    { return m_dofFar; }

        MCORE_INLINE const MCore::RGBAColor& GetRimColor() const                                { return m_rimColor; }
        MCORE_INLINE float GetRimIntensity() const                                              { return m_rimIntensity; }
        MCORE_INLINE float GetRimWidth() const                                                  { return m_rimWidth; }
        MCORE_INLINE float GetRimAngle() const                                                  { return m_rimAngle; }

        MCORE_INLINE float GetMainLightIntensity() const                                        { return m_mainLightIntensity; }
        MCORE_INLINE float GetMainLightAngleA() const                                           { return m_mainLightAngleA; }
        MCORE_INLINE float GetMainLightAngleB() const                                           { return m_mainLightAngleB; }
        MCORE_INLINE float GetSpecularIntensity() const                                         { return m_specularIntensity; }

        void SetupSunsetRim();
        void SetupBlueRim();

    private:
        bool InitPostProcessing();
        bool ResizeTextures(uint32 screenWidth, uint32 screenHeight);
        bool CreateRandomVectorTexture(uint32 width, uint32 height);

        bool                m_postProcessing;
        RenderTexture*      m_renderTexture;                 // Active RT

        GBuffer*            m_gBuffer;       /**< The g-buffer. */

        MCommon::Camera*    m_camera;        /**< The camera used for rendering. */

        ShaderCache         m_shaderCache;   /**< The shader manager used to load and manage vertex and pixel shaders. */
        AZ::IO::Path        m_shaderPath;    /**< The absolute path to the directory where the shaders are located. This string will be added as prefix to each shader file the user tries to load. */
        MCore::RGBAColor    m_clearColor;    /**< The scene background color. */
        MCore::RGBAColor    m_gradientSourceColor;   /**< The background gradient source color. */
        MCore::RGBAColor    m_gradientTargetColor;   /**< The background gradient target color. */
        bool                m_useGradientBackground;
        Shader*             m_activeShader;  /**< The currently used shader. */

        // post process shaders
        PostProcessShader*  m_hBloom;
        PostProcessShader*  m_vBloom;
        PostProcessShader*  m_downSample;
        PostProcessShader*  m_hBlur;
        PostProcessShader*  m_vBlur;
        PostProcessShader*  m_dof;
        PostProcessShader*  m_ssdo;
        PostProcessShader*  m_hSmartBlur;
        PostProcessShader*  m_vSmartBlur;

        Texture*            m_randomVectorTexture;
        AZStd::vector<AZ::Vector3> m_randomOffsets;
        static size_t       s_numRandomOffsets;

        GLRenderUtil*       m_renderUtil;    /**< The rendering utility. */
        TextureCache        m_textureCache;  /**< The texture manager used to load and manage textures. */

        bool                m_bloomEnabled;
        float               m_bloomThreshold;
        float               m_bloomIntensity;
        float               m_bloomRadius;
        bool                m_dofEnabled;
        float               m_dofFocalDistance;
        float               m_dofNear;
        float               m_dofFar;
        float               m_dofBlurRadius;
        float               m_rimAngle;
        float               m_rimWidth;
        float               m_rimIntensity;
        MCore::RGBAColor    m_rimColor;
        float               m_mainLightIntensity;
        float               m_mainLightAngleA;
        float               m_mainLightAngleB;
        float               m_specularIntensity;
        bool                m_createMipMaps;
        bool                m_skipLoadingTextures;
    };

    GraphicsManager* GetGraphicsManager();
}

#endif
