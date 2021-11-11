/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessor.h>
#include <AzCore/Serialization/ObjectStream.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class PrefabProcessorContext;
}
   
namespace Multiplayer
{
    using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessor;
    using AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext;
    using AzToolsFramework::Prefab::PrefabDom;

    class NetworkPrefabProcessor : public PrefabProcessor
    {
    public:
        AZ_CLASS_ALLOCATOR(NetworkPrefabProcessor, AZ::SystemAllocator, 0);
        AZ_RTTI(Multiplayer::NetworkPrefabProcessor, "{AF6C36DA-CBB9-4DF4-AE2D-7BC6CCE65176}", PrefabProcessor);

        ~NetworkPrefabProcessor() override = default;

        void Process(PrefabProcessorContext& context) override;

        static void Reflect(AZ::ReflectContext* context);

        //! The format the network spawnables are going to be stored in.
        enum class SerializationFormats
        {
            Binary, //!< Binary is generally preferable for performance.
            Text //!< Store in text format which is usually slower but helps with debugging.
        };

        AZ::DataStream::StreamType GetAzSerializationFormat() const;

    protected:
        static void ProcessPrefab(PrefabProcessorContext& context, AZStd::string_view prefabName, PrefabDom& prefab, AZ::DataStream::StreamType serializationFormat);

        SerializationFormats m_serializationFormat = SerializationFormats::Binary;
    };
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(Multiplayer::NetworkPrefabProcessor::SerializationFormats, "{F69B49EB-9D67-4D9C-99E7-DFA35D4ACCD2}");
}
