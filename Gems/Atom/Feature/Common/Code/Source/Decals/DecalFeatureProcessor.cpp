/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Decals/DecalFeatureProcessor.h>

#include <AzCore/Debug/EventTrace.h>

#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <AzCore/Math/Quaternion.h>
#include <AtomCore/std/containers/array_view.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    namespace Render
    {
        DecalFeatureProcessor::DecalFeatureProcessor()
            : m_baseColorMapShaderName("m_decalBaseColorMaps")
            , m_opacityMapShaderName("m_opacityMaps")
            , m_baseColorMapPropertyName("baseColor.textureMap")
            , m_opacityMapPropertyName("opacity.textureMap")
        {
        }

        void DecalFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<DecalFeatureProcessor, RPI::FeatureProcessor>()
                    ->Version(0);
            }
        }

        void DecalFeatureProcessor::Activate()
        {
            GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = "DecalBuffer";
            desc.m_bufferSrgName = "m_decals";
            desc.m_elementCountSrgName = "m_decalCount";
            desc.m_elementSize = sizeof(DecalData);
            desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();

            m_decalBufferHandler = GpuBufferHandler(desc);

            CacheShaderIndices();
        }

        void DecalFeatureProcessor::Deactivate()
        {
            m_decalData.Clear();
            m_decalBufferHandler.Release();
        }

        DecalFeatureProcessor::DecalHandle DecalFeatureProcessor::AcquireDecal()
        {
            uint16_t id = m_decalData.GetFreeSlotIndex();

            if (id == DataVector::NoFreeSlot)
            {
                return DecalHandle(DecalHandle::NullIndex);
            }
            else
            {
                m_deviceBufferNeedsUpdate = true;
                return DecalHandle(id);
            }
        }

        bool DecalFeatureProcessor::ReleaseDecal(DecalHandle decal)
        {
            if (decal.IsValid())
            {
                m_decalData.RemoveIndex(decal.GetIndex());
                m_deviceBufferNeedsUpdate = true;
                return true;
            }
            return false;
        }

        DecalFeatureProcessor::DecalHandle DecalFeatureProcessor::CloneDecal(DecalHandle sourceDecal)
        {
            AZ_Assert(sourceDecal.IsValid(), "Invalid DecalHandle passed to DecalFeatureProcessor::CloneDecal().");

            DecalHandle decal = AcquireDecal();
            if (decal.IsValid())
            {
                m_decalData.GetData<0>(decal.GetIndex()) = m_decalData.GetData<0>(sourceDecal.GetIndex());
                m_decalData.GetData<1>(decal.GetIndex()) = m_decalData.GetData<1>(sourceDecal.GetIndex());

                m_deviceBufferNeedsUpdate = true;
            }
            return decal;
        }

        void DecalFeatureProcessor::Simulate(const RPI::FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "DecalFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            if (m_deviceBufferNeedsUpdate)
            {
                m_decalBufferHandler.UpdateBuffer(m_decalData.GetDataVector<0>());
                m_deviceBufferNeedsUpdate = false;
            }
        }

        AZStd::array_view<Data::Instance<RPI::Image>> DecalFeatureProcessor::GetImageArray() const
        {
            // [GFX TODO][ATOM-4445] Replace this hardcoded constant with atlasing / bindless so we can have far more than 8 decal textures
            // Note this constant also is defined in View.srg
            const size_t MaxDecals = 8;
            size_t numImages = AZStd::min(MaxDecals, m_decalData.GetDataCount());

            AZStd::array_view<ImagePtr> imageArrayView(m_decalData.GetDataVector<1>().begin(), m_decalData.GetDataVector<1>().begin() + numImages);
            return imageArrayView;
        }


        void DecalFeatureProcessor::Render(const RPI::FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "DecalFeatureProcessor: Render");

            AZStd::array_view<Data::Instance<RPI::Image>> baseMaps = GetImagesFromDecalData<1>();
            AZStd::array_view<Data::Instance<RPI::Image>> opacityMaps = GetImagesFromDecalData<2>();

            for (const RPI::ViewPtr& view : packet.m_views)
            {
                m_decalBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());

                if (baseMaps.empty() == false)
                {
                    view->GetShaderResourceGroup()->SetImageArray(m_baseColorMapsIndex, baseMaps);
                }
                if (opacityMaps.empty() == false)
                {
                    view->GetShaderResourceGroup()->SetImageArray(m_opacityMapsIndex, opacityMaps);
                }
            }
        }

        void DecalFeatureProcessor::SetDecalData(DecalHandle handle, const DecalData& data)
        {
            if (handle.IsValid())
            {
                m_decalData.GetData<0>(handle.GetIndex()) = data;
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalFeatureProcessor", false, "Invalid handle passed to DecalFeatureProcessor::SetDecalData().");
            }
        }

        const Data::Instance<RPI::Buffer> DecalFeatureProcessor::GetDecalBuffer()const
        {
            return m_decalBufferHandler.GetBuffer();
        }

        uint32_t DecalFeatureProcessor::GetDecalCount() const
        {
            return m_decalBufferHandler.GetElementCount();
        }

        void DecalFeatureProcessor::SetDecalPosition(DecalHandle handle, const AZ::Vector3& position)
        {
            if (handle.IsValid())
            {
                AZStd::array<float, 3>& writePos = m_decalData.GetData<0>(handle.GetIndex()).m_position;

                position.StoreToFloat3(writePos.data());

                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalFeatureProcessor", false, "Invalid handle passed to DecalFeatureProcessor::SetDecalPosition().");
            }
        }

        void DecalFeatureProcessor::SetDecalOrientation(DecalHandle handle, const AZ::Quaternion& orientation)
        {
            if (handle.IsValid())
            {
                orientation.StoreToFloat4(m_decalData.GetData<0>(handle.GetIndex()).m_quaternion.data());
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalFeatureProcessor", false, "Invalid handle passed to DecalFeatureProcessor::SetDecalOrientation().");
            }
        }

        void DecalFeatureProcessor::SetDecalHalfSize(DecalHandle handle, const Vector3& halfSize)
        {
            if (handle.IsValid())
            {
                halfSize.StoreToFloat3(m_decalData.GetData<0>(handle.GetIndex()).m_halfSize.data());
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalFeatureProcessor", false, "Invalid handle passed to DecalFeatureProcessor::SetDecalHalfSize().");
            }
        }

        void DecalFeatureProcessor::SetDecalAttenuationAngle(DecalHandle handle, float angleAttenuation)
        {
            if (handle.IsValid())
            {
                m_decalData.GetData<0>(handle.GetIndex()).m_angleAttenuation = angleAttenuation;

                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalFeatureProcessor", handle.IsValid(), "Invalid handle passed to DecalFeatureProcessor::SetDecalAttenuationAngle().");
            }
        }

        void DecalFeatureProcessor::SetDecalOpacity(DecalHandle handle, float opacity)
        {
            if (handle.IsValid())
            {
                m_decalData.GetData<0>(handle.GetIndex()).m_opacity = opacity;

                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalFeatureProcessor", false, "Invalid handle passed to DecalFeatureProcessor::SetDecalOpacity().");
            }
        }

        void DecalFeatureProcessor::SetDecalSortKey(DecalHandle handle, uint8_t sortKey)
        {
            if (handle.IsValid())
            {
                m_decalData.GetData<0>(handle.GetIndex()).m_sortKey = sortKey;
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalFeatureProcessor", false, "Invalid handle passed to DecalFeatureProcessor::SetDecalSortKey().");
            }
        }

        void DecalFeatureProcessor::SetDecalTransform(DecalHandle handle, const AZ::Transform& world)
        {
            SetDecalTransform(handle, world, AZ::Vector3::CreateOne());
        }

        void DecalFeatureProcessor::SetDecalTransform(DecalHandle handle, const AZ::Transform& world, const AZ::Vector3& nonUniformScale)
        {
            // ATOM-4330
            // Original Open 3D Engine uploads a 4x4 matrix rather than quaternion, rotation, scale.
            // That is more memory but less calculation because it is doing a matrix inverse rather than a polar decomposition
            // I've done some experiments and uploading a 3x4 transform matrix with 3x3 matrix inverse should be possible
            // I am putting it as part of a separate Jira because I would have to upload different data to the light culling system
            // (not a bad thing, but it would make this commit quite a bit more complex)

            if (handle.IsValid())
            {
                Quaternion orientation = world.GetRotation();
                Vector3 scale = world.GetUniformScale() * nonUniformScale;

                SetDecalHalfSize(handle, scale);
                SetDecalPosition(handle, world.GetTranslation());
                SetDecalOrientation(handle, orientation);

                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalFeatureProcessor", false, "Invalid handle passed to DecalFeatureProcessor::SetTransform().");
            }
        }

        Data::Instance<RPI::Image> DecalFeatureProcessor::GetImageFromMaterial(const AZ::Name& materialPropertyName, Data::Instance<RPI::Material> materialInstance) const
        {
            Data::Instance<RPI::Image> image;
            RPI::MaterialPropertyIndex index = materialInstance->FindPropertyIndex(materialPropertyName);
            if (index.IsValid())
            {
                image = materialInstance->GetPropertyValue<Data::Instance<RPI::Image>>(index);
            }
            else
            {
                AZ_Warning("DecalFeatureProcessor", false, "Unable to find %s in material.", materialPropertyName.GetCStr());
            }
            return image;
        }

        void DecalFeatureProcessor::SetDecalMaterial(DecalHandle handle, const AZ::Data::AssetId materialAssetId)
        {
            if (handle.IsNull())
            {
                AZ_Warning("DecalFeatureProcessor", false, "Invalid handle passed to DecalFeatureProcessor::SetMaterial().");
                return;
            }

            if (materialAssetId.IsValid())
            {
                const auto materialAsset = AZ::RPI::AssetUtils::LoadAssetById<AZ::RPI::MaterialAsset>(materialAssetId);
                Data::Instance<RPI::Material> materialInstance = AZ::RPI::Material::FindOrCreate(materialAsset);
                if (materialInstance)
                {
                    const auto baseColorImage = GetImageFromMaterial(m_baseColorMapPropertyName, materialInstance);
                    const auto opacityImage = GetImageFromMaterial(m_opacityMapPropertyName, materialInstance);
                    m_decalData.GetData<1>(handle.GetIndex()) = baseColorImage;
                    m_decalData.GetData<2>(handle.GetIndex()) = opacityImage;
                }
            }
        }

        void DecalFeatureProcessor::CacheShaderIndices()
        {
            const RHI::ShaderResourceGroupLayout* viewSrgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();
            m_baseColorMapsIndex = viewSrgLayout->FindShaderInputImageIndex(m_baseColorMapShaderName);
            AZ_Warning("DecalFeatureProcessor", m_baseColorMapsIndex.IsValid(), "Unable to find baseColorMaps in decal shader.");

            m_opacityMapsIndex = viewSrgLayout->FindShaderInputImageIndex(m_opacityMapShaderName);
            AZ_Warning("DecalFeatureProcessor", m_opacityMapsIndex.IsValid(), "Unable to find opacityMaps in decal shader.");
        }
    } // namespace Render
} // namespace AZ
