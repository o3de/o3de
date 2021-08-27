/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __OPENGLRENDERUTIL_H
#define __OPENGLRENDERUTIL_H

#include "RenderGLConfig.h"
#include "../../Common/RenderUtil.h"

#include <AzCore/PlatformIncl.h>
#include <QOpenGLExtraFunctions>

namespace RenderGL
{
    // forward declaration
    class GraphicsManager;
    class GLSLShader;
    class VertexBuffer;
    class IndexBuffer;
    class Texture;

    class RENDERGL_API GLRenderUtil
        : public MCommon::RenderUtil
        , private QOpenGLExtraFunctions
    {
        MCORE_MEMORYOBJECTCATEGORY(GLRenderUtil, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        GLRenderUtil(GraphicsManager* graphicsManager);
        ~GLRenderUtil();

        void Init();
        void Validate() override;

        // render 2D rects
        void RenderBorderedRect(int32 left, int32 right, int32 top, int32 bottom, const MCore::RGBAColor& fillColor, const MCore::RGBAColor& borderColor) override;

        // render textures
        void RenderTexture(Texture* texture, const AZ::Vector2& pos);
        void RenderTextures();

        // line rendering
        void RenderLines(LineVertex* vertices, uint32 numVertices) override;
        void Render2DLines(Line2D* lines, uint32 numLines) override;

        // util mesh rendering
        void RenderUtilMesh(UtilMesh* mesh, const MCore::RGBAColor& color, const AZ::Transform& globalTM) override;
        bool GetIsMeshRenderingSupported() const override           { return true; }

        // triangle rendering
        void RenderTriangle(const AZ::Vector3& v1, const AZ::Vector3& v2, const AZ::Vector3& v3, const MCore::RGBAColor& color) override;
        void RenderTriangles(const AZStd::vector<TriangleVertex>& triangleVertices) override;

        // text rendering (do not use until really needed, needs to do runtime allocations)
        void RenderTextPeriod(uint32 x, uint32 y, const char* text, float lifeTime, const MCore::RGBAColor& color = MCore::RGBAColor(1.0f, 1.0f, 1.0f), float fontSize = 11.0f, bool centered = false);
        void RenderTextPeriods();

        // render flag helpers
        void EnableCulling(bool cullingEnabled) override;
        bool GetCullingEnabled() override;

        void EnableLighting(bool lightingEnabled) override;
        bool GetLightingEnabled() override;

        void SetDepthMaskWrite(bool writeEnabled) override;

    private:
        void CleanUp();

        #define MAX_LINE_VERTEXBUFFERS 2
        GraphicsManager*            m_graphicsManager;
        VertexBuffer*               m_lineVertexBuffers[MAX_LINE_VERTEXBUFFERS]{};
        uint16                      m_currentLineVb;
        GLSLShader*                 m_lineShader;
        GLSLShader*                 m_meshShader;
        VertexBuffer*               m_meshVertexBuffer;
        IndexBuffer*                m_meshIndexBuffer;

        // vertex and index buffers for rendering triangles
        VertexBuffer*               m_triangleVertexBuffer;
        IndexBuffer*                m_triangleIndexBuffer;

        // texture rendering
        struct TextureEntry
        {
            MCORE_MEMORYOBJECTCATEGORY(GLRenderUtil::TextureEntry, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);
            Texture*                m_texture;
            AZ::Vector2         m_pos;
            TextureEntry()
                : m_pos(0.0f, 0.0f)
                , m_texture(nullptr) {}
        };

        struct TextEntry
        {
            MCORE_MEMORYOBJECTCATEGORY(GLRenderUtil::TextEntry, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);
            uint32                  m_x;
            uint32                  m_y;
            AZStd::string           m_text;
            float                   m_lifeTime;
            MCore::RGBAColor        m_color;
            float                   m_fontSize;
            bool                    m_centered;
        };

        AZStd::vector<TextEntry*>    m_textEntries;
        TextureEntry*               m_textures;
        uint32                      m_numTextures;
        uint32                      m_maxNumTextures;
    };
} // namespace RenderGL


#endif
