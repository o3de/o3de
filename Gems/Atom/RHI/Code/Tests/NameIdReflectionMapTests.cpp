/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <AzFramework/IO/LocalFileIO.h>
#include <Atom/RHI.Edit/Utils.h>
#include <Atom/RHI.Reflect/NameIdReflectionMap.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/Utils.h>

namespace UnitTest
{
    class NamedReflectionTests
        : public RHITestFixture
    {
    protected:

        void SetUp() override
        {
            RHITestFixture::SetUp();
            AZ::IO::FileIOBase::SetInstance(aznew AZ::IO::LocalFileIO());
        }

        void TearDown() override
        {
            delete AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            RHITestFixture::TearDown();
        }
    };

    TEST_F(NamedReflectionTests, NameIdReflectionMap_Empty)
    {
        AZ::RHI::NameIdReflectionMap<AZ::RHI::Handle<>> map;
        EXPECT_EQ(map.Size(), 0);
    }

    TEST_F(NamedReflectionTests, NameIdReflectionMap_Insert)
    {
        AZ::RHI::NameIdReflectionMap<AZ::RHI::Handle<>> map;

        // insert() also sorts the vector

        map.Insert(AZ::Name("name1"), AZ::RHI::Handle<>(3));
        map.Insert(AZ::Name("name2"), AZ::RHI::Handle<>(2));
        map.Insert(AZ::Name("name3"), AZ::RHI::Handle<>(1));

        EXPECT_EQ(map.Size(), 3);
    }

    TEST_F(NamedReflectionTests, NameIdReflectionMap_Serialize)
    {
        AZ::SerializeContext serializeContext;

        AZ::Name::Reflect(&serializeContext);
        AZ::RHI::Handle<>::Reflect(&serializeContext);
        AZ::RHI::NameIdReflectionMap<AZ::RHI::Handle<>>::Reflect(&serializeContext);

        AZ::RHI::NameIdReflectionMap<AZ::RHI::Handle<>> map;
        map.Insert(AZ::Name("name1"), AZ::RHI::Handle<>(3));
        map.Insert(AZ::Name("name2"), AZ::RHI::Handle<>(2));
        map.Insert(AZ::Name("name3"), AZ::RHI::Handle<>(1));

        // XML
        AZStd::vector<char> xmlBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > xmlStream(&xmlBuffer);
        AZ::ObjectStream* xmlObjStream = AZ::ObjectStream::Create(&xmlStream, serializeContext, AZ::ObjectStream::ST_XML);
        xmlObjStream->WriteClass(&map);
        xmlObjStream->Finalize();

        const AZStd::string output(xmlBuffer.data(), xmlBuffer.size());

        EXPECT_NE(output.size(), 0);
    }

    TEST_F(NamedReflectionTests, NameIdReflectionMap_Deserialize)
    {
        const char* serializeDataFormat = R"(<ObjectStream version="3">
            <Class name = "AZ::RHI::NameIdReflectionMap&lt;AZ::RHI::Handle&lt;unsigned int, DefaultNamespaceType&gt;&gt;" type = "{4EAD7B2D-6190-5CB1-898D-5B96EB36EB46}" >
            <Class name = "AZStd::vector" field = "ReflectionMap" type = "{74463005-1C3D-5949-A2FB-90E795144DD6}">
            <Class name = "AZ::RHI::ReflectionNamePair&lt;AZ::RHI::Handle&lt;unsigned int, DefaultNamespaceType&gt;&gt;" field = "element" version = "2" type = "{A9301E84-7228-5301-9B2A-8A096DE3C712}">
            <Class name = "Name" field = "Name" value = "%s" type = "{3D2B920C-9EFD-40D5-AAE0-DF131C3D4931}" />
            <Class name = "AZ::RHI::Handle&lt;unsigned int, DefaultNamespaceType&gt;" field = "Index" version = "1" type = "{1811456D-0C3D-58C8-ACE8-FD47F4E80E25}">
            <Class name = "unsigned int" field = "m_index" value = "%s" type = "{43DA906B-7DEF-4CA8-9790-854106D3F983}" />
            </Class>
            </Class>
            <Class name = "AZ::RHI::ReflectionNamePair&lt;AZ::RHI::Handle&lt;unsigned int, DefaultNamespaceType&gt;&gt;" field = "element" version = "2" type = "{A9301E84-7228-5301-9B2A-8A096DE3C712}">
            <Class name = "Name" field = "Name" value = "%s" type = "{3D2B920C-9EFD-40D5-AAE0-DF131C3D4931}" />
            <Class name = "AZ::RHI::Handle&lt;unsigned int, DefaultNamespaceType&gt;" field = "Index" version = "1" type = "{1811456D-0C3D-58C8-ACE8-FD47F4E80E25}">
            <Class name = "unsigned int" field = "m_index" value = "%s" type = "{43DA906B-7DEF-4CA8-9790-854106D3F983}" />
            </Class>
            </Class>
            <Class name = "AZ::RHI::ReflectionNamePair&lt;AZ::RHI::Handle&lt;unsigned int, DefaultNamespaceType&gt;&gt;" field = "element" version = "2" type = "{A9301E84-7228-5301-9B2A-8A096DE3C712}">
            <Class name = "Name" field = "Name" value = "%s" type = "{3D2B920C-9EFD-40D5-AAE0-DF131C3D4931}" />
            <Class name = "AZ::RHI::Handle&lt;unsigned int, DefaultNamespaceType&gt;" field = "Index" version = "1" type = "{1811456D-0C3D-58C8-ACE8-FD47F4E80E25}">
            <Class name = "unsigned int" field = "m_index" value = "%s" type = "{43DA906B-7DEF-4CA8-9790-854106D3F983}" />
            </Class>
            </Class>
            </Class>
            </Class>
            </ObjectStream>)";

        // The internal storage sorts by the hash value of strings, so name2 comes before name3, which comes before name1.
        // So the inpuit is specifically putting them out order with how it appears sorted when inserted.

        AZStd::string inputData = AZStd::string::format(serializeDataFormat, "name3", "3", "name2", "2", "name1", "1");

        AZ::SerializeContext serializeContext;

        AZ::Name::Reflect(&serializeContext);
        AZ::RHI::Handle<>::Reflect(&serializeContext);
        AZ::RHI::NameIdReflectionMap<AZ::RHI::Handle<>>::Reflect(&serializeContext);

        AZStd::vector<AZ::u8> binaryData(inputData.begin(), inputData.end());

        AZ::IO::ByteContainerStream<const AZStd::vector<AZ::u8> > binaryStream(&binaryData);
        binaryStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        {
            AZ::RHI::NameIdReflectionMap<AZ::RHI::Handle<>> map;

            EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(binaryStream, map, &serializeContext));
            EXPECT_EQ(map.Size(), 3);

            EXPECT_EQ(map.Find(AZ::Name("name1")).m_index, 1);
            EXPECT_EQ(map.Find(AZ::Name("name2")).m_index, 2);
            EXPECT_EQ(map.Find(AZ::Name("name3")).m_index, 3);
        }
    }
}
