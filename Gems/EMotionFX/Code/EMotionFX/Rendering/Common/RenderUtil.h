/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector2.h>
#include <MCore/Source/AzCoreConversions.h>
#include <MCore/Source/Vector.h>
#include <MCore/Source/Color.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Mesh.h>
#include "Camera.h"
#include "MCommonConfig.h"


namespace MCommon
{
    /**
     * class for the colors of the manipulator gizmos.
     */
    class MCOMMON_API ManipulatorColors
    {
        MCORE_MEMORYOBJECTCATEGORY(ManipulatorColors, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);

        ManipulatorColors() {}
        ~ManipulatorColors() {}

    public:
        // colors
        static MCore::RGBAColor s_selectionColor;
        static MCore::RGBAColor s_selectionColorDarker;
        static MCore::RGBAColor s_red;
        static MCore::RGBAColor s_green;
        static MCore::RGBAColor s_blue;
    };


    /**
     * The render util is an quick and easy to implement rendering independent helper class which makes it easy
     * to render character skeletons, vertex and face normals, tangents, bounding boxes, OOBs, wireframe and collision meshes.
     * All you have to do is deriving from this class and overloading the RenderLines() function.
     * It also supports some more advanced skeleton rendering using spheres and cylinders. This will require you to overload another two functions:
     * One to actually render the small helper meshes and the other one is just returning a simple flag if mesh rendering is supported. Mesh rendering is
     * disabled on default and the fallback line based rendering will be used.
     */
    class MCOMMON_API RenderUtil
    {
        MCORE_MEMORYOBJECTCATEGORY(RenderUtil, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);

    public:
        /**
         * Default constructor.
         */
        RenderUtil();

        /**
         * Destructor.
         */
        virtual ~RenderUtil();

        virtual void Validate() {};

        /**
         * Render simple finite grid.
         * @param start The start vector for the grid area to render.
         * @param end The end vector for the grid area to render.
         * @param normal The normal for the grid. In case you want to draw a grid on the ground it is 0, 1, 0.
         * @param scale The scaling value with which the grid should be scaled.
         * @param mainAxisColor The color of the main coordinate axis.
         * @param gridColor The color of every fifth grid line.
         * @param subStepColor The color of the sub step grid lines.
         * @param directlyRender Will call the RenderLines() function internally in case it is set to true. If false
         *                       you have to make sure to call RenderLines() manually at the end of your custom render frame function.
         */
        void RenderGrid(AZ::Vector2 start, AZ::Vector2 end, const AZ::Vector3& normal, float scale = 25.0f, const MCore::RGBAColor& mainAxisColor = MCore::RGBAColor(0.0f, 0.01f, 0.04f), const MCore::RGBAColor& gridColor = MCore::RGBAColor(0.3242f, 0.3593f, 0.40625f), const MCore::RGBAColor& subStepColor = MCore::RGBAColor(0.2460f, 0.2851f, 0.3320f), bool directlyRender = false);

        /**
         * Render wireframe mesh.
         * @param mesh A pointer to the mesh which will be rendered.
         * @param worldTM The world space transformation matrix of the node to which the given mesh belongs to.
         * @param color The color for the wireframe mesh.
         * @param directlyRender Will call the RenderLines() function internally in case it is set to true. If false
         *                       you have to make sure to call RenderLines() manually at the end of your custom render frame function.
         * @param offsetScale The offset scaling factor. The wired mesh is offset slightly from the mesh, to prevent some z-fighting.
         */
        void RenderWireframe(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM, const MCore::RGBAColor& color = MCore::RGBAColor(0.80784315f, 0.23921569f, 0.88627452f, 1.0f), bool directlyRender = false, float offsetScale = 1.0f);

        /**
         * Render vertex and/or face normals of the mesh.
         * @param mesh A pointer to the mesh which will be rendered.
         * @param worldTM The world space transformation matrix of the node to which the given mesh belongs to.
         * @param vertexNormals Set to true in case you want the vertex normals to be rendered, false if not.
         * @param faceNormals Face normals will be rendered in case this flag is set to true, they won't be rendered if it is set to false.
         * @param vertexNormalsScale This parameter controls the length of the vertex normals. The default size of the vertex normals is one unit.
         * @param faceNormalsScale This parameter controls the length of the face normals. The default size of the face normals is two units.
         * @param colorVertexNormals The color of the vertex normals.
         * @param colorFaceNormals The color of the face normals.
         * @param directlyRender Will call the RenderLines() function internally in case it is set to true. If false
         *                       you have to make sure to call RenderLines() manually at the end of your custom render frame function.
         */
        void RenderNormals(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM, bool vertexNormals, bool faceNormals, float vertexNormalsScale = 1.0f, float faceNormalsScale = 2.0f, const MCore::RGBAColor& colorVertexNormals = MCore::RGBAColor(0.0f, 1.0f, 0.0f), const MCore::RGBAColor& colorFaceNormals = MCore::RGBAColor(0.5f, 0.5f, 1.0f), bool directlyRender = false);

