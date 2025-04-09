/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/DrawListContext.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/VisibleObjectContext.h>

#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Name/Name.h>

class MaskedOcclusionCulling;

namespace AZ
{
    // forward declares
    class Job;
    class TaskGraphEvent;
    namespace  RHI
    {
        class FrameScheduler;

    } // namespace RHI

    namespace RPI
    {
        //! Represents a view into a scene, and is the primary interface for adding DrawPackets to the draw queues.
        //! It encapsulates the world<->view<->clip transforms and the per-view shader constants.
        //! Use View::CreateView() to make new vew Objects to ensure that you have a shared ViewPtr to pass around the code.
        //! There are different ways to setup the worldToView/viewToWorld matrices. Only one set function needs to be called:
        //!  SetWorldToViewMatrix()
        //!  SetCameraTransform()
        //! To have a fully formed set of view transforms you also need to call SetViewToClipMatrix() to set up the projection.
        class ATOM_RPI_PUBLIC_API View final
        {
        public:
            AZ_TYPE_INFO(View, "{C3FFC8DE-83C4-4E29-8216-D55BE0ACE3E4}");
            AZ_CLASS_ALLOCATOR(View, AZ::SystemAllocator);

            enum UsageFlags : uint32_t         //bitwise operators are defined for this (see below)
            {
                UsageNone = 0u,
                UsageCamera = (1u << 0),
                UsageShadow = (1u << 1),
                UsageReflectiveCubeMap = (1u << 2),
                UsageXR = (1u << 3)
            };
            //! Only use this function to create a new view object. And force using smart pointer to manage view's life time
            static ViewPtr CreateView(const AZ::Name& name, UsageFlags usage);

            ~View();

            void SetDrawListMask(const RHI::DrawListMask& drawListMask);
            RHI::DrawListMask GetDrawListMask() const { return m_drawListMask; }
            void Reset();

            //! Prints the draw list mask for this view. Useful for printf debugging.
            void PrintDrawListMask();

            RHI::ShaderResourceGroup* GetRHIShaderResourceGroup() const;

            Data::Instance<RPI::ShaderResourceGroup> GetShaderResourceGroup();
            
            //! Add a draw packet to this view. DrawPackets need to be added every frame. This function is thread safe.
            //! The depth value here is the depth of the object from the perspective of the view.
            void AddDrawPacket(const RHI::DrawPacket* drawPacket, float depth = 0.0f);

            //! Similar to previous AddDrawPacket() but calculates depth from packet position
            void AddDrawPacket(const RHI::DrawPacket* drawPacket, const Vector3& worldPosition);
            
            //! Similar to AddDrawPacket, but the view will not submit any draw items for rendering. It will just
            //! maintain a list of visible objects for the current frame, and the caller must get that list, reinterpret the
            //! userData, and submit the draw calls.
            void AddVisibleObject(const void* userData, float depth = 0.0f);

            //! Similar to previous AddVisibleObject() but calculates depth from object position
            void AddVisibleObject(const void* userData, const Vector3& worldPosition);

            //! Add a draw item to this view with its associated draw list tag
            void AddDrawItem(RHI::DrawListTag drawListTag, const RHI::DrawItemProperties& drawItemProperties);

            //! Applies some flags to the view that are reset each frame. The provided flags are combined with m_andFlags
            //! using &, and are combined with m_orFlags using |.
            void ApplyFlags(uint32_t flags);

            //! Clears and resets the flag positions marked with flag. This means the 'and' flag is set to 1 and the 'or' flag is set to 0.
            void ClearFlags(uint32_t flags);

            //! Clears and resets all the flags. This effectively sets the and flags back to 0xFFFFFFFF and the or flags to 0x00000000;
            void ClearAllFlags();

            //! Returns the boolean & combination of all flags provided with ApplyFlags() since the last frame.
            uint32_t GetAndFlags() const;

            //! Returns the boolean | combination of all flags provided with ApplyFlags() since the last frame.
            uint32_t GetOrFlags() const;

            //! Sets the worldToView matrix and recalculates the other matrices.
            void SetWorldToViewMatrix(const AZ::Matrix4x4& worldToView);

            //! Set the viewtoWorld matrix through camera's world transformation (z-up) and recalculates the other matrices
            void SetCameraTransform(const AZ::Matrix3x4& cameraTransform);

            //! Sets the viewToClip matrix and recalculates the other matrices
            void SetViewToClipMatrix(const AZ::Matrix4x4& viewToClip);

            //! Sets the viewToClip exclusion matrix. This is used by culling to exclude items completely contained inside
            //! the exclusion frustum. Pass in nullptr to unset.
            void SetViewToClipExcludeMatrix(const AZ::Matrix4x4* viewToClipExclude);
            
