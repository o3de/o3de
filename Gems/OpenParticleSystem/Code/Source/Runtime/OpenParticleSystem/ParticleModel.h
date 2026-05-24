/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Model/UvStreamTangentBitmask.h>

#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/RHI/DrawItem.h>

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Limits.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Model/ModelLod.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>

#include <AtomCore/std/containers/vector_set.h>

#include <AzCore/std/containers/fixed_vector.h>

namespace OpenParticle
{
    class ParticleModel
    {
    public:
        ParticleModel() = default;
        ~ParticleModel() = default;

        void SetupModel(AZ::Data::Asset<AZ::RPI::ModelAsset>& model, size_t lodIndex);

        size_t GetMeshCount() const;

        AZ::Data::Instance<AZ::RPI::ModelLod> GetModelLod() const;

        bool BuildInputStreamLayouts(const AZ::RPI::ShaderInputContract& contract);

        void SetParticleStreamBufferView(const AZ::RHI::StreamBufferView& streamBufferView, size_t meshIndex);

        bool IsInputStreamLayoutsValid(size_t meshIndex) const;

        const AZ::RHI::InputStreamLayout& GetInputStreamLayout(size_t meshIndex) const;

        const AZ::RPI::ModelLod::StreamBufferViewList& GetStreamBufferViewList(size_t meshIndex) const;

    private:
        AZ::RPI::ShaderInputContract CheckMeshContract(const AZ::RPI::ShaderInputContract& contract);

        bool SetMeshStreamBuffers();

        bool GetStreamsForMesh(
            AZ::RHI::InputStreamLayoutBuilder& layoutBuilder,
            AZ::RPI::UvStreamTangentBitmask* uvStreamTangentBitmaskOut,
            const AZ::RPI::ShaderInputContract& contract,
            size_t meshIndex,
            const AZ::RPI::MaterialModelUvOverrideMap& materialModelUvMap = {},
            const AZ::RPI::MaterialUvNameMap& materialUvNameMap = {});

        AZ::RPI::ModelLod::StreamInfoList::const_iterator FindMatchingStream(
            size_t meshIndex,
            const AZ::RPI::MaterialModelUvOverrideMap& materialModelUvMap,
            const AZ::RPI::MaterialUvNameMap& materialUvNameMap,
            const AZ::RPI::ShaderInputContract::StreamChannelInfo& contractStreamChannel,
            AZ::RPI::ModelLod::StreamInfoList::const_iterator defaultUv,
            AZ::RPI::ModelLod::StreamInfoList::const_iterator firstUv,
            AZ::RPI::UvStreamTangentBitmask* uvStreamTangentBitmaskOut);

        AZ::RPI::ModelLod::StreamInfoList::const_iterator FindFirstUvStreamFromMesh(size_t meshIndex);
        AZ::RPI::ModelLod::StreamInfoList::const_iterator FindDefaultUvStream(
            size_t meshIndex, const AZ::RPI::MaterialUvNameMap& materialUvNameMap);

        AZ::Data::Instance<AZ::RPI::Model> m_modelInstance;
        AZ::Data::Instance<AZ::RPI::ModelLod> m_modelLod;
        size_t m_lodIndex = 0;
        AZStd::vector<AZ::RHI::InputStreamLayout> m_inputStreamLayouts;
        AZStd::vector<AZ::RPI::ModelLod::StreamBufferViewList> m_streamBufferViews;
        AZStd::vector<AZ::Data::Instance<AZ::RPI::Buffer>> m_buffers;
    };
} // namespace OpenParticle