        /**
         * Render tangents and bitangents of the mesh.
         * @param mesh A pointer to the mesh which will be rendered.
         * @param worldTM The world space transformation matrix of the node to which the given mesh belongs to.
         * @param scale This parameter controls the length of the tangents and bitangents. The default size of the tangents and bitangents is one unit.
         * @param colorTangents The color of the tangents.
         * @param mirroredBitangentColor The color of the mirrored bitangents, so the ones that have a w value of -1.
         * @param colorBitangent The color of the face bitangents.
         * @param directlyRender Will call the RenderLines() function internally in case it is set to true. If false
         *                       you have to make sure to call RenderLines() manually at the end of your custom render frame function.
         */
        void RenderTangents(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM, float scale = 1.0f, const MCore::RGBAColor& colorTangents = MCore::RGBAColor(1.0f, 0.0f, 0.0f), const MCore::RGBAColor& mirroredBitangentColor = MCore::RGBAColor(1.0f, 1.0f, 0.0f), const MCore::RGBAColor& colorBitangents = MCore::RGBAColor(1.0f, 1.0f, 1.0f), bool directlyRender = false);

        /**
         * Render the given axis aligned bounding box.
         * @param box The box to be rendered.
         * @param color The color of the bounding box.
         * @param directlyRender Will call the RenderLines() function internally in case it is set to true. If false
         *                       you have to make sure to call RenderLines() manually at the end of your custom render frame function.
         */
        void RenderAabb(const AZ::Aabb& box, const MCore::RGBAColor& color, bool directlyRender = false);

        /**
         * Render a selection gizmo around the given axis aligned bounding box.
         * @param box The box for which we want to see a selection gizmo.
         * @param color The color of the selection representation.
         * @param directlyRender Will call the RenderLines() function internally in case it is set to true. If false
         *                       you have to make sure to call RenderLines() manually at the end of your custom render frame function.
         */
        void RenderSelection(const AZ::Aabb& box, const MCore::RGBAColor& color, bool directlyRender = false);

        /**
         * The render settings used to enable the different AABB types of an actor instance.
         */
        struct MCOMMON_API AABBRenderSettings
        {
            MCORE_MEMORYOBJECTCATEGORY(RenderUtil::AABBRenderSettings, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);

            /**
             * Constructor.
             */
            AABBRenderSettings();

            bool                m_nodeBasedAabb;             /**< Enable in case you want to render the node based AABB (default=true). */
            bool                m_meshBasedAabb;             /**< Enable in case you want to render the mesh based AABB (default=true). */
            bool                m_staticBasedAabb;           /**< Enable in case you want to render the static based AABB (default=true). */
            MCore::RGBAColor    m_nodeBasedColor;            /**< The color of the node based AABB. */
            MCore::RGBAColor    m_meshBasedColor;            /**< The color of the mesh based AABB. */
            MCore::RGBAColor    m_staticBasedColor;          /**< The color of the static based AABB. */
        };

        /**
         * Render the AABBs of an actor instance based on the given render settings.
         * @param actorInstance A pointer to the actor instance for which we will calculate and render the AABBs.
         * @param renderSettings The render settings contain information about whether to render a given AABB type or not and the colors of them. In the case this parameter will be left
         *                       empty the node based, the mesh based and the collision mesh based AABBs will be rendered using default colors.
         * @param directlyRender Will call the RenderLines() function internally in case it is set to true. If false
         *                       you have to make sure to call RenderLines() manually at the end of your custom render frame function.
         */
        void RenderAabbs(EMotionFX::ActorInstance* actorInstance, const AABBRenderSettings& renderSettings = AABBRenderSettings(), bool directlyRender = false);

        /**
         * Render a simple line based skeleton for all enabled nodes of the actor instance.
         * @param[in] actorInstance A pointer to the actor instance which will be rendered.
         * @param[in] visibleJointIndices List of visible joint indices. nullptr in case all joints should be rendered.
         * @param[in] selectedJointIndices List of selected joint indices. nullptr in case selection should not be considered.
         * @param[in] color The color of the skeleton lines.
         * @param[in] selectedColor The color of the selected bones.
         * @param[in] directlyRender Will call the RenderLines() function internally in case it is set to true. If false
         *                           you have to make sure to call RenderLines() manually at the end of your custom render frame function.
         */
        void RenderSimpleSkeleton(EMotionFX::ActorInstance* actorInstance, const AZStd::unordered_set<size_t>* visibleJointIndices = nullptr, const AZStd::unordered_set<size_t>* selectedJointIndices = nullptr,
            const MCore::RGBAColor& color = MCore::RGBAColor(1.0f, 1.0f, 0.0f, 1.0f), const MCore::RGBAColor& selectedColor = MCore::RGBAColor(1.0f, 0.647f, 0.0f),
            float jointSphereRadius = 0.1f, bool directlyRender = false);

