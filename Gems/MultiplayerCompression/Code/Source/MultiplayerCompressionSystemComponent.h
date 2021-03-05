/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/unordered_set.h>

#include <MultiplayerCompression/MultiplayerCompressionBus.h>
#include <MultiplayerCompressionFactory.h>

namespace MultiplayerCompression
{
    /**
    * System component whose sole purpose is to own a compression factory and expose it via EBUS
    * so GridMate/Multiplayer Gem can easily ingest the compressor.
    */
    class MultiplayerCompressionSystemComponent
        : public AZ::Component
        , protected MultiplayerCompressionRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MultiplayerCompressionSystemComponent, "{C3099AC9-47A6-41D2-8928-F38F904BAC1B}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // MultiplayerCompressionRequestBus interface implementation
        AZStd::string GetCompressorName() override;
        AzNetworking::CompressorType GetCompressorType() override;
        AZStd::shared_ptr<AzNetworking::ICompressorFactory> GetCompressionFactory() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        AZStd::shared_ptr<MultiplayerCompressionFactory> m_multiplayerCompressionFactory;
    };
}
