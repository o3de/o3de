/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzTest/AzTest.h>
#include <Builder/ScriptCanvasBuilderWorker.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>

#include "AssetBuilderSDK/SerializationDependencies.h"
#include "ScriptCanvas/Asset/RuntimeAsset.h"
#include "Editor/ReflectComponent.h"

class ScriptCanvasFileHandlingTests
    : public UnitTest::AllocatorsFixture
    , public AZ::ComponentApplicationBus::Handler
{
protected:

    //////////////////////////////////////////////////////////////////////////
    // ComponentApplicationMessages
    AZ::ComponentApplication* GetApplication() override { return nullptr; }
    void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
    void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
    void RegisterEntityAddedEventHandler(AZ::EntityAddedEvent::Handler&) override { }
    void RegisterEntityRemovedEventHandler(AZ::EntityRemovedEvent::Handler&) override { }
    void RegisterEntityActivatedEventHandler(AZ::EntityActivatedEvent::Handler&) override { }
    void RegisterEntityDeactivatedEventHandler(AZ::EntityDeactivatedEvent::Handler&) override { }
    void SignalEntityActivated(AZ::Entity*) override { }
    void SignalEntityDeactivated(AZ::Entity*) override { }
    bool AddEntity(AZ::Entity*) override { return true; }
    bool RemoveEntity(AZ::Entity*) override { return true; }
    bool DeleteEntity(const AZ::EntityId&) override { return true; }
    AZ::Entity* FindEntity(const AZ::EntityId&) override { return nullptr; }
    AZ::SerializeContext* GetSerializeContext() override { return m_serializeContext; }
    AZ::BehaviorContext*  GetBehaviorContext() override { return nullptr; }
    AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return &m_jsonContext; }
    const char* GetEngineRoot() const override { return nullptr; }
    const char* GetExecutableFolder() const override { return nullptr; }
    void EnumerateEntities(const AZ::ComponentApplicationRequests::EntityCallback& /*callback*/) override {}
    void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override {}
    //////////////////////////////////////////////////////////////////////////

    void SetUp() override
    {
        UnitTest::AllocatorsFixture::SetUp();
        AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();
        m_serializeContext = aznew AZ::SerializeContext(true, true);
        AZ::ComponentApplicationBus::Handler::BusConnect();
        AZ::Interface<AZ::ComponentApplicationRequests>::Register(this);

        AZ::JsonSystemComponent::Reflect(&m_jsonContext);

        ScriptCanvas::RuntimeData::Reflect(m_serializeContext);
        ScriptCanvas::GraphData::Reflect(m_serializeContext);
        ScriptCanvas::ScriptCanvasData::Reflect(m_serializeContext);
        ScriptCanvas::VariableData::Reflect(m_serializeContext);
        ScriptCanvasEditor::EditorGraphVariableManagerComponent::Reflect(m_serializeContext);
        ScriptCanvasEditor::EditorGraph::Reflect(m_serializeContext);
        ScriptCanvasEditor::ReflectComponent::Reflect(m_serializeContext);
        AZ::Entity::Reflect(m_serializeContext);

        m_jsonSystemComponent = AZStd::make_unique<AZ::JsonSystemComponent>();
        m_jsonSystemComponent->Reflect(&m_jsonContext);

    }

    void TearDown() override
    {
        AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(this);
        AZ::ComponentApplicationBus::Handler::BusDisconnect();

        delete m_serializeContext;
        AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
        UnitTest::AllocatorsFixture::TearDown();
    }
    AZ::SerializeContext* m_serializeContext = nullptr;
    AZ::JsonRegistrationContext m_jsonContext;
    AZStd::unique_ptr<AZ::JsonSystemComponent> m_jsonSystemComponent;
};