        /**
         * Render advanced skeleton using spheres and cylinders.
         * In case mesh rendering is not supported by the used render util the simple line based skeleton will be used as fallback function.
         * @param[in] actorInstance A pointer to the actor instance which will be rendered.
         * @param[in] boneList An array containing all node indices that are bones (used by the skin).
         * @param[in] visibleJointIndices List of visible joint indices. nullptr in case all joints should be rendered.
         * @param[in] selectedJointIndices List of selected joint indices. nullptr in case selection should not be considered.
         * @param[in] color The desired skeleton color.
         * @param[in] selectedColor The color of the selected bones.
         */
        void RenderSkeleton(EMotionFX::ActorInstance* actorInstance, const AZStd::vector<size_t>& boneList, const AZStd::unordered_set<size_t>* visibleJointIndices = nullptr, const AZStd::unordered_set<size_t>* selectedJointIndices = nullptr, const MCore::RGBAColor& color = MCore::RGBAColor(1.0f, 0.0f, 0.0f, 1.0f), const MCore::RGBAColor& selectedColor = MCore::RGBAColor(1.0f, 0.647f, 0.0f));

        /**
         * Render node orientations.
         * @param[in] actorInstance A pointer to the actor instance which will be rendered.
         * @param[in] boneList An array containing all node indices that are bones (used by the skin).
         * @param[in] visibleJointIndices List of visible joint indices. nullptr in case all joints should be rendered.
         * @param[in] selectedJointIndices List of selected joint indices. nullptr in case selection should not be considered.
         * @param[in] scale The scaling value in units. Axes of normal nodes will use the scaling value as unit length, skinned bones will use the scaling value as multiplier.
         * @param[in] scaleBonesOnLength Automatically scales the bone orientations based on the bone length. This means finger node orientations will be rendered smaller than foot bones as the bone length is a lot smaller as well.
         */
        void RenderNodeOrientations(EMotionFX::ActorInstance* actorInstance, const AZStd::vector<size_t>& boneList, const AZStd::unordered_set<size_t>* visibleJointIndices = nullptr, const AZStd::unordered_set<size_t>* selectedJointIndices = nullptr, float scale = 1.0f, bool scaleBonesOnLength = true);

        /**
         * Render the bind pose of the given actor.
         * @param[in] actorInstance A pointer to the actor instance whose bind pose will be rendered.
         * @param[in] color The color of the skeleton lines.
         * @param[in] directlyRender Will call the RenderLines() function internally in case it is set to true. If false
         *                           you have to make sure to call RenderLines() manually at the end of your custom render frame function.
         */
        void RenderBindPose(EMotionFX::ActorInstance* actorInstance, const MCore::RGBAColor& color = MCore::RGBAColor(1.0f, 1.0f, 0.0f, 1.0f), bool directlyRender = false);

        /**
         * Render the node names of the given actor instance.
         * @param[in] actorInstance The actor instance to render the node names for.
         * @param[in] camera A pointer to the current camera used to render the scene.
         * @param[in] screenWidth The screen width in pixels.
         * @param[in] screenHeight The screen height in pixels.
         * @param[in] color The text color.
         * @param[in] color The text color for selected elements.
         * @param[in] visibleJointIndices List of visible joint indices. nullptr in case all joints should be rendered.
         * @param[in] selectedJointIndices List of selected joint indices. nullptr in case selection should not be considered.
         */
        void RenderNodeNames(EMotionFX::ActorInstance* actorInstance, MCommon::Camera* camera, uint32 screenWidth, uint32 screenHeight, const MCore::RGBAColor& color, const MCore::RGBAColor& selectedColor, const AZStd::unordered_set<size_t>& visibleJointIndices, const AZStd::unordered_set<size_t>& selectedJointIndices);

        /**
         * Render a sphere.
         * @param position The location of the sphere center.
         * @param radius The size of the sphere.
         * @param color The desired sphere color.
         */
        void RenderSphere(const AZ::Vector3& position, float radius, const MCore::RGBAColor& color);

        /**
         * Render a sphere.
         * @param color The desired sphere color.
         * @param worldTM The world space transformation matrix.
         */
        MCORE_INLINE void RenderSphere(const MCore::RGBAColor& color, const AZ::Transform& worldTM)                                                            { RenderUtilMesh(m_unitSphereMesh, color, worldTM); }

        /**
         * Render a circle, consisting of lines.
         * @param position The center of the circle.
         * @param rotation vector (euler angles).
         * @param radius The radius of the circle.
         * @param color The color of the circle.
         */
        void RenderCircle(const AZ::Transform& rotation, float radius, uint32 numSegments, const MCore::RGBAColor& color, float startAngle = 0.0f, float endAngle = MCore::Math::twoPi, bool fillCircle = false, const MCore::RGBAColor& fillColor = MCore::RGBAColor(), bool cullFaces = false, const AZ::Vector3& camRollAxis = AZ::Vector3::CreateZero());

        /**
         * Render a cube.
         * @param color The desired cube color.
         * @param worldTM The world space transformation matrix.
         */
        MCORE_INLINE void RenderCube(const MCore::RGBAColor& color, const AZ::Transform& worldTM)                                                          { RenderUtilMesh(m_unitCubeMesh, color, worldTM); }

