/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <Tests/UI/UIFixture.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/PersistentPlugin.h>
#include <EMotionStudio/Plugins/RenderPlugins/Source/OpenGLRender/OpenGLRenderPlugin.h>

namespace EMotionFX
{
    class PersistentTestPlugin
        : public EMStudio::PersistentPlugin
    {
    public:
        AZ_RTTI(PersistentTestPlugin, "{88360562-1A6D-4BA4-82E3-F9DE0D69732E}", EMStudio::PersistentPlugin)

        const char* GetName() const override { return "PersistentTestPlugin"; }

        MOCK_METHOD1(Reflect, void(AZ::ReflectContext*));
        MOCK_METHOD0(GetOptions, EMStudio::PluginOptions*());
        MOCK_METHOD1(Update, void(float));
        MOCK_METHOD1(Render, void(EMotionFX::ActorRenderFlags));
    };

    TEST_F(UIFixture, CreatePersistentPluginTest)
    {
        EMStudio::PluginManager* pluginManager = EMStudio::GetPluginManager();
        const size_t numPreviousPlugins = pluginManager->GetNumPersistentPlugins();

        PersistentTestPlugin* plugin = new PersistentTestPlugin();
        pluginManager->AddPersistentPlugin(plugin);
        EXPECT_EQ(pluginManager->GetNumPersistentPlugins(), numPreviousPlugins + 1)
            << "Failed to add persistent plugin to plugin manager.";
        EXPECT_EQ(pluginManager->GetPersistentPlugins().size(), pluginManager->GetNumPersistentPlugins())
            << "Mismatch between the actual container size and the returned number of plugins.";
    }

    TEST_F(UIFixture, RemovePersistentPluginTest)
    {
        EMStudio::PluginManager* pluginManager = EMStudio::GetPluginManager();
        const size_t numPreviousPlugins = pluginManager->GetNumPersistentPlugins();

        PersistentTestPlugin* plugin = new PersistentTestPlugin();
        pluginManager->AddPersistentPlugin(plugin);
        EXPECT_EQ(pluginManager->GetNumPersistentPlugins(), numPreviousPlugins + 1)
            << "Failed to add persistent plugin to plugin manager.";

        pluginManager->RemovePersistentPlugin(plugin);
        EXPECT_EQ(pluginManager->GetNumPersistentPlugins(), numPreviousPlugins)
            << "Failed to remove persistent plugin to plugin manager.";
    }

    TEST_F(UIFixture, UpdatePersistentPluginsTest)
    {
        PersistentTestPlugin* plugin = new PersistentTestPlugin();
        EMStudio::GetPluginManager()->AddPersistentPlugin(plugin);

        EXPECT_CALL(*plugin, Update(testing::_)).Times(1);
        EMStudio::GetMainWindow()->UpdatePlugins(1.0f / 60.0f);
    }
} // namespace EMotionFX
