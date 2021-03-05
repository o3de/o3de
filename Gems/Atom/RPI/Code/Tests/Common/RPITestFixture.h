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

#include <Atom/RPI.Public/RPISystem.h>

#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>

#include <Common/AssetManagerTestFixture.h>
#include <Common/RHI/Stubs.h>
#include <Common/RHI/Factory.h>
#include <Common/AssetSystemStub.h>

namespace UnitTest
{
    //! Unit test fixture for setting up things commonly needed by RPI unit tests
    class RPITestFixture
        : public AssetManagerTestFixture
    {
        static const uint32_t s_heapSizeMB = 64;

    public:

    protected:
        virtual void Reflect(AZ::ReflectContext* context);

        void SetUp() override;
        void TearDown() override;

        AZ::RHI::Device* GetDevice();

        //! Performs processing that would normally be done by the frame scheduler, 
        //! which has to happen in order to recompile the same SRG instance multiple times.
        void ProcessQueuedSrgCompilations(AZ::Data::Asset<AZ::RPI::ShaderResourceGroupAsset> srgAsset);

        AZ::JsonRegistrationContext* GetJsonRegistrationContext() override;

        AssetSystemStub m_assetSystemStub;

    private:
        AZStd::unique_ptr<StubRHI::Factory> m_rhiFactory;
        AZStd::unique_ptr<AZ::RPI::RPISystem> m_rpiSystem;

        AZStd::unique_ptr<AZ::JobManager> m_jobManager;
        AZStd::unique_ptr<AZ::JobContext> m_jobContext;

        // Required for json serializer
        AZStd::unique_ptr<AZ::JsonSystemComponent> m_jsonSystemComponent;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;

        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZStd::unique_ptr<AZ::IO::FileIOBase> m_localFileIO;
    };
} // namespace UnitTest
