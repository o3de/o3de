/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/Utils.h>
#include <AzFramework/Application/Application.h>

namespace UnitTest
{
    class GenAppDescriptors
        : public AllocatorsTestFixture
    {
    public:
        void FakePopulateModules(AZ::ComponentApplication::Descriptor& desc, const char* libSuffix)
        {
            static const char* modules[] =
            {
                "LySystemModule",
            };

            if (desc.m_modules.empty())
            {
                for (const char* module : modules)
                {
                    desc.m_modules.push_back();
                    desc.m_modules.back().m_dynamicLibraryPath = module;
                    desc.m_modules.back().m_dynamicLibraryPath += libSuffix;
                }
            }
        }
    };

    TEST_F(GenAppDescriptors, WriteDescriptor_ToXML_Succeeds)
    {
        struct Config
        {
            const char* platformName;
            const char* configName;
            const char* libSuffix;
        };

        AzFramework::Application app;

        AZ::SerializeContext serializeContext;
        AZ::ComponentApplication::Descriptor::Reflect(&serializeContext, &app);
        AZ::Entity::Reflect(&serializeContext);
        AZ::DynamicModuleDescriptor::Reflect(&serializeContext);

        AZ::Entity dummySystemEntity(AZ::SystemEntityId, "SystemEntity");

        const Config config = {"Platform", "Config", "libSuffix"};

        AZ::ComponentApplication::Descriptor descriptor;

        if (config.libSuffix && config.libSuffix[0])
        {
            FakePopulateModules(descriptor, config.libSuffix);
        }

        AZ::Test::ScopedAutoTempDirectory tempDirectory;
        const auto filename = AZ::IO::Path(tempDirectory.GetDirectory()) /
            AZStd::string::format("LYConfig_%s%s.xml", config.platformName, config.configName);

        AZ::IO::FileIOStream stream(filename.c_str(), AZ::IO::OpenMode::ModeWrite);
        auto objStream = AZ::ObjectStream::Create(&stream, serializeContext, AZ::ObjectStream::ST_XML);
        const bool descWriteOk = objStream->WriteClass(&descriptor);
        EXPECT_TRUE(descWriteOk) << "Failed to write memory descriptor to application descriptor file " << filename.c_str() << "!";
        const bool entityWriteOk = objStream->WriteClass(&dummySystemEntity);
        EXPECT_TRUE(entityWriteOk) << "Failed to write system entity to application descriptor file " << filename.c_str() << "!";
        const bool flushOk = objStream->Finalize();
        EXPECT_TRUE(flushOk) << "Failed finalizing application descriptor file " << filename.c_str() << "!";
    }
}
