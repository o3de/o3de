/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/View.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <Atom/RHI/DrawListTagRegistry.h>

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom_RPI_Traits_Platform.h>

#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
#include <MaskedOcclusionCulling/MaskedOcclusionCulling.h>
#endif

namespace AZ
{
    namespace RPI
    {
        // fixed-size software occlusion culling buffer
#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
        const uint32_t MaskedSoftwareOcclusionCullingWidth = 1920;
        const uint32_t MaskedSoftwareOcclusionCullingHeight = 1080;
#endif

        ViewPtr View::CreateView(const AZ::Name& name, UsageFlags usage)
        {
            View* view = aznew View(name, usage);
            return ViewPtr(view);
        }

        View::View(const AZ::Name& name, UsageFlags usage)
            : m_name(name)
            , m_usageFlags(usage)
        {
            AZ_Assert(!name.IsEmpty(), "invalid name");

            // Set default matrixes. 
            SetWorldToViewMatrix(AZ::Matrix4x4::CreateIdentity());
            AZ::Matrix4x4 viewToClipMatrix;
            AZ::MakePerspectiveFovMatrixRH(viewToClipMatrix, AZ::Constants::HalfPi, 1, 0.1f, 1000.f, true);
            SetViewToClipMatrix(viewToClipMatrix);

            Data::Asset<ShaderAsset> viewSrgShaderAsset = RPISystemInterface::Get()->GetCommonShaderAssetForSrgs();

            if (viewSrgShaderAsset.IsReady())
            {
                m_shaderResourceGroup = ShaderResourceGroup::Create(viewSrgShaderAsset, RPISystemInterface::Get()->GetViewSrgLayout()->GetName());
            }
#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
            m_maskedOcclusionCulling = MaskedOcclusionCulling::Create();
            m_maskedOcclusionCulling->SetResolution(MaskedSoftwareOcclusionCullingWidth, MaskedSoftwareOcclusionCullingHeight);
#endif
        }

        View::~View()
        {
#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
            if (m_maskedOcclusionCulling)
            {
                MaskedOcclusionCulling::Destroy(m_maskedOcclusionCulling);
                m_maskedOcclusionCulling = nullptr;
            }
#endif
        }

        void View::SetDrawListMask(const RHI::DrawListMask& drawListMask)
        {
            m_drawListMask = drawListMask;
            m_drawListContext.Shutdown();
            m_drawListContext.Init(m_drawListMask);
        }

        void View::Reset()
        {
            m_drawListMask.reset();
            m_drawListContext.Shutdown();
            m_passesByDrawList = nullptr;
        }

        RHI::ShaderResourceGroup* View::GetRHIShaderResourceGroup() const
        {
            return m_shaderResourceGroup->GetRHIShaderResourceGroup();
        }

        Data::Instance<RPI::ShaderResourceGroup> View::GetShaderResourceGroup()
        {
            return m_shaderResourceGroup;
        }

        void View::AddDrawPacket(const RHI::DrawPacket* drawPacket, float depth)
        {
            // This function is thread safe since DrawListContent has storage per thread for draw item data.
            m_drawListContext.AddDrawPacket(drawPacket, depth);
        }        

        void View::AddDrawPacket(const RHI::DrawPacket* drawPacket, Vector3 worldPosition)
        {
            Vector3 cameraToObject = worldPosition - m_position;
            float depth = cameraToObject.Dot(-m_viewToWorldMatrix.GetBasisZAsVector3());
            AddDrawPacket(drawPacket, depth);
        }

        void View::AddDrawItem(RHI::DrawListTag drawListTag, const RHI::DrawItemProperties& drawItemProperties)
        {
            m_drawListContext.AddDrawItem(drawListTag, drawItemProperties);
        }

        void View::SetWorldToViewMatrix(const AZ::Matrix4x4& worldToView)
        {
            m_viewToWorldMatrix = worldToView.GetInverseFast();
            m_position = m_viewToWorldMatrix.GetTranslation();

            m_worldToViewMatrix = worldToView;
            m_worldToClipMatrix = m_viewToClipMatrix * m_worldToViewMatrix;

            m_onWorldToViewMatrixChange.Signal(m_worldToViewMatrix);
            m_onWorldToClipMatrixChange.Signal(m_worldToClipMatrix);
        }

        AZ::Transform View::GetCameraTransform() const
        {
            static const Quaternion yUpToZUp = Quaternion::CreateRotationX(-AZ::Constants::HalfPi);
            return AZ::Transform::CreateFromQuaternionAndTranslation(
                Quaternion::CreateFromMatrix4x4(m_viewToWorldMatrix) * yUpToZUp,
                m_viewToWorldMatrix.GetTranslation()
            ).GetOrthogonalized();
        }