        /**
         * Render a cylinder.
         * @param baseRadius The radius of the bottom of the cylinder.
         * @param topRadius The radius of the top of the cylinder.
         * @param length The height of the cylinder.
         * @param color The desired cylinder color.
         * @param worldTM The world space transformation matrix.
         */
        MCORE_INLINE void RenderCylinder(float baseRadius, float topRadius, float length, const MCore::RGBAColor& color, const AZ::Transform& worldTM)     { FillCylinder(m_cylinderMesh, baseRadius, topRadius, length); RenderUtilMesh(m_cylinderMesh, color, worldTM); }

        /**
         * Render a cylinder.
         * @param baseRadius The radius of the bottom of the cylinder.
         * @param topRadius The radius of the top of the cylinder.
         * @param length The height of the cylinder.
         * @param position The position of the cylinder base.
         * @param direction The direction towards the cylinder will be oriented.
         * @param color The desired cylinder color.
         */
        void RenderCylinder(float baseRadius, float topRadius, float length, const AZ::Vector3& position, const AZ::Vector3& direction, const MCore::RGBAColor& color);

        /**
         * Render a triangle (CCW).
         * @param v1 The first corner of the triangle.
         * @param v2 The second corner of the triangle.
         * @param v3 The third corner of the triangle.
         */
        virtual void RenderTriangle(const AZ::Vector3& v1, const AZ::Vector3& v2, const AZ::Vector3& v3, const MCore::RGBAColor& color) = 0;

        /**
         * Render a 2D quad defined by four values.
         * @param left The left coordinate of the quad.
         * @param right The right coordinate of the quad.
         * @param top The top coordinate of the quad.
         * @param bottom The bottom coordinate of the quad.
         */
        virtual void RenderBorderedRect(int32 left, int32 right, int32 top, int32 bottom, const MCore::RGBAColor& fillColor, const MCore::RGBAColor& borderColor) = 0;

        /**
         * Render an arrow head.
         * @param height The height of the arrow head from the base to the head.
         * @param radius The radius of the base of the arrow head.
         * @param color The desired arrow head color.
         * @param worldTM The world space transformation matrix.
         */
        MCORE_INLINE void RenderArrowHead(float height, float radius, const MCore::RGBAColor& color, const AZ::Transform& worldTM)             { FillArrowHead(m_arrowHeadMesh, height, radius); RenderUtilMesh(m_arrowHeadMesh, color, worldTM); }

        /**
         * Render an arrow head.
         * @param height The height of the arrow head from the base to the head.
         * @param radius The radius of the base of the arrow head.
         * @param position The position of the arrow head base.
         * @param direction The direction towards the arrow head will be oriented.
         * @param color The desired arrow head color.
         */
        void RenderArrowHead(float height, float radius, const AZ::Vector3& position, const AZ::Vector3& direction, const MCore::RGBAColor& color);

        /**
         * Render a solid mesh based orientated axis. This can be used to visualize coordinate systems for example a node orientation in world space.
         * @param size The size value in units is used to control the scaling of the axis.
         * @param position The center point of the axis in world space.
         * @param right The normalized right axis.
         * @param up The normalized up axis.
         * @param forward The normalized forward axis.
         */
        void RenderAxis(float size, const AZ::Vector3& position, const AZ::Vector3& right, const AZ::Vector3& up, const AZ::Vector3& forward);

        /**
         * Axis rendering settings.
         */
        struct MCOMMON_API AxisRenderingSettings
        {
            MCORE_MEMORYOBJECTCATEGORY(RenderUtil::AxisRenderingSettings, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);

            /**
             * Constructor.
             */
            AxisRenderingSettings();

            AZ::Transform   m_worldTm;           /**< The world space transformation matrix to visualize. */
            AZ::Vector3     m_cameraRight;       /**< The inverse of the camera's right vector used for billboarding the axis names. */
            AZ::Vector3     m_cameraUp;          /**< The inverse of the camera's up vector used for billboarding the axis names. */
            float           m_size;              /**< The size value in units is used to control the scaling of the axis. */
            bool            m_renderXAxis;       /**< Set to true if you want to render the x axis, false if the x axis should be skipped. */
            bool            m_renderYAxis;       /**< Set to true if you want to render the y axis, false if the y axis should be skipped. */
            bool            m_renderZAxis;       /**< Set to true if you want to render the z axis, false if the z axis should be skipped. */
            bool            m_renderXAxisName;   /**< Set to true if you want to render the name of the x axis. The name will only be rendered if the axis itself will be rendered as well. */
            bool            m_renderYAxisName;   /**< Set to true if you want to render the name of the y axis. The name will only be rendered if the axis itself will be rendered as well. */
            bool            m_renderZAxisName;   /**< Set to true if you want to render the name of the z axis. The name will only be rendered if the axis itself will be rendered as well. */
            bool            m_selected;          /**< Set to true if you want to render the axis using the selection color. */
        };

