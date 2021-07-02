/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MCore/Source/Config.h>
#include <MCore/Source/AzCoreConversions.h>

#include "GLRenderUtil.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "GLSLShader.h"
#include "TextureCache.h"
#include "GraphicsManager.h"
#include "../../Common/OrthographicCamera.h"
#include <AzCore/Debug/Timer.h>


namespace RenderGL
{
    // constructor
    GLRenderUtil::GLRenderUtil(GraphicsManager* graphicsManager)
        : RenderUtil()
    {
        // set/reset the member variables
        mGraphicsManager        = graphicsManager;
        mLineShader             = nullptr;
        mMeshShader             = nullptr;
        mMeshVertexBuffer       = nullptr;
        mMeshIndexBuffer        = nullptr;

        mTriangleVertexBuffer    = nullptr;
        mTriangleIndexBuffer     = nullptr;

        mCurrentLineVB          = 0;

        for (uint32 i = 0; i < MAX_LINE_VERTEXBUFFERS; ++i)
        {
            mLineVertexBuffers[i] = nullptr;
        }

        // initialize the vertex buffers and the shader used for line rendering
        for (uint32 i = 0; i < MAX_LINE_VERTEXBUFFERS; ++i)
        {
            mLineVertexBuffers[i] = new VertexBuffer();
            if (mLineVertexBuffers[i]->Init(sizeof(LineVertex), mNumMaxLineVertices, USAGE_DYNAMIC) == false)
            {
                MCore::LogError("[OpenGL]  Failed to create render utility line vertex buffer.");
                CleanUp();
            }
        }

        mLineShader = graphicsManager->LoadShader("Line_VS.glsl", "Line_PS.glsl");

        // initialize the vertex and the index buffers as well as the shader used for rendering util meshes
        mMeshVertexBuffer   = new VertexBuffer();
        mMeshIndexBuffer    = new IndexBuffer();

        if (mMeshVertexBuffer->Init(sizeof(UtilMeshVertex), mNumMaxMeshVertices, USAGE_DYNAMIC) == false)
        {
            MCore::LogError("[OpenGL]  Failed to create render utility mesh vertex buffer.");
            CleanUp();
            return;
        }

        if (mMeshIndexBuffer->Init(IndexBuffer::INDEXSIZE_32BIT, mNumMaxMeshIndices, USAGE_DYNAMIC) == false)
        {
            MCore::LogError("[OpenGL]  Failed to create render utility mesh index buffer.");
            CleanUp();
            return;
        }

        mMeshShader = graphicsManager->LoadShader("RenderUtil_VS.glsl", "RenderUtil_PS.glsl");

        // initialize the triangle rendering buffers
        mTriangleVertexBuffer = new VertexBuffer();
        if (mTriangleVertexBuffer->Init(sizeof(TriangleVertex), mNumMaxTriangleVertices, USAGE_DYNAMIC) == false)
        {
            MCore::LogError("[OpenGL]  Failed to create triangle vertex buffer.");
            CleanUp();
            return;
        }

        mTriangleIndexBuffer = new IndexBuffer();
        if (mTriangleIndexBuffer->Init(IndexBuffer::INDEXSIZE_32BIT, mNumMaxTriangleVertices, USAGE_STATIC) == false)
        {
            MCore::LogError("[OpenGL]  Failed to create triangle index buffer.");
            CleanUp();
            return;
        }

        // lock the index buffer and fill in the static indices
        uint32* indices = (uint32*)mTriangleIndexBuffer->Lock();
        if (indices)
        {
            for (uint32 i = 0; i < mNumMaxTriangleVertices; ++i)
            {
                indices[i] = i;
            }

            mTriangleIndexBuffer->Unlock();
        }

        // texture rendering
        mMaxNumTextures     = 256;
        mNumTextures        = 0;
        mTextures           = new TextureEntry[mMaxNumTextures];

        // text rendering
        mTextEntries.SetMemoryCategory(MEMCATEGORY_RENDERING);
    }


    // destructor
    GLRenderUtil::~GLRenderUtil()
    {
        CleanUp();
    }

    void GLRenderUtil::Init()
    {
        initializeOpenGLFunctions();
    }