        void View::SetCameraTransform(const AZ::Matrix3x4& cameraTransform)
        {
            m_position = cameraTransform.GetTranslation();

            // Before inverting the matrix we must first adjust from Z-up to Y-up. The camera world matrix
            // is in a Z-up world and an identity matrix means that it faces along the positive-Y axis and Z is up.
            // An identity view matrix on the other hand looks along the negative Z-axis.
            // So we adjust for this by rotating the camera world matrix by 90 degrees around the X axis.
            static AZ::Matrix3x4 zUpToYUp = AZ::Matrix3x4::CreateRotationX(AZ::Constants::HalfPi);
            AZ::Matrix3x4 yUpWorld = cameraTransform * zUpToYUp;

            float viewToWorldMatrixRaw[16] = {
                        1,0,0,0,
                        0,1,0,0,
                        0,0,1,0,
                        0,0,0,1 };
            yUpWorld.StoreToRowMajorFloat12(viewToWorldMatrixRaw);
            const AZ::Matrix4x4 prevViewToWorldMatrix = m_viewToWorldMatrix;
            m_viewToWorldMatrix = AZ::Matrix4x4::CreateFromRowMajorFloat16(viewToWorldMatrixRaw);

            m_worldToViewMatrix = m_viewToWorldMatrix.GetInverseFast();

            m_worldToClipMatrix = m_viewToClipMatrix * m_worldToViewMatrix;

            // Only signal an update when there is a change, otherwise this might block
            // user input from changing the value.
            if (!prevViewToWorldMatrix.IsClose(m_viewToWorldMatrix))
            {
                m_onWorldToViewMatrixChange.Signal(m_worldToViewMatrix);
            }
            m_onWorldToClipMatrixChange.Signal(m_worldToClipMatrix);
        }        

        void View::SetViewToClipMatrix(const AZ::Matrix4x4& viewToClip)
        {
            m_viewToClipMatrix = viewToClip;

            m_worldToClipMatrix = m_viewToClipMatrix * m_worldToViewMatrix;

            // Update z depth constant simultaneously
            // zNear -> n, zFar -> f
            // A = f / (n - f), B = nf / (n - f) 
            // the formula of A and B should be the same as projection matrix's definition 
            // currently defined in MakePerspectiveFovMatrixRH in MatrixUtil.cpp
            double A = m_viewToClipMatrix.GetElement(2, 2);
            double B = m_viewToClipMatrix.GetElement(2, 3);

            m_nearZ_farZ_farZTimesNearZ_farZMinusNearZ.SetX(float(B / A));
            m_nearZ_farZ_farZTimesNearZ_farZMinusNearZ.SetY(float(B / (A + 1.0)));
            m_nearZ_farZ_farZTimesNearZ_farZMinusNearZ.SetZ(float((B * B) / (A * (A + 1.0))));
            m_nearZ_farZ_farZTimesNearZ_farZMinusNearZ.SetW(float(-B / (A * (A + 1.0))));

            double tanHalfFovX = 1.0 / m_viewToClipMatrix.GetElement(0, 0);
            double tanHalfFovY = 1.0 / m_viewToClipMatrix.GetElement(1, 1);

            m_unprojectionConstants.SetX(float(2.0 * tanHalfFovX));
            m_unprojectionConstants.SetY(float(-2.0 * tanHalfFovY));
            m_unprojectionConstants.SetZ(float(-tanHalfFovX));
            m_unprojectionConstants.SetW(float(tanHalfFovY));

            m_onWorldToClipMatrixChange.Signal(m_worldToClipMatrix);
        }
        
        void View::SetClipSpaceOffset(float xOffset, float yOffset)
        {
            m_clipSpaceOffset.Set(xOffset, yOffset);
        }

        const AZ::Matrix4x4& View::GetWorldToViewMatrix() const
        {
            return m_worldToViewMatrix;
        }

        const AZ::Matrix4x4& View::GetViewToWorldMatrix() const
        {
            return m_viewToWorldMatrix;
        }

        const AZ::Matrix4x4& View::GetViewToClipMatrix() const
        {
            return m_viewToClipMatrix;
        }

        const AZ::Matrix4x4& View::GetWorldToClipMatrix() const
        {
            return m_worldToClipMatrix;
        }

        bool View::HasDrawListTag(RHI::DrawListTag drawListTag)
        {
            return drawListTag.IsValid() && m_drawListMask[drawListTag.GetIndex()];
        }

