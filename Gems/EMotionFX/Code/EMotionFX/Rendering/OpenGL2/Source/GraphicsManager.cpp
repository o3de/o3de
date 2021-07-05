/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    size_t GraphicsManager::mNumRandomOffsets = 64;
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
        mPostProcessing     = false;

        // render background
        mUseGradientBackground  = true;
        mClearColor             = MCore::RGBAColor(0.359f, 0.3984f, 0.4492f);
        mGradientSourceColor    = MCore::RGBAColor(0.4941f, 0.5686f, 0.6470f);
        mGradientTargetColor    = MCore::RGBAColor(0.0941f, 0.1019f, 0.1098f);

        mGBuffer        = nullptr;
        mHBloom         = nullptr;
        mVBloom         = nullptr;
        mHBlur          = nullptr;
        mVBlur          = nullptr;
        mDownSample     = nullptr;
        mDOF            = nullptr;
        mSSDO           = nullptr;
        mHSmartBlur     = nullptr;
        mVSmartBlur     = nullptr;
        mRenderTexture  = nullptr;
        mActiveShader   = nullptr;
        mCamera         = nullptr;
        mRenderUtil     = nullptr;

        mMainLightIntensity = 1.00f;
        mMainLightAngleA    = -30.0f;
        mMainLightAngleB    = 18.0f;
        mSpecularIntensity  = 1.0f;

        mBloomEnabled   = true;
        mBloomRadius    = 4.0f;
        mBloomIntensity = 0.85f;
        mBloomThreshold = 0.80f;

        mDOFEnabled     = false;
        mDOFBlurRadius  = 2.0f;
        mDOFFocalDistance = 500.0f;
        mDOFNear        = 0.001f;
        mDOFFar         = 1000.0f;

        mRimAngle       = 60.0f;
        mRimWidth       = 0.65f;
        mRimIntensity   = 1.5f;
        mRimColor       = MCore::RGBAColor(1.0f, 0.70f, 0.109f);

        mRandomVectorTexture    = nullptr;
        mCreateMipMaps          = true;
        mSkipLoadingTextures    = false;

        // init random offsets
        mRandomOffsets.resize(mNumRandomOffsets);
        AZStd::vector<AZ::Vector3> samples = MCore::Random::RandomDirVectorsHalton(AZ::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::twoPi, mNumRandomOffsets);
        for (size_t i = 0; i < mNumRandomOffsets; ++i)
        {
            mRandomOffsets[i] = samples[i] * MCore::Random::RandF(0.1f, 1.0f);
        }
    }


    // destructor
    GraphicsManager::~GraphicsManager()
    {
        // shutdown the texture cache
        mTextureCache.Release();

        // delete all shaders
        mShaderCache.Release();

        // get rid of the OpenGL render utility
        delete mRenderUtil;

        // release random vector texture memory
        delete mRandomVectorTexture;

        // clear the string memory
        mShaderPath.clear();
    }


    // setup sunset color style rim lighting
    void GraphicsManager::SetupSunsetRim()
    {
        mRimWidth       = 0.65f;
        mRimIntensity   = 1.5f;
        mRimColor       = MCore::RGBAColor(1.0f, 0.70f, 0.109f);
        //mRimColor     = MCore::RGBAColor(1.0f, 0.77f, 0.30f);
    }


    // setup blue color style rim lighting
    void GraphicsManager::SetupBlueRim()
    {
        mRimWidth       = 0.65f;
        mRimIntensity   = 1.5f;
        mRimColor       = MCore::RGBAColor(81.0f / 255.0f, 160.0f / 255.0f, 1.0f);
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
        glColor3f(bottomColor.r, bottomColor.g, bottomColor.b);
        glVertex2f(-1.0, -1.0);
        glVertex2f(1.0, -1.0);

        // top
        glColor3f(topColor.r, topColor.g, topColor.b);
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
        glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // render the gradient background
        if (mUseGradientBackground)
        {
            RenderGradientBackground(mGradientSourceColor, mGradientTargetColor);
        }

        return true;
    }


    // end a frame (perform the swap)
    void GraphicsManager::EndRender()
    {
        mRenderUtil->RenderTextPeriods();
        mRenderUtil->RenderTextures();
        ((MCommon::RenderUtil*)mRenderUtil)->Render2DLines();
    }


    // try to initialize the graphics system
    bool GraphicsManager::Init(AZ::IO::PathView shaderPath)
    {
        initializeOpenGLFunctions();

        // shaders
        SetShaderPath(shaderPath);

        // texture cache
        if (mTextureCache.Init() == false)
        {
            return false;
        }

        //if (CreateRandomVectorTexture() == false)
        //return false;

        glDepthFunc(GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);

        glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, 1.0f);

        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

        glDisable(GL_BLEND);

        // initialize utility rendering
        mRenderUtil = new GLRenderUtil(this);
        mRenderUtil->Init();

        // post processing
        if (mPostProcessing)
        {
            if (InitPostProcessing() == false)
            {
                mPostProcessing = false;
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
        mHBloom = LoadPostProcessShader("HBloom.glsl");
        if (mHBloom == nullptr)
        {
            MCore::LogWarning("[OpenGL] Failed to load HBloom shader, disabling post processing.");
            return false;
        }

        // load vertical bloom
        mVBloom = LoadPostProcessShader("VBloom.glsl");
        if (mVBloom == nullptr)
        {
            MCore::LogWarning("[OpenGL] Failed to load VBloom shader, disabling post processing.");
            return false;
        }

        // load vertical bloom
        mDownSample = LoadPostProcessShader("DownSample.glsl");
        if (mDownSample == nullptr)
        {
            MCore::LogWarning("[OpenGL] Failed to load DownSample shader, disabling post processing.");
            return false;
        }

        // load horizontal blur
        mHBlur = LoadPostProcessShader("HBlur.glsl");
        if (mHBlur == nullptr)
        {
            MCore::LogWarning("[OpenGL] Failed to load HBlur shader, disabling post processing.");
            return false;
        }

        // load vertical blur
        mVBlur = LoadPostProcessShader("VBlur.glsl");
        if (mVBlur == nullptr)
        {
            MCore::LogWarning("[OpenGL] Failed to load VBlur shader, disabling post processing.");
            return false;
        }

        // load DOF shader
        mDOF = LoadPostProcessShader("DepthOfField.glsl");
        if (mDOF == nullptr)
        {
            MCore::LogWarning("[OpenGL] Failed to load DOF shader, disabling post processing.");
            return false;
        }
        /*
            // load screen space directional occlusion shader
            mSSDO = LoadPostProcessShader("SSDO.glsl");
            if (mSSDO == nullptr)
            {
                MCore::LogWarning("[OpenGL] Failed to load SSDO shader, disabling post processing.");
                return false;
            }

            // horizontal smartblur
            mHSmartBlur = LoadPostProcessShader("HSmartBlur.glsl");
            if (mHSmartBlur == nullptr)
            {
                MCore::LogWarning("[OpenGL] Failed to load HSmartBlur shader, disabling post processing.");
                return false;
            }

            // vertical smartblur
            mVSmartBlur = LoadPostProcessShader("VSmartBlur.glsl");
            if (mVSmartBlur == nullptr)
            {
                MCore::LogWarning("[OpenGL] Failed to load VSmartBlur shader, disabling post processing.");
                return false;
            }
        */
        /*
            // create the post processing shaders
            mSSAO   = LoadPostProcessShader("SSAO.glsl");
            if (mSSAO == nullptr)
            {
                MCore::LogWarning("[OpenGL] Failed to load SSAO shader, disabling post processing.");
                return false;
            }
        */
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
        return LoadTexture(filename, mCreateMipMaps);
    }


    // LoadPostProcessShader
    PostProcessShader* GraphicsManager::LoadPostProcessShader(AZ::IO::PathView cFileName)
    {
        AZ::IO::PathView filename = mShaderPath / cFileName;

        // check if the shader is already in the cache
        Shader* s = mShaderCache.FindShader(filename.Native());
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

        mShaderCache.AddShader(filename.Native(), shader);
        return shader;
    }


    // LoadShader
    GLSLShader* GraphicsManager::LoadShader(AZ::IO::PathView vertexFileName, AZ::IO::PathView pixelFileName)
    {
        MCore::Array<AZStd::string> defines;
        return LoadShader(vertexFileName, pixelFileName, defines);
    }


    // LoadShader
    GLSLShader* GraphicsManager::LoadShader(AZ::IO::PathView vertexFileName, AZ::IO::PathView pixelFileName, MCore::Array<AZStd::string>& defines)
    {
        const AZ::IO::Path vertexPath {vertexFileName.empty() ? AZ::IO::Path{} : mShaderPath / vertexFileName};
        const AZ::IO::Path pixelPath {pixelFileName.empty() ? AZ::IO::Path{} : mShaderPath / pixelFileName};

        // construct the lookup string for the shader cache
        AZStd::string cacheLookupStr = vertexPath.Native() + pixelPath.Native();
        const uint32 numDefines = defines.GetLength();
        for (uint32 n = 0; n < numDefines; n++)
        {
            cacheLookupStr += AZStd::string::format("#%s", defines[n].c_str());
        }

        // check if the shader is already in the cache
        Shader* cShader = mShaderCache.FindShader(cacheLookupStr);
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

        mShaderCache.AddShader(cacheLookupStr, shader);
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
        if (mActiveShader == shader)
        {
            return;
        }

        if (shader == nullptr)
        {
            glUseProgram(0);
            mActiveShader = nullptr;
            return;
        }

        if (shader->GetType() == GLSLShader::TypeID)
        {
            GLSLShader* g = (GLSLShader*)shader;
            glUseProgram(g->GetProgram());
        }

        mActiveShader = shader;
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
        mRandomVectorTexture = new Texture(textureID, width, height);

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
