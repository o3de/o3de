/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector2.h>
#include <MCore/Source/Config.h>

#include "GraphicsManager.h"

#include "RenderTexture.h"
#include "PostProcessShader.h"
#include "GLSLShader.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "../../Common/FirstPersonCamera.h"
#include "../../Common/LookAtCamera.h"
#include "../../Common/OrbitCamera.h"
#include <MCore/Source/Random.h>


namespace RenderGL
{
    size_t GraphicsManager::s_numRandomOffsets = 64;
    GraphicsManager* gGraphicsManager = nullptr;


    // GetGraphicsManager()
    GraphicsManager* GetGraphicsManager()
    {
        return gGraphicsManager;
    }


    // Constructor
    GraphicsManager::GraphicsManager()
    {
        gGraphicsManager    = this;
        m_postProcessing     = false;

        // render background
        m_useGradientBackground  = true;
        m_clearColor             = MCore::RGBAColor(0.359f, 0.3984f, 0.4492f);
        m_gradientSourceColor    = MCore::RGBAColor(0.4941f, 0.5686f, 0.6470f);
        m_gradientTargetColor    = MCore::RGBAColor(0.0941f, 0.1019f, 0.1098f);

        m_gBuffer        = nullptr;
        m_hBloom         = nullptr;
        m_vBloom         = nullptr;
        m_hBlur          = nullptr;
        m_vBlur          = nullptr;
        m_downSample     = nullptr;
        m_dof            = nullptr;
        m_ssdo           = nullptr;
        m_hSmartBlur     = nullptr;
        m_vSmartBlur     = nullptr;
        m_renderTexture  = nullptr;
        m_activeShader   = nullptr;
        m_camera         = nullptr;
        m_renderUtil     = nullptr;

        m_mainLightIntensity = 1.00f;
        m_mainLightAngleA    = -30.0f;
        m_mainLightAngleB    = 18.0f;
        m_specularIntensity  = 1.0f;

        m_bloomEnabled   = true;
        m_bloomRadius    = 4.0f;
        m_bloomIntensity = 0.85f;
        m_bloomThreshold = 0.80f;

        m_dofEnabled     = false;
        m_dofBlurRadius  = 2.0f;
        m_dofFocalDistance = 500.0f;
        m_dofNear        = 0.001f;
        m_dofFar         = 1000.0f;

        m_rimAngle       = 60.0f;
        m_rimWidth       = 0.65f;
        m_rimIntensity   = 1.5f;
        m_rimColor       = MCore::RGBAColor(1.0f, 0.70f, 0.109f);

        m_randomVectorTexture    = nullptr;
        m_createMipMaps          = true;
        m_skipLoadingTextures    = false;

        // init random offsets
        m_randomOffsets.resize(s_numRandomOffsets);
        AZStd::vector<AZ::Vector3> samples = MCore::Random::RandomDirVectorsHalton(AZ::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::twoPi, s_numRandomOffsets);
        for (size_t i = 0; i < s_numRandomOffsets; ++i)
        {
            m_randomOffsets[i] = samples[i] * MCore::Random::RandF(0.1f, 1.0f);
        }
    }


    // destructor
    GraphicsManager::~GraphicsManager()
    {
        // shutdown the texture cache
        m_textureCache.Release();

        // delete all shaders
        m_shaderCache.Release();

        // get rid of the OpenGL render utility
        delete m_renderUtil;

        // release random vector texture memory
        delete m_randomVectorTexture;

        // clear the string memory
        m_shaderPath.clear();
    }


    // setup sunset color style rim lighting
    void GraphicsManager::SetupSunsetRim()
    {
        m_rimWidth       = 0.65f;
        m_rimIntensity   = 1.5f;
        m_rimColor       = MCore::RGBAColor(1.0f, 0.70f, 0.109f);
    }


    // setup blue color style rim lighting
    void GraphicsManager::SetupBlueRim()
    {
        m_rimWidth       = 0.65f;
        m_rimIntensity   = 1.5f;
        m_rimColor       = MCore::RGBAColor(81.0f / 255.0f, 160.0f / 255.0f, 1.0f);
    }


    // render gradient background
    void GraphicsManager::RenderGradientBackground(const MCore::RGBAColor& topColor, const MCore::RGBAColor& bottomColor)
    {
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_LIGHTING);

        SetShader(nullptr);

        glBegin(GL_QUADS);

        // bottom
        glColor3f(bottomColor.m_r, bottomColor.m_g, bottomColor.m_b);
        glVertex2f(-1.0, -1.0);
        glVertex2f(1.0, -1.0);

        // top
        glColor3f(topColor.m_r, topColor.m_g, topColor.m_b);
        glVertex2f(1.0, 1.0);
        glVertex2f(-1.0, 1.0);

