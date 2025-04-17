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
#include <Atom/RHI/RHIUtils.h>

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Task/TaskGraph.h>
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

            // Set default matrices
            SetWorldToViewMatrix(AZ::Matrix4x4::CreateIdentity());
            AZ::Matrix4x4 viewToClipMatrix;
            AZ::MakePerspectiveFovMatrixRH(viewToClipMatrix, AZ::Constants::HalfPi, 1, 0.1f, 1000.f, true);
            SetViewToClipMatrix(viewToClipMatrix);

            if ((usage & UsageFlags::UsageXR))
            {
                SetViewToClipMatrix(AZ::Matrix4x4::CreateIdentity());
            }

            TryCreateShaderResourceGroup();

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
            if (m_drawListMask != drawListMask)
            {
                m_drawListMask = drawListMask;
                m_drawListContext.Shutdown();
                m_drawListContext.Init(m_drawListMask);
            }
        }

        void View::Reset()
        {
            m_drawListMask.reset();
            m_drawListContext.Shutdown();
            m_visibleObjectContext.Shutdown();
            m_passesByDrawList = nullptr;
        }

        void View::PrintDrawListMask()
        {
            AZ_Printf("View", RHI::DrawListMaskToString(m_drawListMask).c_str());
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

        void View::AddDrawPacket(const RHI::DrawPacket* drawPacket, const Vector3& worldPosition)
        {
            Vector3 cameraToObject = worldPosition - m_position;
            float depth = cameraToObject.Dot(-m_viewToWorldMatrix.GetBasisZAsVector3());
            AddDrawPacket(drawPacket, depth);
        }

        void View::AddVisibleObject(const void* userData, float depth)
        {
            // This function is thread safe since VisibleObjectContext has storage per thread for draw item data.
            m_visibleObjectContext.AddVisibleObject(userData, depth);
        }

        void View::AddVisibleObject(const void* userData, const Vector3& worldPosition)
        {
            Vector3 cameraToObject = worldPosition - m_position;
            float depth = cameraToObject.Dot(-m_viewToWorldMatrix.GetBasisZAsVector3());
            AddVisibleObject(userData, depth);
        }

        void View::AddDrawItem(RHI::DrawListTag drawListTag, const RHI::DrawItemProperties& drawItemProperties)
        {
            m_drawListContext.AddDrawItem(drawListTag, drawItemProperties);
        }

        void View::ApplyFlags(uint32_t flags)
        {
            AZStd::atomic_fetch_and(&m_andFlags, flags);
            AZStd::atomic_fetch_or(&m_orFlags, flags);
        }

        void View::ClearFlags(uint32_t flags)
        {
            AZStd::atomic_fetch_or(&m_andFlags, flags);
            AZStd::atomic_fetch_and(&m_orFlags, ~flags);
        }

        void View::ClearAllFlags()
        {
            ClearFlags(0xFFFFFFFF);
        }

        uint32_t View::GetAndFlags() const
        {
            return m_andFlags;
        }

        uint32_t View::GetOrFlags() const
        {
            return m_orFlags;
        }

        void View::UpdateViewToWorldMatrix(const AZ::Matrix4x4& viewToWorld)
        {
            m_viewToWorldMatrix = viewToWorld;

            //Update view transform
            static const Quaternion yUpToZUp = Quaternion::CreateRotationX(-AZ::Constants::HalfPi);
            m_viewTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                       Quaternion::CreateFromMatrix4x4(m_viewToWorldMatrix) * yUpToZUp, m_viewToWorldMatrix.GetTranslation()).GetOrthogonalized();
        }

        void View::SetWorldToViewMatrix(const AZ::Matrix4x4& worldToView)
        {
            UpdateViewToWorldMatrix(worldToView.GetInverseFast());
            m_position = m_viewToWorldMatrix.GetTranslation();

            m_worldToViewMatrix = worldToView;
            m_worldToClipMatrix = m_viewToClipMatrix * m_worldToViewMatrix;
            if (m_viewToClipExcludeMatrix.has_value())
            {
                m_worldToClipExcludeMatrix = m_viewToClipExcludeMatrix.value() * m_worldToViewMatrix;
            }
            m_clipToWorldMatrix = m_worldToClipMatrix.GetInverseFull();

            m_onWorldToViewMatrixChange.Signal(m_worldToViewMatrix);
            m_onWorldToClipMatrixChange.Signal(m_worldToClipMatrix);
        }

        AZ::Transform View::GetCameraTransform() const
        {
            return m_viewTransform;
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
            UpdateViewToWorldMatrix(AZ::Matrix4x4::CreateFromRowMajorFloat16(viewToWorldMatrixRaw));

            m_worldToViewMatrix = m_viewToWorldMatrix.GetInverseFast();

            m_worldToClipMatrix = m_viewToClipMatrix * m_worldToViewMatrix;
            if (m_viewToClipExcludeMatrix.has_value())
            {
                m_worldToClipExcludeMatrix = m_viewToClipExcludeMatrix.value() * m_worldToViewMatrix;
            }
            m_clipToWorldMatrix = m_worldToClipMatrix.GetInverseFull();

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
            m_clipToViewMatrix = m_viewToClipMatrix.GetInverseFull();
            m_worldToClipMatrix = m_viewToClipMatrix * m_worldToViewMatrix;
            m_clipToWorldMatrix = m_worldToClipMatrix.GetInverseFull();

            // Update z depth constant simultaneously
            // zNear -> n, zFar -> f
            // A = f / (n - f), B = nf / (n - f)
            double A = m_viewToClipMatrix.GetElement(2, 2);
            double B = m_viewToClipMatrix.GetElement(2, 3);

            // Based on linearZ = fn / (depth*(f-n) - f)
            m_linearizeDepthConstants.SetX(float(B / A)); //<------------n
            m_linearizeDepthConstants.SetY(float(B / (A + 1.0))); //<---------f
            m_linearizeDepthConstants.SetZ(float((B * B) / (A * (A + 1.0)))); //<-----nf
            m_linearizeDepthConstants.SetW(float(-B / (A * (A + 1.0)))); //<------f-n

            // For reverse depth the expression we dont have to do anything different as m_linearizeDepthConstants works out to be the same.
            // A = n / (f - n), B = nf / (f - n)
            // Based on linearZ = fn / (depth*(n-f) - n)
            //m_linearizeDepthConstants.SetX(float(B / A)); //<----f
            //m_linearizeDepthConstants.SetY(float(B / (A + 1.0))); //<----n
            //m_linearizeDepthConstants.SetZ(float((B * B) / (A * (A + 1.0)))); //<----nf
            //m_linearizeDepthConstants.SetW(float(-B / (A * (A + 1.0)))); //<-----n-f

            double tanHalfFovX = m_clipToViewMatrix.GetElement(0, 0);
            double tanHalfFovY = m_clipToViewMatrix.GetElement(1, 1);

            // The constants below try to remapping 0---1 to -1---+1 and multiplying with inverse of projection.
            // Assuming that inverse of projection matrix only has value in the first column for first row
            // x = (2u-1)*ProjInves[0][0]
            // Assuming that inverse of projection matrix only has value in the second column for second row
            // y = (1-2v)*ProjInves[1][1]
            m_unprojectionConstants.SetX(float(2.0 * tanHalfFovX));
            m_unprojectionConstants.SetY(float(-2.0 * tanHalfFovY));
            m_unprojectionConstants.SetZ(float(-tanHalfFovX));
            m_unprojectionConstants.SetW(float(tanHalfFovY));

            m_onWorldToClipMatrixChange.Signal(m_worldToClipMatrix);
        }

        void View::SetViewToClipExcludeMatrix(const AZ::Matrix4x4* viewToClipExclude)
        {
            if (viewToClipExclude)
            {
                m_viewToClipExcludeMatrix = *viewToClipExclude;
                m_worldToClipExcludeMatrix = *viewToClipExclude * m_worldToViewMatrix;
            }
            else
            {
                m_viewToClipExcludeMatrix.reset();
                m_worldToClipExcludeMatrix.reset();
            }
        }

        void View::SetStereoscopicViewToClipMatrix(const AZ::Matrix4x4& viewToClip, bool reverseDepth)
        {
            m_viewToClipMatrix = viewToClip;
            m_clipToViewMatrix = m_viewToClipMatrix.GetInverseFull();

            m_worldToClipMatrix = m_viewToClipMatrix * m_worldToViewMatrix;
            m_clipToWorldMatrix = m_worldToClipMatrix.GetInverseFull();

            // Update z depth constant simultaneously
            if(reverseDepth)
            {
                // zNear -> n, zFar -> f
                // A = 2n/(f-n), B = 2fn / (f - n)
                // the formula of A and B should be the same as projection matrix's definition
                // currently defined in CreateStereoscopicProjection in XRUtils.cpp
                double A = m_viewToClipMatrix.GetElement(2, 2);
                double B = m_viewToClipMatrix.GetElement(2, 3);

                // Based on linearZ = 2fn / (depth*(n-f) - 2n)
                m_linearizeDepthConstants.SetX(float(B / A)); //<----f
                m_linearizeDepthConstants.SetY(float((2 * B) / (A + 2.0))); //<--- 2n
                m_linearizeDepthConstants.SetZ(float((2 * B * B) / (A * (A + 2.0)))); //<-----2fn
                m_linearizeDepthConstants.SetW(float((-2 * B) / (A * (A + 2.0)))); //<------n-f
            }
            else
            {
                // A = -(f+n)/(f-n), B = -2fn / (f - n)
                double A = m_viewToClipMatrix.GetElement(2, 2);
                double B = m_viewToClipMatrix.GetElement(2, 3);

                //Based on linearZ = 2fn / (depth*(f-n) - (-f-n))
                m_linearizeDepthConstants.SetX(float(B / (A + 1.0))); //<----f
                m_linearizeDepthConstants.SetY(float((-2 * B * A)/ ((A + 1.0) * (A - 1.0)))); //<--- -f-n
                m_linearizeDepthConstants.SetZ(float((2 * B * B) / ((A - 1.0) * (A + 1.0)))); //<-----2fn
                m_linearizeDepthConstants.SetW(float((-2 * B) / ((A - 1.0) * (A + 1.0)))); //<------f-n
            }

            // The constants below try to remap 0---1 to -1---+1 and multiply with inverse of projection.
            // Assuming that inverse of projection matrix only has value in the first column for first row
            // x = (2u-1)*ProjInves[0][0] + ProjInves[0][3]
            // Assuming that inverse of projection matrix only has value in the second column for second row
            // y = (1-2v)*ProjInves[1][1] + ProjInves[1][3]
            float multiplierConstantX = 2.0f * m_clipToViewMatrix.GetElement(0, 0);
            float multiplierConstantY = -2.0f * m_clipToViewMatrix.GetElement(1, 1);
            float additionConstantX = m_clipToViewMatrix.GetElement(0, 3) - m_clipToViewMatrix.GetElement(0, 0);
            float additionConstantY = m_clipToViewMatrix.GetElement(1, 1) + m_clipToViewMatrix.GetElement(1, 3);

            m_unprojectionConstants.SetX(multiplierConstantX);
            m_unprojectionConstants.SetY(multiplierConstantY);
            m_unprojectionConstants.SetZ(additionConstantX);
            m_unprojectionConstants.SetW(additionConstantY);

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

        const AZ::Matrix4x4& View::GetClipToViewMatrix() const
        {
            return m_clipToViewMatrix;
        }

        const Matrix4x4& View::GetWorldToClipPrevMatrixWithOffset() const
        {
            return m_worldToClipPrevMatrixWithOffset;
        }

        const Matrix4x4& View::GetWorldToClipMatrixWithOffset() const
        {
            return m_worldToClipMatrixWithOffset;
        }

        const Matrix4x4& View::GetViewToClipMatrixWithOffset() const
        {
            return m_viewToClipMatrixWithOffset;
        }

        const Matrix4x4& View::GetClipToWorldMatrixWithOffset() const
        {
            return m_clipToWorldMatrixWithOffset;
        }

        const Matrix4x4& View::GetClipToViewMatrixWithOffset() const
        {
            return m_clipToViewMatrixWithOffset;
        }

        AZ::Matrix3x4 View::GetWorldToViewMatrixAsMatrix3x4() const
        {
            return AZ::Matrix3x4::UnsafeCreateFromMatrix4x4(m_worldToViewMatrix);
        }

        AZ::Matrix3x4 View::GetViewToWorldMatrixAsMatrix3x4() const
        {
            return AZ::Matrix3x4::UnsafeCreateFromMatrix4x4(m_viewToWorldMatrix);
        }

        const AZ::Matrix4x4& View::GetViewToClipMatrix() const
        {
            return m_viewToClipMatrix;
        }

        const AZ::Matrix4x4* View::GetWorldToClipExcludeMatrix() const
        {
            return m_worldToClipExcludeMatrix.has_value() ? &m_worldToClipExcludeMatrix.value() : nullptr;
        }

        const AZ::Matrix4x4& View::GetWorldToClipMatrix() const
        {
            return m_worldToClipMatrix;
        }

        const AZ::Matrix4x4& View::GetClipToWorldMatrix() const
        {
            return m_clipToWorldMatrix;
        }

        bool View::HasDrawListTag(RHI::DrawListTag drawListTag)
        {
            return drawListTag.IsValid() && m_drawListMask[drawListTag.GetIndex()];
        }

        RHI::DrawListView View::GetDrawList(RHI::DrawListTag drawListTag)
        {
            return m_drawListContext.GetList(drawListTag);
        }

        VisibleObjectListView View::GetVisibleObjectList()
        {
            return m_visibleObjectContext.GetList();
        }

        void View::FinalizeVisibleObjectList()
        {
            m_visibleObjectContext.FinalizeLists();
        }

        void View::FinalizeDrawListsTG(AZ::TaskGraphEvent& finalizeDrawListsTGEvent)
        {
            AZ_PROFILE_SCOPE(RPI, "View: FinalizeDrawLists");
            m_drawListContext.FinalizeLists();
            SortFinalizedDrawListsTG(finalizeDrawListsTGEvent);
        }
        void View::FinalizeDrawListsJob(AZ::Job* parentJob)
        {
            AZ_PROFILE_SCOPE(RPI, "View: FinalizeDrawLists");
            m_drawListContext.FinalizeLists();
            SortFinalizedDrawListsJob(parentJob);
        }

        void View::SortFinalizedDrawListsTG(AZ::TaskGraphEvent& finalizeDrawListsTGEvent)
        {
            AZ_PROFILE_SCOPE(RPI, "View: SortFinalizedDrawLists");
            RHI::DrawListsByTag& drawListsByTag = m_drawListContext.GetMergedDrawListsByTag();

            AZ::TaskGraph drawListSortTG{ "DrawList Sort" };
            AZ::TaskDescriptor drawListSortTGDescriptor{"RPI_View_SortFinalizedDrawLists", "Graphics"};
            for (size_t idx = 0; idx < drawListsByTag.size(); ++idx)
            {
                if (drawListsByTag[idx].size() > 1)
                {
                    drawListSortTG.AddTask(drawListSortTGDescriptor, [this, &drawListsByTag, idx]()
                    {
                        AZ_PROFILE_SCOPE(RPI, "View: SortDrawList Task");
                        SortDrawList(drawListsByTag[idx], RHI::DrawListTag(idx));
                    });
                }
            }
            if (!drawListSortTG.IsEmpty())
            {
                drawListSortTG.Detach();
                drawListSortTG.Submit(&finalizeDrawListsTGEvent);
            }
        }

        void View::SortFinalizedDrawListsJob(AZ::Job* parentJob)
        {
            AZ_PROFILE_SCOPE(RPI, "View: SortFinalizedDrawLists");
            RHI::DrawListsByTag& drawListsByTag = m_drawListContext.GetMergedDrawListsByTag();

            AZ::JobCompletion jobCompletion;
            for (size_t idx = 0; idx < drawListsByTag.size(); ++idx)
            {
                if (drawListsByTag[idx].size() > 1)
                {
                    auto jobLambda = [this, &drawListsByTag, idx]()
                    {
                        AZ_PROFILE_SCOPE(RPI, "View: SortDrawList Job");
                        SortDrawList(drawListsByTag[idx], RHI::DrawListTag(idx));
                    };
                    Job* jobSortDrawList = aznew JobFunction<decltype(jobLambda)>(jobLambda, true, nullptr); // Auto-deletes
                    if (parentJob)
                    {
                        parentJob->StartAsChild(jobSortDrawList);
                    }
                    else
                    {
                        jobSortDrawList->SetDependent(&jobCompletion);
                        jobSortDrawList->Start();
                    }
                }
            }
            if (parentJob)
            {
                parentJob->WaitForChildren();
            }
            else
            {
                jobCompletion.StartAndWaitForCompletion();
            }
        }

        void View::SortDrawList(RHI::DrawList& drawList, RHI::DrawListTag tag)
        {
            if (!m_passesByDrawList)
            {
                // Nothing to sort.
                return;
            }

            // Note: it's possible that the m_passesByDrawList doesn't have a pass for the input tag.
            // This is because a View can be used for multiple render pipelines.
            // So it may contains draw list tag which exists in one render pipeline but not others.
            auto itr = m_passesByDrawList->find(tag);
            if (itr != m_passesByDrawList->end())
            {
                itr->second->SortDrawList(drawList);
            }
        }

        void View::ConnectWorldToViewMatrixChangedHandler(MatrixChangedEvent::Handler& handler)
        {
            handler.Connect(m_onWorldToViewMatrixChange);
        }

        void View::ConnectWorldToClipMatrixChangedHandler(MatrixChangedEvent::Handler& handler)
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

        const AZ::Name& View::GetName() const
        {
            return m_name;
        }

        View::UsageFlags View::GetUsageFlags() const
        {
            return m_usageFlags;
        }

        void View::SetPassesByDrawList(PassesByDrawList* passes)
        {
            m_passesByDrawList = passes;
        }

        void View::UpdateSrg()
        {
            if (m_clipSpaceOffset.IsZero())
            {
                m_worldToClipPrevMatrixWithOffset = m_viewToClipPrevMatrix * m_worldToViewPrevMatrix;
                m_worldToClipMatrixWithOffset = m_worldToClipMatrix;
                m_viewToClipMatrixWithOffset = m_viewToClipMatrix;
                m_clipToWorldMatrixWithOffset = m_clipToWorldMatrix;
                m_clipToViewMatrixWithOffset = m_clipToViewMatrix;
            }
            else
            {
                // Offset the current and previous frame clip matrices
                Matrix4x4 offsetViewToClipMatrix = m_viewToClipMatrix;
                offsetViewToClipMatrix.SetElement(0, 2, m_clipSpaceOffset.GetX());
                offsetViewToClipMatrix.SetElement(1, 2, m_clipSpaceOffset.GetY());

                Matrix4x4 offsetViewToClipPrevMatrix = m_viewToClipPrevMatrix;
                offsetViewToClipPrevMatrix.SetElement(0, 2, m_clipSpaceOffset.GetX());
                offsetViewToClipPrevMatrix.SetElement(1, 2, m_clipSpaceOffset.GetY());

                // Build other matrices dependent on the view to clip matrices
                Matrix4x4 offsetWorldToClipMatrix = offsetViewToClipMatrix * m_worldToViewMatrix;
                Matrix4x4 offsetWorldToClipPrevMatrix = offsetViewToClipPrevMatrix * m_worldToViewPrevMatrix;

                m_worldToClipPrevMatrixWithOffset = offsetWorldToClipPrevMatrix;
                m_worldToClipMatrixWithOffset = offsetWorldToClipMatrix;
                m_viewToClipMatrixWithOffset = offsetViewToClipMatrix;
                m_clipToWorldMatrixWithOffset = offsetWorldToClipMatrix.GetInverseFull();
                m_clipToViewMatrixWithOffset = offsetViewToClipMatrix.GetInverseFull();
            }

            if (m_shaderResourceGroup)
            {
                m_shaderResourceGroup->SetConstant(m_worldToClipPrevMatrixConstantIndex, m_worldToClipPrevMatrixWithOffset);
                m_shaderResourceGroup->SetConstant(m_viewProjectionMatrixConstantIndex, m_worldToClipMatrix);
                m_shaderResourceGroup->SetConstant(m_projectionMatrixConstantIndex, m_viewToClipMatrix);
                m_shaderResourceGroup->SetConstant(m_clipToWorldMatrixConstantIndex, m_clipToWorldMatrix);
                m_shaderResourceGroup->SetConstant(m_projectionMatrixInverseConstantIndex, m_clipToViewMatrix);

                m_shaderResourceGroup->SetConstant(m_worldPositionConstantIndex, m_position);
                m_shaderResourceGroup->SetConstant(m_viewMatrixConstantIndex, m_worldToViewMatrix);
                m_shaderResourceGroup->SetConstant(m_viewMatrixInverseConstantIndex, m_viewToWorldMatrix);
                m_shaderResourceGroup->SetConstant(m_zConstantsConstantIndex, m_linearizeDepthConstants);
                m_shaderResourceGroup->SetConstant(m_unprojectionConstantsIndex, m_unprojectionConstants);

                m_shaderResourceGroup->Compile();
            }

            m_viewToClipPrevMatrix = m_viewToClipMatrix;
            m_worldToViewPrevMatrix = m_worldToViewMatrix;

            m_clipSpaceOffset.Set(0);
        }

        void View::BeginCulling()
        {
#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
            if (m_maskedOcclusionCullingDirty)
            {
                AZ_PROFILE_SCOPE(RPI, "View: ClearMaskedOcclusionBuffer");
                m_maskedOcclusionCulling->ClearBuffer();
                m_maskedOcclusionCullingDirty = false;
            }
#endif
        }

        MaskedOcclusionCulling* View::GetMaskedOcclusionCulling()
        {
            return m_maskedOcclusionCulling;
        }

        void View::SetMaskedOcclusionCullingDirty(bool dirty)
        {
            m_maskedOcclusionCullingDirty = dirty;
        }

        bool View::GetMaskedOcclusionCullingDirty() const
        {
            return m_maskedOcclusionCullingDirty;
        }

        void View::TryCreateShaderResourceGroup()
        {
            if (!m_shaderResourceGroup)
            {
                if (auto rpiSystemInterface = RPISystemInterface::Get())
                {
                    if (Data::Asset<ShaderAsset> viewSrgShaderAsset = rpiSystemInterface->GetCommonShaderAssetForSrgs();
                        viewSrgShaderAsset.IsReady())
                    {
                        m_shaderResourceGroup =
                            ShaderResourceGroup::Create(viewSrgShaderAsset, rpiSystemInterface->GetViewSrgLayout()->GetName());
                    }
                }
            }
        }

        void View::OnAddToRenderPipeline()
        {
            TryCreateShaderResourceGroup();
            if (!m_shaderResourceGroup)
            {
                AZ_Warning("RPI::View", false, "Shader Resource Group failed to initialize");
            }
        }

        void View::SetShadowPassRenderPipelineId(const RenderPipelineId renderPipelineId)
        {
            m_shadowPassRenderpipelineId = renderPipelineId;
        }

        RenderPipelineId View::GetShadowPassRenderPipelineId() const
        {
            return m_shadowPassRenderpipelineId;
        }

    } // namespace RPI
} // namespace AZ