        /**
         * Render a line based orientated axis. This function will visualize the given world space matrix.
         * @param settings The axis rendering settings to be used.
         */
        void RenderLineAxis(const AxisRenderingSettings& settings);

        void RenderArrow(float size, const AZ::Vector3& position, const AZ::Vector3& direction, const MCore::RGBAColor& color);

        /**
         * Render a solid mesh based orientated axis. This function will visualize the given world space matrix.
         * @param size The size value in units is used to control the scaling of the axis.
         * @param worldTM The world space transformation matrix to visualize.
         */
        MCORE_INLINE void RenderAxis(float size, const AZ::Transform& worldTM) { RenderAxis(size, worldTM.GetTranslation(), MCore::GetRight(worldTM), MCore::GetUp(worldTM), MCore::GetForward(worldTM)); }

        //---------------------------------------------------------------------------------------------

        /**
         * The vertex structure to be used for rendering lines.
         */
        struct MCOMMON_API LineVertex
        {
            MCORE_MEMORYOBJECTCATEGORY(RenderUtil::LineVertex, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);
            AZ::Vector3         m_position;  /**< The position of the vertex. */
            MCore::RGBAColor    m_color;     /**< The vertex color. */
        };

        /**
         * Render a single line.
         * @param v1 The starting point of the line.
         * @param v2 The ending point of the line.
         * @param color The color of the line.
         */
        MCORE_INLINE void RenderLine(const AZ::Vector3& v1, const AZ::Vector3& v2, const MCore::RGBAColor& color)
        {
            m_vertexBuffer[m_numVertices].m_position = v1;
            m_vertexBuffer[m_numVertices + 1].m_position = v2;
            m_vertexBuffer[m_numVertices].m_color = color;
            m_vertexBuffer[m_numVertices + 1].m_color = color;
            m_numVertices += 2;
            if (m_numVertices >= s_numMaxLineVertices)
            {
                RenderLines();
            }
        }

        /**
         * Render the given lines.
         * This function needs to be implemented by the user. The whole RenderUtil class is render engine independent.
         * @param vertices A pointer to the vertex array to be rendered.
         * @param numVertices The number of vertices in the vertex array. The number of vertices must be a multiplier of two, uneven numbers are not valid.
         */
        virtual void RenderLines(LineVertex* vertices, uint32 numVertices) = 0;

        /**
         * Render all lines which are currently in the local line vertex buffer. Make sure to call this at the end of your custom render frame function.
         */
        void RenderLines();

        //---------------------------------------------------------------------------------------------

        /**
         * The line structure to be used for rendering 2D lines.
         */
        struct MCOMMON_API Line2D
        {
            MCORE_MEMORYOBJECTCATEGORY(RenderUtil::Line2D, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);
            float               m_x1;    /**< The x position of the first vertex. */
            float               m_y1;    /**< The y position of the first vertex. */
            float               m_x2;    /**< The x position of the second vertex. */
            float               m_y2;    /**< The y position of the second vertex. */
            MCore::RGBAColor    m_color; /**< The line color. */
        };

        /**
         * Render a single line.
         * @param x1 The x coordinate in screen space of the starting point.
         * @param y1 The y coordinate in screen space of the starting point.
         * @param x2 The x coordinate in screen space of the ending point.
         * @param y2 The y coordinate in screen space of the ending point.
         * @param color The color of the line.
         */
        MCORE_INLINE void Render2DLine(float x1, float y1, float x2, float y2, const MCore::RGBAColor& color)
        {
            m_m2DLines[m_num2DLines].m_x1 = x1;
            m_m2DLines[m_num2DLines].m_y1 = y1;
            m_m2DLines[m_num2DLines].m_x2 = x2;
            m_m2DLines[m_num2DLines].m_y2 = y2;
            m_m2DLines[m_num2DLines].m_color = color;
            m_num2DLines++;
            if (m_num2DLines >= s_numMax2DLines)
            {
                Render2DLines();
            }
        }

        /**
         * Render the 2D given lines.
         * @param lines A pointer to the line array to be rendered.
         * @param numLines The number of lines in the array.
         */
        virtual void Render2DLines(Line2D* lines, uint32 numLines)                                                                          { MCORE_UNUSED(lines); MCORE_UNUSED(numLines); }

        /**
         * Render all lines which are currently in the local line buffer. Make sure to call this at the end of your custom render frame function.
         */
        void Render2DLines();

        //---------------------------------------------------------------------------------------------

        /**
         * Small helper mesh class used to render spheres, cylinders and other helper objects.
         */
        struct MCOMMON_API UtilMesh
        {
            MCORE_MEMORYOBJECTCATEGORY(RenderUtil::UtilMesh, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);
            /**
             * Default constructor.
             */
            UtilMesh();

            /**
             * Destructor.
             */
            ~UtilMesh();

            /**
             * Recalculate the normals of the mesh. Note that the normals must have been allocated in order to calculate them.
             * @param counterClockWise Vertex order used to control if the normals are pointing towards the inside or the outside of the mesh.
             */
            void CalculateNormals(bool counterClockWise = true);

