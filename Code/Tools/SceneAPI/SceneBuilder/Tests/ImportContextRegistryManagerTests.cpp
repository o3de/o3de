/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/vector.h>
#include <SceneAPI/SDKWrapper/SceneWrapper.h>
#include <SceneAPI/SceneBuilder/ImportContextRegistryManager.h>
#include <SceneAPI/SceneBuilder/SceneImportRequestHandler.h>

namespace AZ::SceneAPI::SceneBuilder
{
    namespace
    {
        struct TestImportContextProvider : public ImportContextProvider
        {
            TestImportContextProvider(AZStd::string name, AZStd::vector<AZStd::string> extensions, int* destroyedCounter = nullptr)
                : m_name(AZStd::move(name))
                , m_extensions(AZStd::move(extensions))
                , m_destroyedCounter(destroyedCounter)
            {
            }

            ~TestImportContextProvider() override
            {
                if (m_destroyedCounter)
                {
                    ++(*m_destroyedCounter);
                }
            }

            AZStd::shared_ptr<NodeEncounteredContext> CreateNodeEncounteredContext(
                Containers::Scene&, Containers::SceneGraph::NodeIndex, const SceneSystem&, RenamedNodesMap&,
                SDKScene::SceneWrapperBase&, SDKNode::NodeWrapper&) override { return {}; }
            AZStd::shared_ptr<SceneDataPopulatedContextBase> CreateSceneDataPopulatedContext(
                NodeEncounteredContext&, AZStd::shared_ptr<DataTypes::IGraphObject>, const AZStd::string&) override { return {}; }
            AZStd::shared_ptr<SceneNodeAppendedContextBase> CreateSceneNodeAppendedContext(
                SceneDataPopulatedContextBase&, Containers::SceneGraph::NodeIndex) override { return {}; }
            AZStd::shared_ptr<SceneAttributeDataPopulatedContextBase> CreateSceneAttributeDataPopulatedContext(
                SceneNodeAppendedContextBase&, AZStd::shared_ptr<DataTypes::IGraphObject>, const Containers::SceneGraph::NodeIndex,
                const AZStd::string&) override { return {}; }
            AZStd::shared_ptr<SceneAttributeNodeAppendedContextBase> CreateSceneAttributeNodeAppendedContext(
                SceneAttributeDataPopulatedContextBase&, Containers::SceneGraph::NodeIndex) override { return {}; }
            AZStd::shared_ptr<SceneNodeAddedAttributesContextBase> CreateSceneNodeAddedAttributesContext(
                SceneNodeAppendedContextBase&) override { return {}; }
            AZStd::shared_ptr<SceneNodeFinalizeContextBase> CreateSceneNodeFinalizeContext(
                SceneNodeAddedAttributesContextBase&) override { return {}; }
            AZStd::shared_ptr<FinalizeSceneContextBase> CreateFinalizeSceneContext(
                Containers::Scene&, const SceneSystem&, SDKScene::SceneWrapperBase&, RenamedNodesMap&) override { return {}; }
            AZStd::unique_ptr<SDKScene::SceneWrapperBase> CreateSceneWrapper() const override { return {}; }

            bool CanHandleExtension(AZStd::string_view fileExtension) const override
            {
                return AZStd::find(m_extensions.begin(), m_extensions.end(), fileExtension) != m_extensions.end();
            }

            void GetSupportedFileExtensions(AZStd::unordered_set<AZStd::string>& extensions) const override
            {
                extensions.insert(m_extensions.begin(), m_extensions.end());
            }

            AZStd::string GetImporterName() const override { return m_name; }

            AZStd::string m_name;
            AZStd::vector<AZStd::string> m_extensions;
            int* m_destroyedCounter = nullptr;
        };
    } // namespace

    TEST(ImportContextRegistryManagerTests, SelectImportProvider_NoProviders_ReturnsNull)
    {
        ImportContextRegistryManager registry;
        EXPECT_EQ(nullptr, registry.SelectImportProvider(".fbx"));

        AZStd::unordered_set<AZStd::string> extensions;
        registry.GetSupportedFileExtensions(extensions);
        EXPECT_TRUE(extensions.empty());
    }

