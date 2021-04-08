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

#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/ToolsComponents/ScriptEditorComponent.h>

#include "EntityTestbed.h"

namespace UnitTest
{
    using namespace AZ;
    using namespace AzFramework;

    class EntityScriptTest
        : public EntityTestbed
    {
    public:

        ScriptContext* m_scriptContext;

        ~EntityScriptTest()
        {
        }

        void OnDestroy() override
        {
            delete m_scriptContext;
            m_scriptContext = nullptr;
        }

        void run()
        {
            int argc = 0;
            char* argv = nullptr;
            Run(argc, &argv);
        }

        void OnReflect(AZ::SerializeContext& context, AZ::Entity& systemEntity) override
        {
            (void)context;
            (void)systemEntity;
        }

        void OnSetup() override
        {
            m_scriptContext = aznew AZ::ScriptContext();

            auto* catalogBus = AZ::Data::AssetCatalogRequestBus::FindFirstHandler();
            if (catalogBus)
            {
                // Register asset types the asset DB should query our catalog for.
                catalogBus->AddAssetType(AZ::AzTypeInfo<AZ::ScriptAsset>::Uuid());

                // Build the catalog (scan).
                catalogBus->AddExtension(".lua");
            }
        }

        void OnEntityAdded(AZ::Entity& entity) override
        {
            // Add your components.
            entity.CreateComponent<AzToolsFramework::Components::ScriptEditorComponent>();
            entity.Activate();
        }
    };

    TEST_F(EntityScriptTest, DISABLED_Test)
    {
        run();
    }
} // namespace UnitTest