            /**
             * Allocate memory for the vertices, indices and normals.
             * @param numVertices The number of vertices.
             * @param numIndices The number of indices. Note that this has to be a multiplier of three.
             * @param hasNormals Set to true in case you want to allocate memory for normals, false if your mesh won't have any normals.
             */
            void Allocate(uint32 numVertices, uint32 numIndices, bool hasNormals);

            AZStd::vector<AZ::Vector3>        m_positions;     /**< The vertex buffer. */
            AZStd::vector<uint32>             m_indices;       /**< The index buffer. */
            AZStd::vector<AZ::Vector3>        m_normals;       /**< The normal buffer. */
        };

        /**
         * The vertex structure to be used for rendering util meshes.
         */
        struct UtilMeshVertex
        {
            MCORE_MEMORYOBJECTCATEGORY(RenderUtil::UtilMeshVertex, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);
            AZ::Vector3 m_position;   /**< The position of the vertex. */
            AZ::Vector3 m_normal;     /**< The vertex normal. */

            UtilMeshVertex(const AZ::Vector3& pos, const AZ::Vector3& normal)
                : m_position(pos)
                , m_normal(normal) {}
        };

        /**
         * Render the given util mesh.
         * This function can be implemented by the user but is no requirement. In case you want to render the advanced skeleton using spheres and cylinders you need to
         * overload this function. Don't forget to overload the IsMeshRenderingSupported() function as well returning true, else the fallback line rendering functions will be used.
         * @param mesh A pointer to the util mesh to be rendered.
         * @param color The desired color.
         * @param worldTM The world space transformation matrix.
         */
        virtual void RenderUtilMesh(UtilMesh* mesh, const MCore::RGBAColor& color, const AZ::Transform& worldTM)           { MCORE_UNUSED(mesh); MCORE_UNUSED(color); MCORE_UNUSED(worldTM); }

        /**
         * Get if mesh rendering is supported in the given render util implementation.
         * Overload this function and return true in case you implemented the RenderUtilMesh() function.
         * @return True in case util mesh rendering is supported, false if not.
         */
        virtual bool GetIsMeshRenderingSupported() const                                                                    { return false; }

        /**
         * Rendering the normals or the tangents of a mesh need the world space transformed positions of the mesh.
         * To avoid recalculating them several times we do this at a central place. This function needs to be called before switching to a new mesh inside
         * the render loop as well as before an animation update, so before calling any of the render normals, face normals, tangents and bitangents functions.
         */
        MCORE_INLINE void ResetCurrentMesh()                                                                                { m_currentMesh = NULL; }

        //---------------------------------------------------------------------------------------------

        /**
         * The vertex structure to be used for rendering util meshes.
         */
        struct TriangleVertex
        {
            MCORE_MEMORYOBJECTCATEGORY(RenderUtil::TriangleVertex, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);
            AZ::Vector3  m_position;      /**< The position of the vertex. */
            AZ::Vector3  m_normal;        /**< The vertex normal. */
            uint32       m_color;

            MCORE_INLINE TriangleVertex(const AZ::Vector3& pos, const AZ::Vector3& normal, uint32 color)
                : m_position(pos)
                , m_normal(normal)
                , m_color(color) {}
        };

        MCORE_INLINE void AddTriangle(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& normalA, const AZ::Vector3& normalB, const AZ::Vector3& normalC, uint32 color)
        {
            m_triangleVertices.emplace_back(TriangleVertex(posA, normalA, color));
            m_triangleVertices.emplace_back(TriangleVertex(posB, normalB, color));
            m_triangleVertices.emplace_back(TriangleVertex(posC, normalC, color));

            if (m_triangleVertices.size() + 2 >= s_numMaxTriangleVertices)
            {
                RenderTriangles();
            }
        }

        virtual void RenderTriangles(const AZStd::vector<TriangleVertex>& triangleVertices) { MCORE_UNUSED(triangleVertices); }
        void RenderTriangles();

        //---------------------------------------------------------------------------------------------

        /**
         * Calculate the grid area visible by the given camera.
         * @param camera The camera to calculate the visible grid area for.
         * @param screenWidth The screen width in pixels.
         * @param screenHeight The screen height in pixels.
         * @param unitSize The grid unit size.
         * @param outGridStart The starting/minimum point of visible grid area/rect.
         * @param outGridEnd The ending/maximum point of visible grid area/rect.
         */
        void CalcVisibleGridArea(Camera* camera, uint32 screenWidth, uint32 screenHeight, float unitSize, AZ::Vector2* outGridStart, AZ::Vector2* outGridEnd);

        /**
         * Calculate the aabb which includes all actor instances.
         * @return The aabb which includes all actor instances.
         */
        AZ::Aabb CalcSceneAabb();

        struct TrajectoryPathParticle
        {
            EMotionFX::Transform m_worldTm;
        };

        struct TrajectoryTracePath
        {
            AZStd::vector<TrajectoryPathParticle> m_traceParticles;
            EMotionFX::ActorInstance*   m_actorInstance;
            float                       m_timePassed;