    void GLRenderUtil::Validate()
    {
        if (mLineShader)
        {
            mLineShader->Validate();
        }
        if (mMeshShader)
        {
            mMeshShader->Validate();
        }
    }

    // destroy the allocated memory
    void GLRenderUtil::CleanUp()
    {
        for (uint32 i = 0; i < MAX_LINE_VERTEXBUFFERS; ++i)
        {
            delete mLineVertexBuffers[i];
            mLineVertexBuffers[i] = nullptr;
        }

        delete mMeshVertexBuffer;
        delete mMeshIndexBuffer;

        delete mTriangleVertexBuffer;
        delete mTriangleIndexBuffer;

        mMeshVertexBuffer   = nullptr;
        mMeshIndexBuffer    = nullptr;

        mTriangleVertexBuffer   = nullptr;
        mTriangleIndexBuffer    = nullptr;

        mCurrentLineVB      = 0;

        // get rid of the texture entries
        delete[] mTextures;

        // get rid of texture entries
        const uint32 numTextEntries = mTextEntries.GetLength();
        for (uint32 i = 0; i < numTextEntries; ++i)
        {
            delete mTextEntries[i];
        }
        mTextEntries.Clear();
    }


    // render texture
    void GLRenderUtil::RenderTexture(Texture* texture, const AZ::Vector2& pos)
    {
        mTextures[mNumTextures].pos     = pos;
        mTextures[mNumTextures].texture = texture;
        mNumTextures++;

        if (mNumTextures >= mMaxNumTextures)
        {
            RenderTextures();
        }
    }


    // render textures
    void GLRenderUtil::RenderTextures()
    {
        if (mNumTextures == 0)
        {
            return;
        }

        //MCore::Timer time;

        // use the ffp
        GetGraphicsManager()->SetShader(nullptr);

        // remember the rendering settings
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // get the viewport dimensions
        float viewportDimensions[4];
        glGetFloatv(GL_VIEWPORT, viewportDimensions);
        uint32 screenWidth  = (uint32)viewportDimensions[2];
        uint32 screenHeight = (uint32)viewportDimensions[3];

        // setup orthographic view
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, screenWidth, screenHeight, 0.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glColor3f(1.0f, 1.0f, 1.0f);

        // iterate through the textures and render them
        for (uint32 i = 0; i < mNumTextures; ++i)
        {
            TextureEntry&   e = mTextures[i];
            float           w = static_cast<float>(e.texture->GetWidth());
            float           h = static_cast<float>(e.texture->GetHeight());

            glBindTexture(GL_TEXTURE_2D, e.texture->GetID());

            glBegin(GL_QUADS);
            glTexCoord2f(0.0f,           0.0f);
            glVertex3f(e.pos.GetX(),       e.pos.GetY(),       -1.0f);
            glTexCoord2f(1.0f,           0.0f);
            glVertex3f(e.pos.GetX() + w,   e.pos.GetY(),       -1.0f);
            glTexCoord2f(1.0f,           1.0f);
            glVertex3f(e.pos.GetX() + w,   e.pos.GetY() + h,   -1.0f);
            glTexCoord2f(0.0f,           1.0f);
            glVertex3f(e.pos.GetX(),       e.pos.GetY() + h,   -1.0f);
            glEnd();
        }

        glPopAttrib();

        //const float renderTime = time.GetTime();
        //LOG("numTextures=%i, renderTime=%.3fms", mNumTextures, renderTime*1000);

        mNumTextures = 0;
    }


    // overloaded render lines function
    void GLRenderUtil::RenderLines(LineVertex* vertices, uint32 numVertices)
    {
        if (mLineShader == nullptr)
        {
            return;
        }

        VertexBuffer* vertexBuffer = mLineVertexBuffers[mCurrentLineVB];

        // copy the vertices into the OpenGL vertex buffer
        LineVertex* lineVertices = (LineVertex*)vertexBuffer->Lock();
        if (lineVertices)
        {
            MCore::MemCopy(lineVertices, vertices, sizeof(LineVertex) * numVertices);
        }
        vertexBuffer->Unlock();

        // activate vertex buffer, no indices
        vertexBuffer->Activate();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // setup the shader and render the lines
        mLineShader->Activate();

        mLineShader->SetAttribute("inPosition",   4, GL_FLOAT, sizeof(LineVertex), 0);
        mLineShader->SetAttribute("inColor",      4, GL_FLOAT, sizeof(LineVertex), sizeof(AZ::Vector3));
        mLineShader->SetUniform("matViewProj",  mGraphicsManager->GetCamera()->GetViewProjMatrix(), false);

        glDrawArrays(GL_LINES, 0, numVertices);

        mLineShader->Deactivate();
        GetGraphicsManager()->SetShader(nullptr);       // if only lines are rendered, we need to unbind this shader totally
        // otherwise it will stay active and another context can't use it
        vertexBuffer->Deactivate();

        mCurrentLineVB++;
        if (mCurrentLineVB >= MAX_LINE_VERTEXBUFFERS)
        {
            mCurrentLineVB = 0;
        }
    }