        RHI::DrawListView View::GetDrawList(RHI::DrawListTag drawListTag)
        {
            return m_drawListContext.GetList(drawListTag);
        }

        void View::FinalizeDrawLists()
        {
            AZ_PROFILE_SCOPE(RPI, "View: FinalizeDrawLists");
            m_drawListContext.FinalizeLists();
            SortFinalizedDrawLists();
        }

        void View::SortFinalizedDrawLists()
        {
            RHI::DrawListsByTag& drawListsByTag = m_drawListContext.GetMergedDrawListsByTag();

            for (size_t idx = 0; idx < drawListsByTag.size(); ++idx)
            {
                if (drawListsByTag[idx].size() > 1)
                {
                    SortDrawList(drawListsByTag[idx], RHI::DrawListTag(idx));
                }
            }
        }

        void View::SortDrawList(RHI::DrawList& drawList, RHI::DrawListTag tag)
        {
            const Pass* passWithDrawListTag = (*m_passesByDrawList)[tag];
            passWithDrawListTag->SortDrawList(drawList);
        }

        void View::ConnectWorldToViewMatrixChangedHandler(View::MatrixChangedEvent::Handler& handler)
        {
            handler.Connect(m_onWorldToViewMatrixChange);
        }

        void View::ConnectWorldToClipMatrixChangedHandler(View::MatrixChangedEvent::Handler& handler)
        {
            handler.Connect(m_onWorldToClipMatrixChange);
        }

        // [GFX TODO] This function needs unit tests and might need to be reworked 
        RHI::DrawItemSortKey View::GetSortKeyForPosition(const Vector3& positionInWorld) const
        {
            // We are using fixed-point depth representation for the u64 sort key

            // Compute position in clip space
            const Vector4 worldPosition4 = Vector4::CreateFromVector3(positionInWorld);
            const Vector4 clipSpacePosition = m_worldToClipMatrix * worldPosition4;

            // Get a depth value guaranteed to be in the range 0 to 1
            float normalizedDepth = clipSpacePosition.GetZ() / clipSpacePosition.GetW();
            normalizedDepth = (normalizedDepth + 1.0f) * 0.5f;
            normalizedDepth = AZStd::clamp<float>(normalizedDepth, 0.f, 1.f);

            // Convert the depth into a uint64
            RHI::DrawItemSortKey sortKey = static_cast<RHI::DrawItemSortKey>(normalizedDepth * azlossy_cast<double>(std::numeric_limits<RHI::DrawItemSortKey>::max()));

            return sortKey;
        }

        float View::CalculateSphereAreaInClipSpace(const AZ::Vector3& sphereWorldPosition, float sphereRadius) const
        {
            // Projection of a sphere to clip space 
            // Derived from https://www.iquilezles.org/www/articles/sphereproj/sphereproj.htm

            if (sphereRadius <= 0.0f)
            {
                return 0.0f;
            }

            const AZ::Matrix4x4& worldToViewMatrix = GetWorldToViewMatrix();
            const AZ::Matrix4x4& viewToClipMatrix = GetViewToClipMatrix();

            // transform to camera space (eye space)
            const Vector4 worldPosition4 = Vector4::CreateFromVector3(sphereWorldPosition);
            const Vector4 viewSpacePosition = worldToViewMatrix * worldPosition4;

            float zDist = -viewSpacePosition.GetZ();    // in our view space Z is negative in front of the camera

            if (zDist < 0.0f)
            {
                // sphere center is behind camera.
                if (zDist < -sphereRadius)
                {
                    return 0.0f;    // whole of sphere is behind camera so zero coverage
                }
                else
                {
                    return 1.0f;    // camera is inside sphere so treat as covering whole view
                }
            }
            else
            {
                if (zDist < sphereRadius)
                {
                    return 1.0f;   // camera is inside sphere so treat as covering whole view
                }
            }

            // Element 1,1 of the projection matrix is equal to :  1 / tan(fovY/2) AKA cot(fovY/2)
            // See https://stackoverflow.com/questions/46182845/field-of-view-aspect-ratio-view-matrix-from-projection-matrix-hmd-ost-calib
            float cotHalfFovY = viewToClipMatrix.GetElement(1, 1);

            float radiusSq = sphereRadius * sphereRadius;
            float depthSq = zDist * zDist;
            float distanceSq = viewSpacePosition.GetAsVector3().GetLengthSq();
            float cotHalfFovYSq = cotHalfFovY * cotHalfFovY;

            float radiusSqSubDepthSq = radiusSq - depthSq;

            const float epsilon = 0.00001f;
            if (fabsf(radiusSqSubDepthSq) < epsilon)
            {
                // treat as covering entire view since we don't want to divide by zero
                return 1.0f;
            }

            // This will return 1.0f when an area equal in size to the viewport height squared is covered.
            // So to get actual pixels covered do : coverage * viewport-resolution-y * viewport-resolution-y
            // The actual math computes the area of an ellipse as a percentage of the view area, see the paper above for the steps
            // to simplify the equations into this calculation.
            return  -0.25f * cotHalfFovYSq * AZ::Constants::Pi * radiusSq * sqrt(fabsf((distanceSq - radiusSq)/radiusSqSubDepthSq))/radiusSqSubDepthSq;
        }

