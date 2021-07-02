/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Component/ComponentApplication.h>

namespace UnitTest
{
    using namespace AZ;

    class SetRestoreFileIOBaseRAII
    {
    public:
        SetRestoreFileIOBaseRAII(AZ::IO::FileIOBase& fileIO)
            : m_prevFileIO(AZ::IO::FileIOBase::GetInstance())
        {
            AZ::IO::FileIOBase::SetInstance(&fileIO);
        }

        ~SetRestoreFileIOBaseRAII()
        {
            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
        }
    private:
        AZ::IO::FileIOBase* m_prevFileIO;
    };

    class GenAppDescriptors
        : public AllocatorsTestFixture
    {
    public:

        void run()
        {
            struct Config
            {
                const char* platformName;
                const char* configName;
                const char* libSuffix;
            };

            ComponentApplication app;

            SerializeContext serializeContext;
            AZ::ComponentApplication::Descriptor::Reflect(&serializeContext, &app);
            AZ::Entity::Reflect(&serializeContext);
            DynamicModuleDescriptor::Reflect(&serializeContext);

            AZ::Entity dummySystemEntity(AZ::SystemEntityId, "SystemEntity");

            const Config config = {"Platform", "Config", "libSuffix"};

            AZ::ComponentApplication::Descriptor descriptor;

            if (config.libSuffix && config.libSuffix[0])
            {
                FakePopulateModules(descriptor, config.libSuffix);
            }

            const AZStd::string filename = AZStd::string::format("LYConfig_%s%s.xml", config.platformName, config.configName);

            IO::FileIOStream stream(filename.c_str(), IO::OpenMode::ModeWrite);
            ObjectStream* objStream = ObjectStream::Create(&stream, serializeContext, ObjectStream::ST_XML);
            bool descWriteOk = objStream->WriteClass(&descriptor);
            (void)descWriteOk;
            AZ_Warning("ComponentApplication", descWriteOk, "Failed to write memory descriptor to application descriptor file %s!", filename.c_str());
            bool entityWriteOk = objStream->WriteClass(&dummySystemEntity);
            (void)entityWriteOk;
            AZ_Warning("ComponentApplication", entityWriteOk, "Failed to write system entity to application descriptor file %s!", filename.c_str());
            bool flushOk = objStream->Finalize();
            (void)flushOk;
            AZ_Warning("ComponentApplication", flushOk, "Failed finalizing application descriptor file %s!", filename.c_str());

        }

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

    TEST_F(GenAppDescriptors, Test)
    {
        AZ::IO::LocalFileIO fileIO;
        SetRestoreFileIOBaseRAII restoreFileIOScope(fileIO);
        run();
    }
}