    TEST(ImportContextRegistryManagerTests, GetSupportedFileExtensions_MultipleProviders_ReturnsUnion)
    {
        ImportContextRegistryManager registry;
        registry.RegisterContextProvider(new TestImportContextProvider("Fallback", { ".fbx", ".obj", ".gltf" }));
        registry.RegisterContextProvider(new TestImportContextProvider("Native", { ".gltf", ".glb" }));

        AZStd::unordered_set<AZStd::string> extensions;
        registry.GetSupportedFileExtensions(extensions);

        EXPECT_EQ(4, extensions.size());
        EXPECT_TRUE(extensions.contains(".fbx"));
        EXPECT_TRUE(extensions.contains(".obj"));
        EXPECT_TRUE(extensions.contains(".gltf"));
        EXPECT_TRUE(extensions.contains(".glb"));
    }

    TEST(ImportContextRegistryManagerTests, SelectImportProvider_LaterProviderClaimsExtension_OverridesEarlierProvider)
    {
        ImportContextRegistryManager registry;
        auto* fallback = new TestImportContextProvider("Fallback", { ".fbx", ".gltf" });
        auto* native = new TestImportContextProvider("Native", { ".gltf" });
        registry.RegisterContextProvider(fallback);
        registry.RegisterContextProvider(native);

        EXPECT_EQ(native, registry.SelectImportProvider(".gltf"));
        EXPECT_EQ(fallback, registry.SelectImportProvider(".fbx"));
    }

    TEST(ImportContextRegistryManagerTests, SelectImportProvider_NoProviderHandlesExtension_ReturnsNull)
    {
        ImportContextRegistryManager registry;
        registry.RegisterContextProvider(new TestImportContextProvider("Fallback", { ".fbx" }));

        EXPECT_EQ(nullptr, registry.SelectImportProvider(".xyz"));
    }

    TEST(ImportContextRegistryManagerTests, UnregisterContextProvider_RemovesAndDestroysProvider_SelectionFallsBack)
    {
        int destroyed = 0;
        ImportContextRegistryManager registry;
        auto* fallback = new TestImportContextProvider("Fallback", { ".gltf" }, &destroyed);
        auto* native = new TestImportContextProvider("Native", { ".gltf", ".glb" }, &destroyed);
        registry.RegisterContextProvider(fallback);
        registry.RegisterContextProvider(native);

        registry.UnregisterContextProvider(native);

        EXPECT_EQ(1, destroyed);
        EXPECT_EQ(fallback, registry.SelectImportProvider(".gltf"));
        EXPECT_EQ(nullptr, registry.SelectImportProvider(".glb"));

        AZStd::unordered_set<AZStd::string> extensions;
        registry.GetSupportedFileExtensions(extensions);
        EXPECT_EQ(1, extensions.size());
        EXPECT_TRUE(extensions.contains(".gltf"));
    }

    TEST(ImportContextRegistryManagerTests, RegisterAndUnregister_NullOrUnknownProvider_IsIgnored)
    {
        int destroyed = 0;
        ImportContextRegistryManager registry;
        registry.RegisterContextProvider(nullptr);
        registry.UnregisterContextProvider(nullptr);

        TestImportContextProvider unregistered("Unregistered", { ".fbx" }, &destroyed);
        registry.UnregisterContextProvider(&unregistered);

        EXPECT_EQ(0, destroyed);
        EXPECT_EQ(nullptr, registry.SelectImportProvider(".fbx"));
    }

    TEST(ImportContextRegistryManagerTests, Destructor_DestroysRemainingProvidersAndUnregistersInterface)
    {
        int destroyed = 0;
        {
            ImportContextRegistryManager registry;
            EXPECT_EQ(&registry, ImportContextRegistryInterface::Get());
            registry.RegisterContextProvider(new TestImportContextProvider("A", { ".fbx" }, &destroyed));
            registry.RegisterContextProvider(new TestImportContextProvider("B", { ".obj" }, &destroyed));
        }
        EXPECT_EQ(2, destroyed);
        EXPECT_EQ(nullptr, ImportContextRegistryInterface::Get());
    }

    TEST(ImportContextRegistryManagerTests, SceneImportRequestHandler_GetSupportedFileExtensions_NormalizesProviderExtensions)
    {
        ImportContextRegistryManager registry;
        registry.RegisterContextProvider(new TestImportContextProvider("Mixed", { ".FBX", "obj", "", "." }));

        SceneImportRequestHandler handler;
        AZStd::unordered_set<AZStd::string> extensions;
        handler.GetSupportedFileExtensions(extensions);

        EXPECT_EQ(2, extensions.size());
        EXPECT_TRUE(extensions.contains(".fbx"));
        EXPECT_TRUE(extensions.contains(".obj"));
    }
} // namespace AZ::SceneAPI::SceneBuilder
