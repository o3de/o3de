/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainWorldComponent.h>
#include <Components/TerrainWorldRendererComponent.h>

#include <Atom/RPI.Public/RPISystem.h>
#include <Common/RHI/Stubs.h>
#include <Common/RHI/Factory.h>
#include <AzFramework/Scene/SceneSystemComponent.h>

#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>
#include <Tests/FileIOBaseTestTypes.h>

#include <AzTest/AzTest.h>

#include <TerrainTestFixtures.h>

class TerrainWorldRendererComponentTest
    : public UnitTest::TerrainTestFixture
{
protected:
    TerrainWorldRendererComponentTest()
        : m_restoreFileIO(m_fileIOMock)
    {
        // Install Mock File IO, since the ShaderMetricsSystem inside of Atom's RPISystem will try to read/write a file.
        AZ::IO::MockFileIOBase::InstallDefaultReturns(m_fileIOMock);
    }

    void SetUp() override
    {
        UnitTest::TerrainTestFixture::SetUp();

        // Create the SceneSystemComponent for use by Atom
        m_sceneSystemEntity = CreateEntity();
        m_sceneSystemEntity->CreateComponent<AzFramework::SceneSystemComponent>();
        ActivateEntity(m_sceneSystemEntity.get());

        // Create a stub RHI for use by Atom
        m_rhiFactory.reset(aznew UnitTest::StubRHI::Factory());

        // Create the Atom RPISystem
        AZ::RPI::RPISystemDescriptor rpiSystemDescriptor;
        m_rpiSystem = AZStd::make_unique<AZ::RPI::RPISystem>();
        m_rpiSystem->Initialize(rpiSystemDescriptor);
    }

    void TearDown() override
    {
        m_rpiSystem->Shutdown();
        m_rpiSystem = nullptr;
        m_rhiFactory = nullptr;

        m_sceneSystemEntity.reset();

        UnitTest::TerrainTestFixture::TearDown();
    }

private:
    AZStd::unique_ptr<UnitTest::StubRHI::Factory> m_rhiFactory;
    AZStd::unique_ptr<AZ::RPI::RPISystem> m_rpiSystem;

    UnitTest::SetRestoreFileIOBaseRAII m_restoreFileIO;
    ::testing::NiceMock<AZ::IO::MockFileIOBase> m_fileIOMock;

    AZStd::unique_ptr<AZ::Entity> m_sceneSystemEntity;
};

TEST_F(TerrainWorldRendererComponentTest, ComponentActivatesSuccessfully)
{
    auto entity = CreateEntity();

    entity->CreateComponent<Terrain::TerrainWorldComponent>();
    entity->CreateComponent<Terrain::TerrainWorldRendererComponent>();

    ActivateEntity(entity.get());
    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);

    entity.reset();
}