            //! Sets the viewToClip matrix and recalculates the other matrices for stereoscopic projection
            void SetStereoscopicViewToClipMatrix(const AZ::Matrix4x4& viewToClip, bool reverseDepth = true);

            //! Sets a pixel offset on the view, usually used for jittering the camera for anti-aliasing techniques.
            void SetClipSpaceOffset(float xOffset, float yOffset);

            const AZ::Matrix4x4& GetWorldToViewMatrix() const;
            //! Use GetViewToWorldMatrix().GetTranslation() to get the camera's position.
            const AZ::Matrix4x4& GetViewToWorldMatrix() const;
            const AZ::Matrix4x4& GetViewToClipMatrix() const;
            const AZ::Matrix4x4& GetWorldToClipMatrix() const;
            const AZ::Matrix4x4* GetWorldToClipExcludeMatrix() const;
            const AZ::Matrix4x4& GetClipToWorldMatrix() const;
            const AZ::Matrix4x4& GetClipToViewMatrix() const;

            //! Functions for getting the matrices that are used in the view srg
            //! These are different from the matrices returned above as they take clip space offset into account
            //! They are updated in the UpdateSrg function
            //! Calling these functions before UpdateSrg will return the last frames values
            const Matrix4x4& GetWorldToClipPrevMatrixWithOffset() const;
            const Matrix4x4& GetWorldToClipMatrixWithOffset() const;
            const Matrix4x4& GetViewToClipMatrixWithOffset() const;
            const Matrix4x4& GetClipToWorldMatrixWithOffset() const;
            const Matrix4x4& GetClipToViewMatrixWithOffset() const;

            AZ::Matrix3x4 GetWorldToViewMatrixAsMatrix3x4() const;
            AZ::Matrix3x4 GetViewToWorldMatrixAsMatrix3x4() const;

            //! Get the camera's world transform, converted from the viewToWorld matrix's native y-up to z-up
            AZ::Transform GetCameraTransform() const;

            //! Finalize visible object lists in this view. This function should only be called when all
            //! visible objects for current frame are added, but before FinalizeDrawLists is called. 
            void FinalizeVisibleObjectList();

            //! Finalize draw lists in this view. This function should only be called when all
            //! draw packets for current frame are added. 
            void FinalizeDrawListsJob(AZ::Job* parentJob);
            void FinalizeDrawListsTG(AZ::TaskGraphEvent& finalizeDrawListsTGEvent);

            bool HasDrawListTag(RHI::DrawListTag drawListTag);

            RHI::DrawListView GetDrawList(RHI::DrawListTag drawListTag);
            VisibleObjectListView GetVisibleObjectList();

            //! Helper function to generate a sort key from a given position in world
            RHI::DrawItemSortKey GetSortKeyForPosition(const Vector3& positionInWorld) const;

            //! Returns the area of the given sphere projected into clip space in terms of percentage coverage of the viewport.
            //! Value returned is 1.0f when an area equal to the viewport height squared is covered. Useful for accurate LOD decisions.
            float CalculateSphereAreaInClipSpace(const AZ::Vector3& sphereWorldPosition, float sphereRadius) const;

            const AZ::Name& GetName() const;
            UsageFlags GetUsageFlags() const;

            void SetPassesByDrawList(PassesByDrawList* passes);

            //! Update View's SRG values and compile. This should only be called once per frame before execute command lists.
            void UpdateSrg();

            //! Notifies consumers when the world to view matrix has changed.
            void ConnectWorldToViewMatrixChangedHandler(MatrixChangedEvent::Handler& handler);
            //! Notifies consumers when the world to clip matrix has changed.
            void ConnectWorldToClipMatrixChangedHandler(MatrixChangedEvent::Handler& handler);

            //! Prepare for view culling
            void BeginCulling();

            //! Returns the masked occlusion culling interface
            MaskedOcclusionCulling* GetMaskedOcclusionCulling();
            void SetMaskedOcclusionCullingDirty(bool dirty);
            bool GetMaskedOcclusionCullingDirty() const;

            //! This is called by RenderPipeline when this view is added to the pipeline.
            void OnAddToRenderPipeline();

            //! Accessors for shadow pass render pipeline id.
            void SetShadowPassRenderPipelineId(const RenderPipelineId renderPipelineId);
            RenderPipelineId GetShadowPassRenderPipelineId() const;
            
        private:
            View() = delete;
            View(const AZ::Name& name, UsageFlags usage);