        void View::UpdateSrg()
        {
            if (m_clipSpaceOffset.IsZero())
            {
                Matrix4x4 worldToClipPrevMatrix = m_viewToClipPrevMatrix * m_worldToViewPrevMatrix;
                m_shaderResourceGroup->SetConstant(m_worldToClipPrevMatrixConstantIndex, worldToClipPrevMatrix);
                m_shaderResourceGroup->SetConstant(m_viewProjectionMatrixConstantIndex, m_worldToClipMatrix);
                m_shaderResourceGroup->SetConstant(m_projectionMatrixConstantIndex, m_viewToClipMatrix);
                m_shaderResourceGroup->SetConstant(m_clipToWorldMatrixConstantIndex, m_clipToWorldMatrix);
                m_shaderResourceGroup->SetConstant(m_projectionMatrixInverseConstantIndex, m_viewToClipMatrix.GetInverseFull());
            }
            else
            {
                // Offset the current and previous frame clip matricies
                Matrix4x4 offsetViewToClipMatrix = m_viewToClipMatrix;
                offsetViewToClipMatrix.SetElement(0, 2, m_clipSpaceOffset.GetX());
                offsetViewToClipMatrix.SetElement(1, 2, m_clipSpaceOffset.GetY());

                Matrix4x4 offsetViewToClipPrevMatrix = m_viewToClipPrevMatrix;
                offsetViewToClipPrevMatrix.SetElement(0, 2, m_clipSpaceOffset.GetX());
                offsetViewToClipPrevMatrix.SetElement(1, 2, m_clipSpaceOffset.GetY());

                // Build other matricies dependent on the view to clip matricies
                Matrix4x4 offsetWorldToClipMatrix = offsetViewToClipMatrix * m_worldToViewMatrix;
                Matrix4x4 offsetWorldToClipPrevMatrix = offsetViewToClipPrevMatrix * m_worldToViewPrevMatrix;
            
                Matrix4x4 offsetClipToViewMatrix = offsetViewToClipMatrix.GetInverseFull();
                Matrix4x4 offsetClipToWorldMatrix = m_viewToWorldMatrix * offsetClipToViewMatrix;
                    
                m_shaderResourceGroup->SetConstant(m_worldToClipPrevMatrixConstantIndex, offsetWorldToClipPrevMatrix);
                m_shaderResourceGroup->SetConstant(m_viewProjectionMatrixConstantIndex, offsetWorldToClipMatrix);
                m_shaderResourceGroup->SetConstant(m_projectionMatrixConstantIndex, offsetViewToClipMatrix);
                m_shaderResourceGroup->SetConstant(m_clipToWorldMatrixConstantIndex, offsetClipToWorldMatrix);
                m_shaderResourceGroup->SetConstant(m_projectionMatrixInverseConstantIndex, offsetViewToClipMatrix.GetInverseFull());
            }

            m_shaderResourceGroup->SetConstant(m_worldPositionConstantIndex, m_position);
            m_shaderResourceGroup->SetConstant(m_viewMatrixConstantIndex, m_worldToViewMatrix);
            m_shaderResourceGroup->SetConstant(m_viewMatrixInverseConstantIndex, m_worldToViewMatrix.GetInverseFull());
            m_shaderResourceGroup->SetConstant(m_zConstantsConstantIndex, m_nearZ_farZ_farZTimesNearZ_farZMinusNearZ);
            m_shaderResourceGroup->SetConstant(m_unprojectionConstantsIndex, m_unprojectionConstants);

            m_shaderResourceGroup->Compile();

            m_viewToClipPrevMatrix = m_viewToClipMatrix;
            m_worldToViewPrevMatrix = m_worldToViewMatrix;

            m_clipSpaceOffset.Set(0);
        }

        void View::BeginCulling()
        {
#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
            m_maskedOcclusionCulling->ClearBuffer();
#endif
        }

        MaskedOcclusionCulling* View::GetMaskedOcclusionCulling()
        {
            return m_maskedOcclusionCulling;
        }
    } // namespace RPI
} // namespace AZ
