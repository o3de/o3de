/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzTest/AzTest.h>

#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Asset/AssetCommon.h>

namespace UnitTest
{
    template <typename T>
    class SerializeTester
    {
    public:
        SerializeTester(AZ::SerializeContext* serializeContext)
            : m_serializeContext{serializeContext}
            , m_outStream{&m_buffer}
        {}
        virtual ~SerializeTester() = default;

        // Serializes an object out to a the internal stream. Resets the stream with each call.
        virtual void SerializeOut(T* object, AZ::DataStream::StreamType streamType = AZ::DataStream::ST_XML);

        // Serializes the object back in. Requires that you call SerializeOut first.
        virtual AZ::Data::Asset<T> SerializeIn(const AZ::Data::AssetId& assetId, AZ::ObjectStream::FilterDescriptor filterDesc = AZ::ObjectStream::FilterDescriptor());

    private:
        AZ::SerializeContext* m_serializeContext = nullptr;
        AZStd::vector<char, AZ::OSStdAllocator> m_buffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char, AZ::OSStdAllocator>> m_outStream;
    };

    template <typename T>
    void SerializeTester<T>::SerializeOut(T* object, AZ::DataStream::StreamType streamType)
    {
        m_buffer.clear();
        m_outStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&m_outStream, *m_serializeContext, streamType);
        bool writeOK = objStream->WriteClass(object);
        ASSERT_TRUE(writeOK);

        // NOTE: This looks like a leak, but Finalize calls 'delete this'.
        bool finalizeOK = objStream->Finalize();
        ASSERT_TRUE(finalizeOK);

        m_outStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
    }

    template <typename T>
    AZ::Data::Asset<T> SerializeTester<T>::SerializeIn(const AZ::Data::AssetId& assetId, AZ::ObjectStream::FilterDescriptor filterDesc)
    {
        AZ::Data::Asset<T> asset = AZ::Data::AssetManager::Instance().CreateAsset<T>(assetId);

        [[maybe_unused]] bool success = AZ::Utils::LoadObjectFromStreamInPlace<T>(m_outStream, *asset.Get(), m_serializeContext, filterDesc);
        AZ_Assert(success, "Failed to load object from stream");

        return AZStd::move(asset);
    }

    // Helper class to test asset saving and loading by utilizing asset handler. 
    // Unlike SerializeTester, it's not restricted to testing assets which are saved with ObjectStream.
    template<class AssetDataT>
    class AssetTester
    {

    public:
        AssetTester()
        {
            m_assetHandler = AZ::Data::AssetManager::Instance().GetHandler(AssetDataT::RTTI_Type());
        }

        virtual ~AssetTester() = default;

        void SerializeOut(AZ::Data::Asset<AssetDataT> assetToSave)
        {
            m_streamBuffer.clear();
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&m_streamBuffer);
            m_assetHandler->SaveAssetData(assetToSave, &stream);
        }

        AZ::Data::Asset<AssetDataT> SerializeIn(const AZ::Data::AssetId& assetId)
        {
            AZ::Data::Asset<AssetDataT> assetToLoad = AZ::Data::AssetManager::Instance().CreateAsset<AssetDataT>(assetId);

            AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();
            assetDataStream->Open(AZStd::move(m_streamBuffer));

            m_assetHandler->LoadAssetDataFromStream(assetToLoad, assetDataStream, {});
            SetAssetReady(assetToLoad);
            return assetToLoad;
        }

        virtual void SetAssetReady([[maybe_unused]] AZ::Data::Asset<AssetDataT>& asset) {};

    protected:
        AZ::Data::AssetHandler* m_assetHandler = nullptr;
        AZStd::vector<AZ::u8> m_streamBuffer;
    };
}