            TrajectoryTracePath()
            {
                m_traceParticles.reserve(250);
                m_timePassed     = 0.0f;
                m_actorInstance  = NULL;
            }
        };

        /**
         * Render an arrow for the trajectory of the actor instance.
         * In case the actor doesn't have a motion extraction node set, nothing is rendered.
         * @param[in] innerColor The color of the solid inside of the arrow.
         * @param[in] borderColor The color of the arrow border.
         * @param[in] scale The size or the arrow.
         */
        void RenderTrajectory(EMotionFX::ActorInstance* actorInstance, const MCore::RGBAColor& innerColor, const MCore::RGBAColor& borderColor, float scale);
        void RenderTrajectory(const AZ::Transform& worldTM, const MCore::RGBAColor& innerColor, const MCore::RGBAColor& borderColor, float scale);

        void RenderTrajectoryPath(TrajectoryTracePath* trajectoryPath, const MCore::RGBAColor& innerColor, float scale);
        static void ResetTrajectoryPath(TrajectoryTracePath* trajectoryPath);

        /**
         * Render text to screen. This will only work in case the Render2DLines() function has been implemented.
         */
        MCORE_INLINE void RenderText(float x, float y, const char* text, const MCore::RGBAColor& color = MCore::RGBAColor(1.0f, 1.0f, 1.0f), float fontSize = 11.0f, bool centered = false)   { m_font->Render(x * m_devicePixelRatio, (y * m_devicePixelRatio) + fontSize - 1, fontSize, centered, text, color); }

        /**
         * Render text to screen. This will only work in case the Render2DLines() function has been implemented.
         * @param[in] text The text to render.
         * @param[in] textSize The text size in pixels.
         * @param[in] worldSpacePos The world space position of the node.
         * @param[in] camera A pointer to the current camera used to render the scene.
         * @param[in] screenWidth The screen width in pixels.
         * @param[in] screenHeight The screen height in pixels.
         * @param[in] color The text color.
         */
        void RenderText(const char* text, uint32 textSize, const AZ::Vector3& worldSpacePos, MCommon::Camera* camera, uint32 screenWidth, uint32 screenHeight, const MCore::RGBAColor& color);

        void SetDevicePixelRatio(float devicePixelRatio)                                                { m_devicePixelRatio = devicePixelRatio; }

        virtual void EnableCulling(bool cullingEnabled) = 0;
        virtual bool GetCullingEnabled() = 0;

        virtual void EnableLighting(bool lightingEnabled) = 0;
        virtual bool GetLightingEnabled() = 0;

        virtual void SetDepthMaskWrite(bool writeEnabled) = 0;

        void RenderWireframeBox(const AZ::Vector3& dimensions, const AZ::Transform& worldTM, const MCore::RGBAColor& color, bool directlyRender = false);
        void RenderWireframeSphere(float radius, const AZ::Transform& worldTM, const MCore::RGBAColor& color, bool directlyRender = false);
        void RenderWireframeCapsule(float radius, float height, const AZ::Transform& worldTM, const MCore::RGBAColor& color, bool directlyRender = false);

    protected:
        /**
         * Get the "size" of the joint based on the bone length. This will be the size of the joint spheres and is also
         * used to calculate the correct length of the bone cylinders and the gaps.
         * @param actorInstance A pointer to the actor instance in which the bone is located.
         * @param node The bone used for the calculation.
         * @return The "size" of the joint. Zero will be returned in case the bone has no parent.
         */
        static float GetBoneScale(EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node);

        /**
         * Creates a util mesh we can use to render cylinders.
         * @param baseRadius The radius of the bottom of the cylinder.
         * @param topRadius The radius of the top of the cylinder.
         * @param length The height of the cylinder.
         * @param numSegments The number of sides/segments of the cylinder.
         * @return A pointer to the newly created util cylinder mesh.
         */
        static UtilMesh* CreateCylinder(float baseRadius, float topRadius, float length, uint32 numSegments = 8);

        /**
         * Change the shape of a given cylinder util mesh. This method can be used to adjust an already allocated cylinder util mesh.
         * For example this can be usedful if you need to change the base radius or the length of a cylinder while you don't want to create
         * a completely new cylinder util mesh.
         * @param mesh A pointer to the cylinder util mesh. Note that this mesh has to be created using CreateCylinder().
         * @param baseRadius The radius of the bottom of the cylinder.
         * @param topRadius The radius of the top of the cylinder.
         * @param length The height of the cylinder.
         * @param calculateNomals Set to true in case you want to recalculate the normals after adjusting the cylinder shape, false if you want to skip this calculation.
         */
        static void FillCylinder(UtilMesh* mesh, float baseRadius, float topRadius, float length, bool calculateNormals = false);

        /**
         * Create an util mesh we can use to render arrow heads.
         * @param height The height of the arrow head from the base to the head.
         * @param radius The radius of the base of the arrow head.
         * @return A pointer to the newly created util arrow head mesh.
         */
        static UtilMesh* CreateArrowHead(float height, float radius);