        glEnd();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_DEPTH_TEST);
    }


    // BeginRender
    bool GraphicsManager::BeginRender()
    {
        //glPushAttrib( GL_ALL_ATTRIB_BITS );

        // Activate render targets
        glClearColor(m_clearColor.m_r, m_clearColor.m_g, m_clearColor.m_b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // render the gradient background
        if (m_useGradientBackground)
        {
            RenderGradientBackground(m_gradientSourceColor, m_gradientTargetColor);
        }

        return true;
    }


    // end a frame (perform the swap)
    void GraphicsManager::EndRender()
    {
        m_renderUtil->RenderTextPeriods();
        m_renderUtil->RenderTextures();
        ((MCommon::RenderUtil*)m_renderUtil)->Render2DLines();
    }


    // try to initialize the graphics system
    bool GraphicsManager::Init(AZ::IO::PathView shaderPath)
    {
        initializeOpenGLFunctions();

        // shaders
        SetShaderPath(shaderPath);

        // texture cache
        if (m_textureCache.Init() == false)
        {
            return false;
        }

        //if (CreateRandomVectorTexture() == false)
        //return false;

        glDepthFunc(GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);

        glClearColor(m_clearColor.m_r, m_clearColor.m_g, m_clearColor.m_b, 1.0f);

        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

        glDisable(GL_BLEND);

        // initialize utility rendering
        m_renderUtil = new GLRenderUtil(this);
        m_renderUtil->Init();

        // post processing
        if (m_postProcessing)
        {
            if (InitPostProcessing() == false)
            {
                m_postProcessing = false;
            }
        }

        // Get the maximum number of shader constant components.
        // The number of registers is this number divided by 4.
        // We need at least 1024 registers, so 4096 constant components.
        GLint maxConstantComponents = 0;
        glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxConstantComponents);
        AZ_Printf("EMotionFX", "Max shader constant components = %d (%d registers)\n", maxConstantComponents, maxConstantComponents / 4);
        AZ_Assert(maxConstantComponents >= 4096, "The GPU does not have the minimum required number of shader constants of 4096. It has %d instead.", maxConstantComponents);

        return true;
    }


    // InitPostProcessing()
    bool GraphicsManager::InitPostProcessing()
    {
        // get the maximum number of draw buffers
        GLint maxBuffers = 0;
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxBuffers);
        //MCore::LogDetailedInfo("[OpenGL] Maximum draw buffers = %d", maxBuffers);
        if (maxBuffers < 2)
        {
            MCore::LogDetailedInfo("[OpenGL] Maximum draw buffers is %d, while two are required for advanced rendering", maxBuffers);
            return false;
        }

        // get the viewport dimensions
        float viewportDimensions[4];
        glGetFloatv(GL_VIEWPORT, viewportDimensions);
        const uint32 screenWidth  = (uint32)viewportDimensions[2];
        const uint32 screenHeight = (uint32)viewportDimensions[3];

        // resize the render targets
        ResizeTextures(screenWidth, screenHeight);

        // load horizontal bloom
        m_hBloom = LoadPostProcessShader("HBloom.glsl");
        if (m_hBloom == nullptr)
        {
            MCore::LogWarning("[OpenGL] Failed to load HBloom shader, disabling post processing.");
            return false;
        }

        // load vertical bloom
        m_vBloom = LoadPostProcessShader("VBloom.glsl");
        if (m_vBloom == nullptr)
        {
            MCore::LogWarning("[OpenGL] Failed to load VBloom shader, disabling post processing.");
            return false;
        }

        // load vertical bloom
        m_downSample = LoadPostProcessShader("DownSample.glsl");
        if (m_downSample == nullptr)
        {
            MCore::LogWarning("[OpenGL] Failed to load DownSample shader, disabling post processing.");
            return false;
        }

        // load horizontal blur
        m_hBlur = LoadPostProcessShader("HBlur.glsl");
        if (m_hBlur == nullptr)
        {
            MCore::LogWarning("[OpenGL] Failed to load HBlur shader, disabling post processing.");
            return false;
        }

        // load vertical blur
        m_vBlur = LoadPostProcessShader("VBlur.glsl");
        if (m_vBlur == nullptr)
        {
            MCore::LogWarning("[OpenGL] Failed to load VBlur shader, disabling post processing.");
            return false;
        }

        // load DOF shader
        m_dof = LoadPostProcessShader("DepthOfField.glsl");
        if (m_dof == nullptr)
        {
            MCore::LogWarning("[OpenGL] Failed to load DOF shader, disabling post processing.");
            return false;
        }
        return true;
    }


    // try to load a texture
    Texture* GraphicsManager::LoadTexture([[maybe_unused]] AZ::IO::PathView filename, [[maybe_unused]] bool createMipMaps)
    {
        //Texture Library is no longer used
        //temporarily blank
        return nullptr;
    }


    // try to load a texture
    Texture* GraphicsManager::LoadTexture(AZ::IO::PathView filename)
    {
        return LoadTexture(filename, m_createMipMaps);
    }


    // LoadPostProcessShader
    PostProcessShader* GraphicsManager::LoadPostProcessShader(AZ::IO::PathView cFileName)
    {
        AZ::IO::PathView filename = m_shaderPath / cFileName;

        // check if the shader is already in the cache
        Shader* s = m_shaderCache.FindShader(filename.Native());
        if (s)
        {
            return (PostProcessShader*)s;
        }

        // load the shader from disk
        PostProcessShader* shader = new PostProcessShader();
        if (!shader->Init(filename))
        {
            delete shader;
            return nullptr;
        }

        m_shaderCache.AddShader(filename.Native(), shader);
        return shader;
    }


    // LoadShader
    GLSLShader* GraphicsManager::LoadShader(AZ::IO::PathView vertexFileName, AZ::IO::PathView pixelFileName)
    {
        AZStd::vector<AZStd::string> defines;
        return LoadShader(vertexFileName, pixelFileName, defines);
    }


    // LoadShader
    GLSLShader* GraphicsManager::LoadShader(AZ::IO::PathView vertexFileName, AZ::IO::PathView pixelFileName, AZStd::vector<AZStd::string>& defines)
    {
        const AZ::IO::Path vertexPath {vertexFileName.empty() ? AZ::IO::Path{} : m_shaderPath / vertexFileName};
        const AZ::IO::Path pixelPath {pixelFileName.empty() ? AZ::IO::Path{} : m_shaderPath / pixelFileName};

        // construct the lookup string for the shader cache
        AZStd::string cacheLookupStr = vertexPath.Native() + pixelPath.Native();
        for (const AZStd::string& define : defines)
        {
            cacheLookupStr += AZStd::string::format("#%s", define.c_str());
        }

        // check if the shader is already in the cache
        Shader* cShader = m_shaderCache.FindShader(cacheLookupStr);
        if (cShader)
        {
            return (GLSLShader*)cShader;
        }

        // load the shader from disk
        GLSLShader* shader = new GLSLShader();
        if (!shader->Init(vertexPath, pixelPath, defines))
        {
            delete shader;
            return nullptr;
        }

        m_shaderCache.AddShader(cacheLookupStr, shader);
        return shader;
    }


    // Resize
    void GraphicsManager::Resize(uint32 width, uint32 height)
    {
        MCORE_UNUSED(width);
        MCORE_UNUSED(height);
        // resize post processing textures
        //ResizeTextures(width, height);
    }


    // ResizeTextures
    bool GraphicsManager::ResizeTextures(uint32 screenWidth, uint32 screenHeight)
    {
        MCORE_UNUSED(screenWidth);
        MCORE_UNUSED(screenHeight);
        return true;
    }


    // SetShader
    void GraphicsManager::SetShader(Shader* shader)
    {
        if (m_activeShader == shader)
        {
            return;
        }

        if (shader == nullptr)
        {
            glUseProgram(0);
            m_activeShader = nullptr;
            return;
        }

        if (shader->GetType() == GLSLShader::TypeID)
        {
            GLSLShader* g = (GLSLShader*)shader;
            glUseProgram(g->GetProgram());
        }

        m_activeShader = shader;
    }


    // create the random vector texture
    bool GraphicsManager::CreateRandomVectorTexture(uint32 width, uint32 height)
    {
        uint32 textureID;

        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        // init texture data
        float* data = (float*)MCore::Allocate(width * height * 4 * sizeof(float));
        for (uint32 h = 0; h < height; ++h)
        {
            uint32 offset = h * (width * 4);
            for (uint32 w = 0; w < width; ++w)
            {
                AZ::Vector3 randVec = MCore::Random::RandDirVecF();
                data[offset  ] = randVec.GetX();
                data[offset + 1] = randVec.GetY();
                data[offset + 2] = randVec.GetZ();
                data[offset + 3] = MCore::Random::RandF(0.0f, 1.0f);
                offset += 4;
            }
        }

        // allocate texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, data);

        MCore::Free(data);

        //  assert( glGetError() == GL_NO_ERROR );
        if (glGetError() != GL_NO_ERROR)
        {
            MCore::LogWarning("[OpenGL] Failed to create random vector texture.");
            return false;
        }

        // create Texture object
        m_randomVectorTexture = new Texture(textureID, width, height);

        glDisable(GL_TEXTURE_2D);
        return true;
    }

    const char* GraphicsManager::GetDeviceName()
    {
        return (const char*)glGetString(GL_VENDOR);
    }

    const char* GraphicsManager::GetDeviceVendor()
    {
        return (const char*)glGetString(GL_RENDERER);
    }
}   // namespace RenderGL
