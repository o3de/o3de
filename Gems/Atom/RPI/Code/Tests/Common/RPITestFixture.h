/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/RPISystem.h>

#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>

#include <Common/AssetManagerTestFixture.h>
#include <Common/RHI/Stubs.h>
#include <Common/RHI/Factory.h>
#include <Atom/Utils/TestUtils/AssetSystemStub.h>

namespace AZ
{
    class ScriptSystemComponent;
}

namespace UnitTest
{
    //! Unit test fixture for setting up things commonly needed by RPI unit tests
    class RPITestFixture
        : public AssetManagerTestFixture
    {
        static const uint32_t s_heapSizeMB = 64;

    public:
        RPITestFixture();
        ~RPITestFixture();

    protected:
        virtual void Reflect(AZ::ReflectContext* context);

        void SetUp() override;
        void TearDown() override;

        //! Performs processing that would normally be done by the frame scheduler, 
        //! which has to happen in order to recompile the same SRG instance multiple times.
        void ProcessQueuedSrgCompilations(AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset, const AZ::Name& srgName);

        AZ::JsonRegistrationContext* GetJsonRegistrationContext() override;

        AssetSystemStub m_assetSystemStub;

    private:
        AZStd::unique_ptr<StubRHI::Factory> m_rhiFactory;
        AZStd::unique_ptr<AZ::RPI::RPISystem> m_rpiSystem;

        AZStd::unique_ptr<AZ::JobManager> m_jobManager;
        AZStd::unique_ptr<AZ::JobContext> m_jobContext;

        // Required for json serializer
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;

        // Required for Lua
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_scriptSystemComponentDescriptor;

        AZStd::unique_ptr<AZ::Entity> m_systemEntity;

        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZStd::unique_ptr<AZ::IO::FileIOBase> m_localFileIO;
    };
} // namespace UnitTest