            //! Sorts the finalized draw lists in this view
            void SortFinalizedDrawListsJob(AZ::Job* parentJob);
            void SortFinalizedDrawListsTG(AZ::TaskGraphEvent& finalizeDrawListsTGEvent);

            //! Sorts a drawList using the sort function from a pass with the corresponding drawListTag
            void SortDrawList(RHI::DrawList& drawList, RHI::DrawListTag tag);

            //! Attempt to create a shader resource group.
            void TryCreateShaderResourceGroup();

            //! Update ViewToWorld matrix as well as the view transform
            void UpdateViewToWorldMatrix(const AZ::Matrix4x4& viewToWorld);

            AZ::Name m_name;
            UsageFlags m_usageFlags;

            // Shader resource group used per view
            Data::Instance<RPI::ShaderResourceGroup> m_shaderResourceGroup;

            // Pointer to list of passes relevant to the draw lists (passes will be used for sorting the draw lists)
            PassesByDrawList* m_passesByDrawList = nullptr;

            // Indies of constants in default view srg
            RHI::ShaderInputNameIndex m_viewProjectionMatrixConstantIndex = "m_viewProjectionMatrix";
            RHI::ShaderInputNameIndex m_worldPositionConstantIndex = "m_worldPosition";
            RHI::ShaderInputNameIndex m_viewMatrixConstantIndex = "m_viewMatrix";
            RHI::ShaderInputNameIndex m_viewMatrixInverseConstantIndex = "m_viewMatrixInverse";
            RHI::ShaderInputNameIndex m_projectionMatrixConstantIndex = "m_projectionMatrix";
            RHI::ShaderInputNameIndex m_projectionMatrixInverseConstantIndex = "m_projectionMatrixInverse";
            RHI::ShaderInputNameIndex m_clipToWorldMatrixConstantIndex = "m_viewProjectionInverseMatrix";
            RHI::ShaderInputNameIndex m_worldToClipPrevMatrixConstantIndex = "m_viewProjectionPrevMatrix";
            RHI::ShaderInputNameIndex m_zConstantsConstantIndex = "m_linearizeDepthConstants";
            RHI::ShaderInputNameIndex m_unprojectionConstantsIndex = "m_unprojectionConstants";

            // The context containing draw lists associated with the view.
            RHI::DrawListContext m_drawListContext;
            RHI::DrawListMask m_drawListMask;

            RPI::VisibleObjectContext m_visibleObjectContext;

            Matrix4x4 m_worldToViewMatrix;
            Matrix4x4 m_viewToWorldMatrix;
            Matrix4x4 m_viewToClipMatrix;
            AZStd::optional<Matrix4x4> m_viewToClipExcludeMatrix;
            Matrix4x4 m_clipToViewMatrix;
            Matrix4x4 m_clipToWorldMatrix;
            AZStd::optional<Matrix4x4> m_worldToClipExcludeMatrix;

            Matrix4x4 m_worldToClipPrevMatrixWithOffset;
            Matrix4x4 m_worldToClipMatrixWithOffset;
            Matrix4x4 m_viewToClipMatrixWithOffset;
            Matrix4x4 m_clipToWorldMatrixWithOffset;
            Matrix4x4 m_clipToViewMatrixWithOffset;

            // Cached View transform from ViewToWorld matrix 
            AZ::Transform m_viewTransform;

            // View's position in world space
            Vector3 m_position;

            // Precached constants for linearZ process
            Vector4 m_linearizeDepthConstants;

            // Constants used to unproject depth values and reconstruct the view-space position (Z-forward & Y-up)
            Vector4 m_unprojectionConstants;

            // Cached matrix to transform from world space to clip space
            Matrix4x4 m_worldToClipMatrix;

            Matrix4x4 m_worldToViewPrevMatrix;
            Matrix4x4 m_viewToClipPrevMatrix;

            // Clip space offset for camera jitter with taa
            Vector2 m_clipSpaceOffset = Vector2(0.0f, 0.0f);

            MatrixChangedEvent m_onWorldToClipMatrixChange;
            MatrixChangedEvent m_onWorldToViewMatrixChange;

            // Masked Occlusion Culling interface
            MaskedOcclusionCulling* m_maskedOcclusionCulling = nullptr;
            AZStd::atomic_bool m_maskedOcclusionCullingDirty = true;

            AZStd::atomic_uint32_t m_andFlags{ 0xFFFFFFFF };
            AZStd::atomic_uint32_t m_orFlags { 0x00000000 };

            // Get the render pipeline id associated with this view if used as a shadow light view.
            RenderPipelineId m_shadowPassRenderpipelineId;
        };

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(View::UsageFlags);
    } // namespace RPI
} // namespace AZ