        /**
         * Change the shape of a given arrow head util mesh. This method can be used to adjust an already allocated arrow head util mesh.
         * For example this can be useful if you need to change the radius or the height of an arrow head.
         * @param mesh A pointer to the arrow head util mesh. Note that this mesh has to be created using CreateArrowHead().
         * @param height The height of the arrow head from the base to the head.
         * @param radius The radius of the base of the arrow head.
         * @param calculateNomals Set to true in case you want to recalculate the normals after adjusting the cylinder shape, false if you want to skip this calculation.
         */
        static void FillArrowHead(UtilMesh* mesh, float height, float radius, bool calculateNormals = false);

        /**
         * Create an util mesh we can use to render spheres.
         * @param radius The radius of the sphere.
         * @return A pointer to the newly created util sphere mesh.
         */
        static UtilMesh* CreateSphere(float radius, uint32 numSegments = 5);

        /**
         * Create an util mesh we can use to render cubes.
         * @param size The edge size of the cube.
         * @return A pointer to the newly created util cube mesh.
         */
        static UtilMesh* CreateCube(float size);

        /**
         * Rendering the normals, the tangents or the wireframe of a mesh need the world space transformed positions of the mesh.
         * To avoid recalculating them several times we do this at a central place. This function will pre-calculate the world space
         * positions of the mesh which will be then used to render the normals, tangents and the wireframe.
         * @param mesh A pointer to the mesh to pre-calculate the world space positions for.
         * @param worldTM The world space transformation matrix of the node to which the given mesh belongs to.
         */
        void PrepareForMesh(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM);

        class MCOMMON_API FontChar
        {
            MCORE_MEMORYOBJECTCATEGORY(RenderUtil::FontChar, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);

        public:
            FontChar();
            const unsigned char* Init(const unsigned char* data, const float* vertices);
            void Render(const float* vertices, RenderUtil* renderUtil, float textScale, float& x, float& y, float posX, float posY, const MCore::RGBAColor& color);

            MCORE_INLINE float GetWidth() const
            {
                float ret = 0.1f;
                if (m_indexCount > 0)
                {
                    ret = (m_x2 - m_x1) + 0.05f;
                }
                return ret;
            }

            float                   m_x1;
            float                   m_x2;
            float                   m_y1;
            float                   m_y2;
            unsigned short          m_indexCount;
            const unsigned short*   m_indices;
        };

        class MCOMMON_API VectorFont
        {
            MCORE_MEMORYOBJECTCATEGORY(RenderUtil::VectorFont, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);

        public:
            VectorFont(RenderUtil* renderUtil);
            ~VectorFont();

            void Init(const unsigned char* fontData);
            void Release();

            void Render(float posX, float posY, float textScale, bool centered, const char* text, const MCore::RGBAColor& color);
            float CalculateTextWidth(const char* text);

        private:
            uint32      m_version;
            uint32      m_vcount;
            uint32      m_count;
            float*      m_vertices;
            uint32      m_icount;
            FontChar    m_characters[256];
            RenderUtil* m_renderUtil;
        };

        EMotionFX::Mesh*                m_currentMesh;           /**< A pointer to the mesh whose world space positions are in the pre-calculated positions buffer. NULL in case we haven't pre-calculated any positions yet. */
        AZStd::vector<AZ::Vector3>      m_worldSpacePositions;   /**< The buffer used to store world space positions for rendering normals, tangents and the wireframe. */

        LineVertex*                     m_vertexBuffer;          /**< Array of line vertices. */
        uint32                          m_numVertices;           /**< The current number of vertices in the array. */
        static uint32                   s_numMaxLineVertices;    /**< The maximum capacity of the line vertex buffer. */

        Line2D*                         m_m2DLines;               /**< Array of 2D lines. */
        uint32                          m_num2DLines;            /**< The current number of 2D lines in the array. */
        static uint32                   s_numMax2DLines;         /**< The maximum capacity of the 2D line buffer. */
        static float                    s_wireframeSphereSegmentCount;

        float                           m_devicePixelRatio;
        VectorFont*                     m_font;                  /**< The vector font used to render text. */

        UtilMesh*                       m_unitSphereMesh;        /**< The preallocated and preconstructed sphere mesh used for rendering. */
        UtilMesh*                       m_cylinderMesh;          /**< The preallocated and preconstructed cylinder mesh used for rendering. */
        UtilMesh*                       m_unitCubeMesh;          /**< The preallocated and preconstructed cube mesh used for rendering. */
        UtilMesh*                       m_arrowHeadMesh;         /**< The preallocated and preconstructed arrow head mesh used for rendering. */
        static uint32                   s_numMaxMeshVertices;    /**< The maximum capacity of the util mesh vertex buffer. */
        static uint32                   s_numMaxMeshIndices;     /**< The maximum capacity of the util mesh index buffer */

        // helper variables for rendering triangles
        AZStd::vector<TriangleVertex>    m_triangleVertices;
        static uint32                   s_numMaxTriangleVertices;    /**< The maximum capacity of the triangle vertex buffer */
    };
} // namespace MCommon
