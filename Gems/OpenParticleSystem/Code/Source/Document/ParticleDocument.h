/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Serializer/ParticleSourceData.h>
#include <Document/ParticleDocumentBus.h>

namespace OpenParticleSystemEditor
{
    class ParticleDocument
        : private AZ::Data::AssetBus::MultiHandler
        , private AzToolsFramework::AssetSystemBus::Handler
        , private ParticleDocumentRequestBus::Handler
    {
    public:
        AZ_RTTI(ParticleDocument, "{0114614B-17AB-4B4A-9761-EB44F23B6FCA}");
        AZ_CLASS_ALLOCATOR(ParticleDocument, AZ::SystemAllocator, 0);

        ParticleDocument();
        ~ParticleDocument() override;

        bool CreateParticle(const AZStd::string_view absuolutePath);
        void OpenParticle(const AZStd::string_view absuolutePath);

        bool Open(AZStd::string_view loadPath);
        bool Save();
        bool Close();
        bool UpdateAsset();

        bool IsOpen() const;
        bool IsModified() const;
        AZStd::string_view GetAbsolutePath() const;

    private:

        // AssetSystemBus
        void SourceFileChanged(AZStd::string /*relativePath*/, AZStd::string /*scanFolder*/, AZ::Uuid /*sourceUUID*/) override;

        // AssetBus
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        OpenParticle::ParticleSourceData* GetParticleSourceData() override;
        void NotifyParticleSourceDataModified() override;
        OpenParticle::ParticleSourceData::DetailInfo* AddDetail(const AZStd::string&) override;
        OpenParticle::ParticleSourceData::DetailInfo* CopyDetail(AZStd::string&) override;
        void RemoveDetail(OpenParticle::ParticleSourceData::DetailInfo*) override;

        void SetCopyName(AZStd::string copyName);
        OpenParticle::ParticleSourceData::DetailInfo* GetDetail();
        OpenParticle::ParticleSourceData::EmitterInfo* GetEmitter();
        OpenParticle::ParticleSourceData::DetailInfo* SetEmitterAndDetail(OpenParticle::ParticleSourceData::EmitterInfo* destEmitter, OpenParticle::ParticleSourceData::DetailInfo* destDetail);
        AZStd::string GetCopyWidgetName();
        void ClearCopyCache();

        AZStd::string m_relativePath;
        AZStd::string m_absolutePath;
        AZ::Data::AssetId m_sourceAssetId;
        AZ::Data::Asset<OpenParticle::ParticleAsset> m_particleAsset;

        AZ::SerializeContext* m_serializeContext = nullptr;
        OpenParticle::ParticleSourceData m_sourceData;
        bool m_modified = false;

        OpenParticle::ParticleSourceData::DetailInfo* m_copyDetail = nullptr;
        OpenParticle::ParticleSourceData::EmitterInfo* m_copyEmitter = nullptr;
        AZStd::string m_copyWidgetTitleName = "";
    };
}