TEST_F(ScriptCanvasFileHandlingTests, LoadFromString_ValidJSONString_ReturnsSuccess)
{
    const char* valid_json_script_canvas = R"scriptCanvas(
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "ScriptCanvasData",
    "ClassData": {
        "m_scriptCanvas": {
            "Id": {
                "id": 565217549483
            },
            "Name": "Script Canvas Graph",
            "Components": {
                "Component_[12949128714562389820]": {
                    "$type": "EditorGraphVariableManagerComponent",
                    "Id": 12949128714562389820
                },
                "Component_[5646480141261742117]": {
                    "$type": "EditorGraph",
                    "Id": 5646480141261742117,
                    "m_graphData": {
                        "m_nodes": [
                            {
                                "Id": {
                                    "id": 599577287851
                                },
                                "Name": "SC-Node(Print)",
                                "Components": {
                                    "Component_[7525242489065207166]": {
                                        "$type": "Print",
                                        "Id": 7525242489065207166,
                                        "Slots": [
                                            {
                                                "id": {
                                                    "m_id": "{B2783019-A525-4A10-9F95-C8BE40ADE4C4}"
                                                },
                                                "contracts": [
                                                    {
                                                        "$type": "SlotTypeContract"
                                                    }
                                                ],
                                                "slotName": "In",
                                                "toolTip": "Input signal",
                                                "Descriptor": {
                                                    "ConnectionType": 1,
                                                    "SlotType": 1
                                                }
                                            },
                                            {
                                                "id": {
                                                    "m_id": "{85A82A82-D3C2-475A-B1E1-BA57B1E20B1F}"
                                                },
                                                "DynamicTypeOverride": 3,
                                                "contracts": [
                                                    {
                                                        "$type": "SlotTypeContract"
                                                    }
                                                ],
                                                "slotName": "Value",
                                                "toolTip": "Value which replaces instances of {Value} in the resulting string.",
                                                "DisplayGroup": {
                                                    "Value": 1015031923
                                                },
                                                "Descriptor": {
                                                    "ConnectionType": 1,
                                                    "SlotType": 2
                                                },
                                                "DataType": 1
                                            },
                                            {
                                                "id": {
                                                    "m_id": "{6F3917F9-C4BC-4E7D-9A69-2D1E9A3A34D7}"
                                                },
                                                "contracts": [
                                                    {
                                                        "$type": "SlotTypeContract"
                                                    }
                                                ],
                                                "slotName": "Out",
                                                "Descriptor": {
                                                    "ConnectionType": 2,
                                                    "SlotType": 1
                                                }
                                            }
                                        ],
                                        "Datums": [
                                            {}
                                        ],
                                        "m_arrayBindingMap": [
                                            {
                                                "Key": 1,
                                                "Value": {
                                                    "m_id": "{85A82A82-D3C2-475A-B1E1-BA57B1E20B1F}"
                                                }
                                            }
                                        ],
                                        "m_unresolvedString": [
                                            {},
                                            {}
                                        ],
                                        "m_formatSlotMap": {
                                            "Value": {
                                                "m_id": "{85A82A82-D3C2-475A-B1E1-BA57B1E20B1F}"
                                            }
                                        }
                                    }
                                }
                            }
                        ]
                    },
                    "versionData": {
                        "_grammarVersion": 1,
                        "_runtimeVersion": 1,
                        "_fileVersion": 1
                    },
                    "GraphCanvasData": [
                        {
                            "Key": {
                                "id": 565217549483
                            },
                            "Value": {
                                "ComponentData": {
                                    "{5F84B500-8C45-40D1-8EFC-A5306B241444}": {
                                        "$type": "SceneComponentSaveData"
                                    }
                                }
                            }
                        },
                        {
                            "Key": {
                                "id": 599577287851
                            },
                            "Value": {
                                "ComponentData": {
                                    "{24CB38BB-1705-4EC5-8F63-B574571B4DCD}": {
                                        "$type": "NodeSaveData"
                                    },
                                    "{328FF15C-C302-458F-A43D-E1794DE0904E}": {
                                        "$type": "GeneralNodeTitleComponentSaveData",
                                        "PaletteOverride": "StringNodeTitlePalette"
                                    },
                                    "{7CC444B1-F9B3-41B5-841B-0C4F2179F111}": {
                                        "$type": "GeometrySaveData",
                                        "Position": [
                                            100.0,
                                            100.0
                                        ]
                                    },
                                    "{B0B99C8A-03AF-4CF6-A926-F65C874C3D97}": {
                                        "$type": "StylingComponentSaveData"
                                    },
                                    "{B1F49A35-8408-40DA-B79E-F1E3B64322CE}": {
                                        "$type": "PersistentIdComponentSaveData",
                                        "PersistentId": "{78036B98-AECE-4193-A077-C39153B04C30}"
                                    }
                                }
                            }
                        }
                    ],
                    "StatisticsHelper": {
                        "InstanceCounter": [
                            {
                                "Key": 10684225535275896474,
                                "Value": 1
                            }
                        ]
                    }
                }
            }
        }
    }
}
)scriptCanvas";

    auto result = ScriptCanvasEditor::LoadFromString(valid_json_script_canvas, "", true);
    EXPECT_TRUE(result.IsSuccess());
}

TEST_F(ScriptCanvasFileHandlingTests, LoadFromString_EmptyString_ReturnsFailure)
{
    auto result = ScriptCanvasEditor::LoadFromString("", "", true);
    EXPECT_FALSE(result.IsSuccess());
}

TEST_F(ScriptCanvasFileHandlingTests, LoadFromString_MultipleTimes_makeEntityIdsUnique_EntityIdsAreUnique)
{
    EXPECT_TRUE(false);
}

TEST_F(ScriptCanvasFileHandlingTests, LoadFromString_MultipleTimes_NotMakeEntityIdsUnique_EntityIdsMatchSourceString)
{
    EXPECT_TRUE(false);
}