    // render 2D lines
    void GLRenderUtil::Render2DLines(Line2D* lines, uint32 numLines)
    {
        //MCore::Timer time;

        // remember the rendering settings
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);

        // get the viewport dimensions
        float viewportDimensions[4];
        glGetFloatv(GL_VIEWPORT, viewportDimensions);
        uint32 screenWidth  = (uint32)viewportDimensions[2];
        uint32 screenHeight = (uint32)viewportDimensions[3];

        // setup orthographic view
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, screenWidth, screenHeight, 0.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // use the fixed function pipeline
        mGraphicsManager->SetShader(nullptr);

        glBegin(GL_LINES);
        for (uint32 i = 0; i < numLines; ++i)
        {
            glColor3f(lines[i].mColor.r, lines[i].mColor.g, lines[i].mColor.b);
            glVertex3f(lines[i].mX1, lines[i].mY1, 0.0);
            glVertex3f(lines[i].mX2, lines[i].mY2, 0.0);
        }
        glEnd();

        glPopAttrib();

        //const float render2DTime = time.GetTime();
        //LOG("numLines=%i, renderTime=%.3fms", numLines, render2DTime*1000);
    }


    void GLRenderUtil::RenderBorderedRect(int32 left, int32 right, int32 top, int32 bottom, const MCore::RGBAColor& fillColor, const MCore::RGBAColor& borderColor)
    {
        // remember the rendering settings
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);

        // get the viewport dimensions
        float viewportDimensions[4];
        glGetFloatv(GL_VIEWPORT, viewportDimensions);
        uint32 screenWidth  = (uint32)viewportDimensions[2];
        uint32 screenHeight = (uint32)viewportDimensions[3];

        // setup orthographic view
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, screenWidth, screenHeight, 0.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // use the fixed function pipeline
        mGraphicsManager->SetShader(nullptr);

        glColor3f(fillColor.r, fillColor.g, fillColor.b);
        glBegin(GL_QUADS);
        glVertex3i(left, top, 0);
        glVertex3i(left, bottom, 0);
        glVertex3i(right, bottom, 0);
        glVertex3i(right, top, 0);
        glEnd();

        glPopAttrib();

        Render2DLine(static_cast<float>(left), static_cast<float>(top), static_cast<float>(right), static_cast<float>(top), borderColor);
        Render2DLine(static_cast<float>(left), static_cast<float>(top), static_cast<float>(left), static_cast<float>(bottom), borderColor);
        Render2DLine(static_cast<float>(left), static_cast<float>(bottom), static_cast<float>(right), static_cast<float>(bottom), borderColor);
        Render2DLine(static_cast<float>(right), static_cast<float>(top), static_cast<float>(right), static_cast<float>(bottom), borderColor);
    }


    // overloaded render util mesh function
    void GLRenderUtil::RenderUtilMesh(UtilMesh* mesh, const MCore::RGBAColor& color, const AZ::Transform& globalTM)
    {
        if (mMeshShader == nullptr)
        {
            return;
        }

        // lock the vertex and the index buffer
        UtilMeshVertex* vertices = (UtilMeshVertex*)mMeshVertexBuffer->Lock();
        uint32*         indices = (uint32*)mMeshIndexBuffer->Lock();

        // copy the vertices and the indices into the OpenGL buffers
        MCORE_ASSERT(mesh->mPositions.size() <= mNumMaxMeshVertices);
        MCore::MemCopy(indices, mesh->mIndices.data(), mesh->mIndices.size() * sizeof(uint32));

        if (mesh->mNormals.empty())
        {
            const size_t numVertices = mesh->mPositions.size();
            for (size_t i = 0; i < numVertices; ++i)
            {
                vertices[i].mPosition = mesh->mPositions[i];
                vertices[i].mNormal   = AZ::Vector3(1.0f, 0.0f, 0.0f);
            }
        }
        else
        {
            const size_t numVertices = mesh->mPositions.size();
            for (size_t i = 0; i < numVertices; ++i)
            {
                vertices[i].mPosition = mesh->mPositions[i];
                vertices[i].mNormal   = mesh->mNormals[i];
            }
        }

        // unlock and activate the vertex and the index buffer
        mMeshVertexBuffer->Unlock();
        mMeshIndexBuffer->Unlock();
        mMeshVertexBuffer->Activate();
        mMeshIndexBuffer->Activate();

        // setup shader
        mMeshShader->Activate();

        MCommon::Camera* camera = mGraphicsManager->GetCamera();
        const AZ::Matrix4x4 globalMatrix = AZ::Matrix4x4::CreateFromTransform(globalTM);
        mMeshShader->SetUniform("worldViewProjectionMatrix", camera->GetViewProjMatrix() * globalMatrix);
        mMeshShader->SetUniform("cameraPosition", camera->GetPosition());
        mMeshShader->SetUniform("lightDirection", MCore::GetUp(camera->GetViewMatrix().GetTranspose()).GetNormalized());   // This is GetUp() now, as lookat matrices always seem to use the z axis to point forward
        mMeshShader->SetUniform("diffuseColor", color);
        mMeshShader->SetUniform("specularColor", AZ::Vector3::CreateOne() * 0.3f);
        mMeshShader->SetUniform("specularPower", 8.0f);

        // setup shader attributes and draw the mesh
        const uint32 stride = sizeof(UtilMeshVertex);
        mMeshShader->SetAttribute("inPosition", 4, GL_FLOAT, stride, 0);
        mMeshShader->SetAttribute("inNormal", 4, GL_FLOAT, stride, sizeof(AZ::Vector3));
        mMeshShader->SetUniform("worldMatrix", globalMatrix);

        glDrawElements(GL_TRIANGLES, (GLsizei)mesh->mIndices.size(), GL_UNSIGNED_INT, (GLvoid*)nullptr);

        mMeshShader->Deactivate();
    }


    // overloaded RenderTriangle function
    void GLRenderUtil::RenderTriangle(const AZ::Vector3& v1, const AZ::Vector3& v2, const AZ::Vector3& v3, const MCore::RGBAColor& color)
    {
        glPushAttrib(GL_ENABLE_BIT);

        // load the camera view projection matrix
        glMatrixMode(GL_PROJECTION);
        MCommon::Camera* camera = mGraphicsManager->GetCamera();
        const AZ::Matrix4x4 transposedProjMatrix = camera->GetViewProjMatrix().GetTranspose();
        glLoadMatrixf((float*)&transposedProjMatrix);
    
        // reset the model view matrix
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // disable the shaders
        mGraphicsManager->SetShader(nullptr);

        // set up blending properties
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // render the triangle
        glColor4f(color.r, color.g, color.b, color.a);
        glBegin(GL_TRIANGLES);
        glVertex3f(v1.GetX(), v1.GetY(), v1.GetZ());
        glVertex3f(v2.GetX(), v2.GetY(), v2.GetZ());
        glVertex3f(v3.GetX(), v3.GetY(), v3.GetZ());
        glEnd();

        // disable blending again
        glDisable(GL_BLEND);

        glPopAttrib();
    }


    void GLRenderUtil::RenderTriangles(const MCore::Array<TriangleVertex>& triangleVertices)
    {
        // check if there are any triangles to render, if not return directly
        if (triangleVertices.GetIsEmpty())
        {
            return;
        }

        glDisable(GL_CULL_FACE);

        // get the number of vertices to render
        const uint32 numVertices = triangleVertices.GetLength();
        MCORE_ASSERT(numVertices <= mNumMaxTriangleVertices);

        // lock the vertex buffer
        TriangleVertex* vertices = (TriangleVertex*)mTriangleVertexBuffer->Lock();
        if (vertices == nullptr)
        {
            return;
        }

        // TODO: Not nice yet, get the color from the first vertex and use if for all triangles
        MCore::RGBAColor color((uint32)triangleVertices[0].mColor);

        // fill in the vertex buffer
        for (uint32 i = 0; i < numVertices; ++i)
        {
            vertices[i].mPosition = triangleVertices[i].mPosition;
            vertices[i].mNormal   = triangleVertices[i].mNormal;
        }

        // unlock and activate the vertex buffer and index buffer
        mTriangleVertexBuffer->Unlock();
        mTriangleVertexBuffer->Activate();
        mTriangleIndexBuffer->Activate();

        // setup shader
        mMeshShader->Activate();

        MCommon::Camera* camera = mGraphicsManager->GetCamera();

        mMeshShader->SetUniform("worldViewProjectionMatrix", camera->GetViewProjMatrix());
        mMeshShader->SetUniform("cameraPosition", camera->GetPosition());
        mMeshShader->SetUniform("lightDirection", MCore::GetUp(camera->GetViewMatrix().GetTranspose()).GetNormalized());
        mMeshShader->SetUniform("diffuseColor", color);
        mMeshShader->SetUniform("specularColor", AZ::Vector3::CreateOne());
        mMeshShader->SetUniform("specularPower", 30.0f);

        // setup shader attributes and draw the mesh
        const uint32 stride = sizeof(TriangleVertex);
        mMeshShader->SetAttribute("inPosition", 4, GL_FLOAT, stride, 0);
        mMeshShader->SetAttribute("inNormal", 4, GL_FLOAT, stride, sizeof(AZ::Vector3));
        mMeshShader->SetUniform("worldMatrix", AZ::Matrix4x4::CreateIdentity());

        glDrawElements(GL_TRIANGLES, numVertices, GL_UNSIGNED_INT, (GLvoid*)nullptr);

        mMeshShader->Deactivate();
    }


    void GLRenderUtil::RenderTextPeriod(uint32 x, uint32 y, const char* text, float lifeTime, const MCore::RGBAColor& color, float fontSize, bool centered)
    {
        TextEntry* textEntry = new TextEntry();
        textEntry->mX       = x;
        textEntry->mY       = y;
        textEntry->mText    = text;
        textEntry->mLifeTime = lifeTime;
        textEntry->mColor   = color;
        textEntry->mFontSize = fontSize;
        textEntry->mCentered = centered;

        mTextEntries.Add(textEntry);
    }


    void GLRenderUtil::RenderTextPeriods()
    {
        static AZ::Debug::Timer timer;
        const float timeDelta = static_cast<float>(timer.StampAndGetDeltaTimeInSeconds());
        for (uint32 i = 0; i < mTextEntries.GetLength(); )
        {
            TextEntry* textEntry = mTextEntries[i];
            RenderText(static_cast<float>(textEntry->mX), static_cast<float>(textEntry->mY), textEntry->mText.c_str(), textEntry->mColor, textEntry->mFontSize, textEntry->mCentered);

            textEntry->mLifeTime -= timeDelta;
            if (textEntry->mLifeTime < 0.0f)
            {
                delete textEntry;
                mTextEntries.Remove(i);
            }
            else
            {
                i++;
            }
        }
    }


    void GLRenderUtil::SetDepthMaskWrite(bool writeEnabled)
    {
        glDepthMask(writeEnabled);
    }


    void GLRenderUtil::EnableCulling(bool cullingEnabled)
    {
        if (cullingEnabled)
        {
            glCullFace(GL_BACK);
            glEnable(GL_CULL_FACE);
        }
        else
        {
            glCullFace(GL_BACK);
            glDisable(GL_CULL_FACE);
        }
    }


    bool GLRenderUtil::GetCullingEnabled()
    {
        return glIsEnabled(GL_CULL_FACE) != 0;
    }


    void GLRenderUtil::EnableLighting(bool lightingEnabled)
    {
        if (lightingEnabled)
        {
            glEnable(GL_LIGHTING);
        }
        else
        {
            glDisable(GL_LIGHTING);
        }
    }


    bool GLRenderUtil::GetLightingEnabled()
    {
        return glIsEnabled(GL_LIGHTING) != 0;
    }
} // namespace RenderGL
