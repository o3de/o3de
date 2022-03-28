/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>

namespace AZ
{
    class Vector3;
}

namespace AzPhysics
{
    struct SimulatedBody;
}

namespace Physics
{
    class Shape;
    class Material;
    class MaterialConfiguration;
    class ColliderConfiguration;
    class ShapeConfiguration;

    /// Represents a debug vertex (position & color).
    struct DebugDrawVertex
    {
        AZ::Vector3     m_position;
        AZ::Color       m_color;

        DebugDrawVertex(const AZ::Vector3& v, const AZ::Color& c)
            : m_position(v)
            , m_color(c)
        {}

        static AZ::Color GetGhostColor()        { return AZ::Color(1.0f, 0.7f, 0.0f, 1.0f); }
        static AZ::Color GetRigidBodyColor()    { return AZ::Color(0.0f, 1.0f, 0.0f, 1.0f); }
        static AZ::Color GetSleepingBodyColor() { return AZ::Color(0.5f, 0.5f, 0.5f, 1.0f); }
        static AZ::Color GetCharacterColor()    { return AZ::Color(0.0f, 1.0f, 1.0f, 1.0f); }
        static AZ::Color GetRayColor()          { return AZ::Color(0.8f, 0.4f, 0.2f, 1.0f); }
        static AZ::Color GetRed()               { return AZ::Color(1.0f, 0.0f, 0.0f, 1.0f); }
        static AZ::Color GetGreen()             { return AZ::Color(0.0f, 1.0f, 0.0f, 1.0f); }
        static AZ::Color GetBlue()              { return AZ::Color(0.0f, 0.0f, 1.0f, 1.0f); }
        static AZ::Color GetWhite()             { return AZ::Color(1.0f, 1.0f, 1.0f, 1.0f); }
    };

    /// Settings structure provided to DebugDrawPhysics to drive debug drawing behavior.
    struct DebugDrawSettings
    {
        using DebugDrawLineCallback = AZStd::function<void(const DebugDrawVertex& from, const DebugDrawVertex& to, const AZStd::shared_ptr<AzPhysics::SimulatedBody>& body, float thickness, void* udata)>;
        using DebugDrawTriangleCallback = AZStd::function<void(const DebugDrawVertex& a, const DebugDrawVertex& b, const DebugDrawVertex& c, const AZStd::shared_ptr<AzPhysics::SimulatedBody>& body, void* udata)>;
        using DebugDrawTriangleBatchCallback = AZStd::function<void(const DebugDrawVertex* verts, AZ::u32 numVerts, const AZ::u32* indices, AZ::u32 numIndices, const AZStd::shared_ptr<AzPhysics::SimulatedBody>& body, void* udata)>;

        DebugDrawLineCallback           m_drawLineCB;                               ///< Required user callback for line drawing.
        DebugDrawTriangleBatchCallback  m_drawTriBatchCB;                           ///< User callback for triangle batch drawing. Required if \ref m_isWireframe is false.
        bool                            m_isWireframe = false;                      ///< Specifies whether or not physics shapes should be draw as wireframe (lines only) or solid triangles.
        AZ::u32                         m_objectLayers = static_cast<AZ::u32>(~0);  ///< Mask specifying which \ref AzFramework::Physics::StandardObjectLayers should be drawn.
        AZ::Vector3                     m_cameraPos = AZ::Vector3::CreateZero();    ///< Camera position, for limiting objects based on \ref m_drawDistance.
        float                           m_drawDistance = 500.f;                     ///< Distance from \ref m_cameraPos within which objects will be drawn.
        bool                            m_drawBodyTransforms = false;               ///< If enabled, draws transform axes for each body.
        void*                           m_udata = nullptr;                          ///< Platform specific and/or gem specific optional user data pointer.

        void DrawLine(const DebugDrawVertex& from, const DebugDrawVertex& to, const AZStd::shared_ptr<AzPhysics::SimulatedBody>& body, float thickness = 1.0f) { m_drawLineCB(from, to, body, thickness, m_udata); }
        void DrawTriangleBatch(const DebugDrawVertex* verts, AZ::u32 numVerts, const AZ::u32* indices, AZ::u32 numIndices, const AZStd::shared_ptr<AzPhysics::SimulatedBody>& body) { m_drawTriBatchCB(verts, numVerts, indices, numIndices, body, m_udata); }
    };

    /// An interface to get the default physics world for systems that do not support multiple worlds.
    class DefaultWorldRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::mutex;

