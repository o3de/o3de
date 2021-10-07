/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        m_graphicsManager        = graphicsManager;
        m_lineShader             = nullptr;
        m_meshShader             = nullptr;
        m_meshVertexBuffer       = nullptr;
        m_meshIndexBuffer        = nullptr;

        m_triangleVertexBuffer    = nullptr;
        m_triangleIndexBuffer     = nullptr;

        m_currentLineVb          = 0;

        // initialize the vertex buffers and the shader used for line rendering
        for (VertexBuffer*& lineVertexBuffer : m_lineVertexBuffers)
        {
            lineVertexBuffer = new VertexBuffer();
            if (lineVertexBuffer->Init(sizeof(LineVertex), s_numMaxLineVertices, USAGE_DYNAMIC) == false)
            {
                MCore::LogError("[OpenGL]  Failed to create render utility line vertex buffer.");
                CleanUp();
            }
        }

        m_lineShader = graphicsManager->LoadShader("Line_VS.glsl", "Line_PS.glsl");

        // initialize the vertex and the index buffers as well as the shader used for rendering util meshes
        m_meshVertexBuffer   = new VertexBuffer();
        m_meshIndexBuffer    = new IndexBuffer();

        if (m_meshVertexBuffer->Init(sizeof(UtilMeshVertex), s_numMaxMeshVertices, USAGE_DYNAMIC) == false)
        {
            MCore::LogError("[OpenGL]  Failed to create render utility mesh vertex buffer.");
            CleanUp();
            return;
        }

        if (m_meshIndexBuffer->Init(IndexBuffer::INDEXSIZE_32BIT, s_numMaxMeshIndices, USAGE_DYNAMIC) == false)
        {
            MCore::LogError("[OpenGL]  Failed to create render utility mesh index buffer.");
            CleanUp();
            return;
        }

        m_meshShader = graphicsManager->LoadShader("RenderUtil_VS.glsl", "RenderUtil_PS.glsl");

        // initialize the triangle rendering buffers
        m_triangleVertexBuffer = new VertexBuffer();
        if (m_triangleVertexBuffer->Init(sizeof(TriangleVertex), s_numMaxTriangleVertices, USAGE_DYNAMIC) == false)
        {
            MCore::LogError("[OpenGL]  Failed to create triangle vertex buffer.");
            CleanUp();
            return;
        }

        m_triangleIndexBuffer = new IndexBuffer();
        if (m_triangleIndexBuffer->Init(IndexBuffer::INDEXSIZE_32BIT, s_numMaxTriangleVertices, USAGE_STATIC) == false)
        {
            MCore::LogError("[OpenGL]  Failed to create triangle index buffer.");
            CleanUp();
            return;
        }

        // lock the index buffer and fill in the static indices
        uint32* indices = (uint32*)m_triangleIndexBuffer->Lock();
        if (indices)
        {
            for (uint32 i = 0; i < s_numMaxTriangleVertices; ++i)
            {
                indices[i] = i;
            }

            m_triangleIndexBuffer->Unlock();
        }

        // texture rendering
        m_maxNumTextures     = 256;
        m_numTextures        = 0;
        m_textures           = new TextureEntry[m_maxNumTextures];

        // text rendering
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
        if (m_lineShader)
        {
            m_lineShader->Validate();
        }
        if (m_meshShader)
        {
            m_meshShader->Validate();
        }
    }

    // destroy the allocated memory
    void GLRenderUtil::CleanUp()
    {
        for (VertexBuffer*& lineVertexBuffer : m_lineVertexBuffers)
        {
            delete lineVertexBuffer;
            lineVertexBuffer = nullptr;
        }

        delete m_meshVertexBuffer;
        delete m_meshIndexBuffer;

        delete m_triangleVertexBuffer;
        delete m_triangleIndexBuffer;

        m_meshVertexBuffer   = nullptr;
        m_meshIndexBuffer    = nullptr;

        m_triangleVertexBuffer   = nullptr;
        m_triangleIndexBuffer    = nullptr;

        m_currentLineVb      = 0;

        // get rid of the texture entries
        delete[] m_textures;

        // get rid of texture entries
        for (TextEntry* textEntry : m_textEntries)
        {
            delete textEntry;
        }
        m_textEntries.clear();
    }


    // render texture
    void GLRenderUtil::RenderTexture(Texture* texture, const AZ::Vector2& pos)
    {
        m_textures[m_numTextures].m_pos     = pos;
        m_textures[m_numTextures].m_texture = texture;
        m_numTextures++;

        if (m_numTextures >= m_maxNumTextures)
        {
            RenderTextures();
        }
    }


    // render textures
    void GLRenderUtil::RenderTextures()
    {
        if (m_numTextures == 0)
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
        for (uint32 i = 0; i < m_numTextures; ++i)
        {
            TextureEntry&   e = m_textures[i];
            float           w = static_cast<float>(e.m_texture->GetWidth());
            float           h = static_cast<float>(e.m_texture->GetHeight());

            glBindTexture(GL_TEXTURE_2D, e.m_texture->GetID());

            glBegin(GL_QUADS);
            glTexCoord2f(0.0f,           0.0f);
            glVertex3f(e.m_pos.GetX(),       e.m_pos.GetY(),       -1.0f);
            glTexCoord2f(1.0f,           0.0f);
            glVertex3f(e.m_pos.GetX() + w,   e.m_pos.GetY(),       -1.0f);
            glTexCoord2f(1.0f,           1.0f);
            glVertex3f(e.m_pos.GetX() + w,   e.m_pos.GetY() + h,   -1.0f);
            glTexCoord2f(0.0f,           1.0f);
            glVertex3f(e.m_pos.GetX(),       e.m_pos.GetY() + h,   -1.0f);
            glEnd();
        }

        glPopAttrib();

        m_numTextures = 0;
    }


    // overloaded render lines function
    void GLRenderUtil::RenderLines(LineVertex* vertices, uint32 numVertices)
    {
        if (m_lineShader == nullptr)
        {
            return;
        }

        VertexBuffer* vertexBuffer = m_lineVertexBuffers[m_currentLineVb];

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
        m_lineShader->Activate();

        m_lineShader->SetAttribute("inPosition",   4, GL_FLOAT, sizeof(LineVertex), 0);
        m_lineShader->SetAttribute("inColor",      4, GL_FLOAT, sizeof(LineVertex), sizeof(AZ::Vector3));
        m_lineShader->SetUniform("matViewProj",  m_graphicsManager->GetCamera()->GetViewProjMatrix(), false);

        glDrawArrays(GL_LINES, 0, numVertices);

        m_lineShader->Deactivate();
        GetGraphicsManager()->SetShader(nullptr);       // if only lines are rendered, we need to unbind this shader totally
        // otherwise it will stay active and another context can't use it
        vertexBuffer->Deactivate();

        m_currentLineVb++;
        if (m_currentLineVb >= MAX_LINE_VERTEXBUFFERS)
        {
            m_currentLineVb = 0;
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
        m_graphicsManager->SetShader(nullptr);

        glBegin(GL_LINES);
        for (uint32 i = 0; i < numLines; ++i)
        {
            glColor3f(lines[i].m_color.m_r, lines[i].m_color.m_g, lines[i].m_color.m_b);
            glVertex3f(lines[i].m_x1, lines[i].m_y1, 0.0);
            glVertex3f(lines[i].m_x2, lines[i].m_y2, 0.0);
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
        m_graphicsManager->SetShader(nullptr);

        glColor3f(fillColor.m_r, fillColor.m_g, fillColor.m_b);
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
        if (m_meshShader == nullptr)
        {
            return;
        }

        // lock the vertex and the index buffer
        UtilMeshVertex* vertices = (UtilMeshVertex*)m_meshVertexBuffer->Lock();
        uint32*         indices = (uint32*)m_meshIndexBuffer->Lock();

        // copy the vertices and the indices into the OpenGL buffers
        MCORE_ASSERT(mesh->m_positions.size() <= s_numMaxMeshVertices);
        MCore::MemCopy(indices, mesh->m_indices.data(), mesh->m_indices.size() * sizeof(uint32));

        if (mesh->m_normals.empty())
        {
            const size_t numVertices = mesh->m_positions.size();
            for (size_t i = 0; i < numVertices; ++i)
            {
                vertices[i].m_position = mesh->m_positions[i];
                vertices[i].m_normal   = AZ::Vector3(1.0f, 0.0f, 0.0f);
            }
        }
        else
        {
            const size_t numVertices = mesh->m_positions.size();
            for (size_t i = 0; i < numVertices; ++i)
            {
                vertices[i].m_position = mesh->m_positions[i];
                vertices[i].m_normal   = mesh->m_normals[i];
            }
        }

        // unlock and activate the vertex and the index buffer
        m_meshVertexBuffer->Unlock();
        m_meshIndexBuffer->Unlock();
        m_meshVertexBuffer->Activate();
        m_meshIndexBuffer->Activate();

        // setup shader
        m_meshShader->Activate();

        MCommon::Camera* camera = m_graphicsManager->GetCamera();
        const AZ::Matrix4x4 globalMatrix = AZ::Matrix4x4::CreateFromTransform(globalTM);
        m_meshShader->SetUniform("worldViewProjectionMatrix", camera->GetViewProjMatrix() * globalMatrix);
        m_meshShader->SetUniform("cameraPosition", camera->GetPosition());
        m_meshShader->SetUniform("lightDirection", MCore::GetUp(camera->GetViewMatrix().GetTranspose()).GetNormalized());   // This is GetUp() now, as lookat matrices always seem to use the z axis to point forward
        m_meshShader->SetUniform("diffuseColor", color);
        m_meshShader->SetUniform("specularColor", AZ::Vector3::CreateOne() * 0.3f);
        m_meshShader->SetUniform("specularPower", 8.0f);

        // setup shader attributes and draw the mesh
        const uint32 stride = sizeof(UtilMeshVertex);
        m_meshShader->SetAttribute("inPosition", 4, GL_FLOAT, stride, 0);
        m_meshShader->SetAttribute("inNormal", 4, GL_FLOAT, stride, sizeof(AZ::Vector3));
        m_meshShader->SetUniform("worldMatrix", globalMatrix);

        glDrawElements(GL_TRIANGLES, (GLsizei)mesh->m_indices.size(), GL_UNSIGNED_INT, (GLvoid*)nullptr);

        m_meshShader->Deactivate();
    }


    // overloaded RenderTriangle function
    void GLRenderUtil::RenderTriangle(const AZ::Vector3& v1, const AZ::Vector3& v2, const AZ::Vector3& v3, const MCore::RGBAColor& color)
    {
        glPushAttrib(GL_ENABLE_BIT);

        // load the camera view projection matrix
        glMatrixMode(GL_PROJECTION);
        MCommon::Camera* camera = m_graphicsManager->GetCamera();
        const AZ::Matrix4x4 transposedProjMatrix = camera->GetViewProjMatrix().GetTranspose();
        glLoadMatrixf((float*)&transposedProjMatrix);
    
        // reset the model view matrix
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // disable the shaders
        m_graphicsManager->SetShader(nullptr);

        // set up blending properties
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // render the triangle
        glColor4f(color.m_r, color.m_g, color.m_b, color.m_a);
        glBegin(GL_TRIANGLES);
        glVertex3f(v1.GetX(), v1.GetY(), v1.GetZ());
        glVertex3f(v2.GetX(), v2.GetY(), v2.GetZ());
        glVertex3f(v3.GetX(), v3.GetY(), v3.GetZ());
        glEnd();

        // disable blending again
        glDisable(GL_BLEND);

        glPopAttrib();
    }


    void GLRenderUtil::RenderTriangles(const AZStd::vector<TriangleVertex>& triangleVertices)
    {
        // check if there are any triangles to render, if not return directly
        if (triangleVertices.empty())
        {
            return;
        }

        glDisable(GL_CULL_FACE);

        // get the number of vertices to render
        const uint32 numVertices = aznumeric_caster(triangleVertices.size());
        MCORE_ASSERT(numVertices <= s_numMaxTriangleVertices);

        // lock the vertex buffer
        TriangleVertex* vertices = (TriangleVertex*)m_triangleVertexBuffer->Lock();
        if (vertices == nullptr)
        {
            return;
        }

        // TODO: Not nice yet, get the color from the first vertex and use if for all triangles
        MCore::RGBAColor color((uint32)triangleVertices[0].m_color);

        // fill in the vertex buffer
        for (uint32 i = 0; i < numVertices; ++i)
        {
            vertices[i].m_position = triangleVertices[i].m_position;
            vertices[i].m_normal   = triangleVertices[i].m_normal;
        }

        // unlock and activate the vertex buffer and index buffer
        m_triangleVertexBuffer->Unlock();
        m_triangleVertexBuffer->Activate();
        m_triangleIndexBuffer->Activate();

        // setup shader
        m_meshShader->Activate();

        MCommon::Camera* camera = m_graphicsManager->GetCamera();

        m_meshShader->SetUniform("worldViewProjectionMatrix", camera->GetViewProjMatrix());
        m_meshShader->SetUniform("cameraPosition", camera->GetPosition());
        m_meshShader->SetUniform("lightDirection", MCore::GetUp(camera->GetViewMatrix().GetTranspose()).GetNormalized());
        m_meshShader->SetUniform("diffuseColor", color);
        m_meshShader->SetUniform("specularColor", AZ::Vector3::CreateOne());
        m_meshShader->SetUniform("specularPower", 30.0f);

        // setup shader attributes and draw the mesh
        const uint32 stride = sizeof(TriangleVertex);
        m_meshShader->SetAttribute("inPosition", 4, GL_FLOAT, stride, 0);
        m_meshShader->SetAttribute("inNormal", 4, GL_FLOAT, stride, sizeof(AZ::Vector3));
        m_meshShader->SetUniform("worldMatrix", AZ::Matrix4x4::CreateIdentity());

        glDrawElements(GL_TRIANGLES, numVertices, GL_UNSIGNED_INT, (GLvoid*)nullptr);

        m_meshShader->Deactivate();
    }


    void GLRenderUtil::RenderTextPeriod(uint32 x, uint32 y, const char* text, float lifeTime, const MCore::RGBAColor& color, float fontSize, bool centered)
    {
        TextEntry* textEntry = new TextEntry();
        textEntry->m_x       = x;
        textEntry->m_y       = y;
        textEntry->m_text    = text;
        textEntry->m_lifeTime = lifeTime;
        textEntry->m_color   = color;
        textEntry->m_fontSize = fontSize;
        textEntry->m_centered = centered;

        m_textEntries.emplace_back(textEntry);
    }


    void GLRenderUtil::RenderTextPeriods()
    {
        static AZ::Debug::Timer timer;
        const float timeDelta = static_cast<float>(timer.StampAndGetDeltaTimeInSeconds());
        for (uint32 i = 0; i < m_textEntries.size(); )
        {
            TextEntry* textEntry = m_textEntries[i];
            RenderText(static_cast<float>(textEntry->m_x), static_cast<float>(textEntry->m_y), textEntry->m_text.c_str(), textEntry->m_color, textEntry->m_fontSize, textEntry->m_centered);

            textEntry->m_lifeTime -= timeDelta;
            if (textEntry->m_lifeTime < 0.0f)
            {
                delete textEntry;
                m_textEntries.erase(AZStd::next(begin(m_textEntries), i));
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