        //! Returns a handle to the Default Scene managed by a relevant system.
        virtual AzPhysics::SceneHandle GetDefaultSceneHandle() const = 0;
    };

    typedef AZ::EBus<DefaultWorldRequests> DefaultWorldBus;

    /// An interface to get the editor physics world for doing edit time physics queries
    class EditorWorldRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::mutex;

        //! Returns a handle to the Editor Scene managed by editor system component.
        virtual AzPhysics::SceneHandle GetEditorSceneHandle() const = 0;
    };
    using EditorWorldBus = AZ::EBus<EditorWorldRequests>;

    class SystemRequestsTraits 
        : public AZ::EBusTraits 
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        // EBusTraits
        // singleton pattern
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    /// Physics system global requests.
    class System
    {
    public:
        // Required for AZ::Interface
        AZ_TYPE_INFO(System, "{35965894-BFBC-437C-A4FB-E22F3DB09ACF}")

        System() = default;
        virtual ~System() = default;

        // AZ::Interface requires these to be deleted.
        System(System&&) = delete;
        System& operator=(System&&) = delete;

        //////////////////////////////////////////////////////////////////////////
        //// General Physics

        virtual AZStd::shared_ptr<Shape> CreateShape(const ColliderConfiguration& colliderConfiguration, const ShapeConfiguration& configuration) = 0;

        virtual AZStd::shared_ptr<Material> CreateMaterial(const Physics::MaterialConfiguration& materialConfiguration) = 0;

        /// Releases the height field object created by the physics backend.
        /// @param nativeHeightfieldObject Pointer to the height field object.
        virtual void ReleaseNativeHeightfieldObject(void* nativeHeightfieldObject) = 0;

        /// Releases the mesh object created by the physics backend.
        /// @param nativeMeshObject Pointer to the mesh object.
        virtual void ReleaseNativeMeshObject(void* nativeMeshObject) = 0;

        //////////////////////////////////////////////////////////////////////////
        //// Cooking

        /// Cooks a convex mesh and writes it to a file.
        /// @param filePath Path to the output file.
        /// @param vertices Pointer to beginning of vertex data.
        /// @param vertexCount Number of vertices in the mesh.
        /// @return Succeeded cooking.
        virtual bool CookConvexMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount) = 0;

        /// Cooks a convex mesh to a memory buffer.
        /// @param vertices Pointer to beginning of vertex data.
        /// @param vertexCount Number of vertices in the mesh.
        /// @param result The resulting memory buffer.
        /// @return Succeeded cooking.
        virtual bool CookConvexMeshToMemory(const AZ::Vector3* vertices, AZ::u32 vertexCount, AZStd::vector<AZ::u8>& result) = 0;

        /// Cooks a triangular mesh and writes it to a file.
        /// @param filePath Path to the output file.
        /// @param vertices Pointer to beginning of vertex data.
        /// @param vertexCount Number of vertices in the mesh.
        /// @param indices Pointer to beginning of index data.
        /// @param indexCount Number of indices in the mesh.
        /// @return Succeeded cooking.
        virtual bool CookTriangleMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount,
            const AZ::u32* indices, AZ::u32 indexCount) = 0;

        /// Cook a triangular mesh to a memory buffer.
        /// @param vertices Pointer to beginning of vertex data.
        /// @param vertexCount Number of vertices in the mesh.
        /// @param indices Pointer to beginning of index data.
        /// @param indexCount Number of indices in the mesh.
        /// @param result The resulting memory buffer.
        /// @return Succeeded cooking.
        virtual bool CookTriangleMeshToMemory(const AZ::Vector3* vertices, AZ::u32 vertexCount,
            const AZ::u32* indices, AZ::u32 indexCount, AZStd::vector<AZ::u8>& result) = 0;
    };

    using SystemRequests = System;
    using SystemRequestBus = AZ::EBus<SystemRequests, SystemRequestsTraits>;

    /// Physics system global debug requests.
    class SystemDebugRequests
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits
        // singleton pattern
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        /// Draw physics system state.
        /// \param settings see \ref DebugDrawSettings.
        virtual void DebugDrawPhysics(const DebugDrawSettings& settings) { (void)settings; }

        /// Exports an entity's physics body(ies) to the specified filename, if supported by the physics backend.
        virtual void ExportEntityPhysics(const AZStd::vector<AZ::EntityId>& ids, const AZStd::string& filename) { (void)ids; (void)filename; }
    };

    typedef AZ::EBus<SystemDebugRequests> SystemDebugRequestBus;

} // namespace Physics
