/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Math/Uuid.h>

#include <ScriptEvents/Internal/VersionedProperty.h>
#include <ScriptEvents/ScriptEventDefinition.h>
#include <ScriptEvents/ScriptEvent.h>
#include <ScriptEvents/ScriptEventsAsset.h>

#include <Internal/BehaviorContextBinding/ScriptEventMethod.h>

#include <Tests/ScriptEventsTestFixture.h>
#include <Tests/ScriptEventTestUtilities.h>

namespace ScriptEventsTests
{
    //////////////////////////////////////////////////////////////////////////]
    TEST_F(ScriptEventsTestFixture, ScriptEventRefactor_LuaScriptEventWithId)
    {
        // Tests Script Events that rely on addressable buses.

        const char luaCode[] =
            R"( luaScriptEventWithId = {
                    MethodWithId0 = function(self, param1, param2)
                        ScriptTrace("Handler: " ..  tostring(param1) .. " " .. tostring(param2))

                        ScriptExpectTrue(typeid(param1) == typeid(0), "Type of param1 must be "..tostring(typeid(0)))
                        ScriptExpectTrue(typeid(param2) == typeid(EntityId()), "Type of param2 must be "..tostring(typeid(EntityId())))
                        ScriptExpectTrue(param1 == 1, "The first parameter must be 1")
                        ScriptExpectTrue(param2 == EntityId(12345), "The received entity Id must match the one sent")

                        ScriptTrace("MethodWithId0 handled")

                        return true
                    end,

                    MethodWithId1 = function(self)
                        ScriptTrace("MethodWithId1 handled")
                    end
                }

                local scriptEventDefinition = ScriptEvent("Script_Event", typeid("")) -- Event address is of string type
                local method0 = scriptEventDefinition:AddMethod("MethodWithId0", typeid(false)) -- Return value is Boolean
                method0:AddParameter("Param0", typeid(0))
                method0:AddParameter("Param1", typeid(EntityId()))

                scriptEventDefinition:AddMethod("MethodWithId1") -- No return, no parameters

                scriptEventDefinition:Register()

                ScriptTrace("Need to connect to bus!")
                scriptEventHandler = Script_Event.Connect(luaScriptEventWithId, "ScriptEventAddress")
                ScriptTrace("Should be connected !")

                local returnValue = Script_Event.Event.MethodWithId0("ScriptEventAddress", 1, EntityId(12345))
                ScriptExpectTrue(returnValue, "Method0's return value must be true [ScriptEventRefactor_LuaScriptEventWithId]")

                Script_Event.Event.MethodWithId1("ScriptEventAddress")

            )";

        AZ::ScriptContext script;
        script.BindTo(m_behaviorContext);
        script.Execute(luaCode);
        script.GarbageCollect();
    }

    //////////////////////////////////////////////////////////////////////////]
    TEST_F(ScriptEventsTestFixture, ScriptEventRefactor_LuaScriptEventBroadcast)
    {
        // Tests Script Events that rely on broadcast buses (No address).

        const char luaCode[] =
            R"( luaScriptEventBroadcast = {
                    BroadcastMethod0 = function(self, param1, param2)
                        ScriptTrace("Handler: " ..  tostring(param1) .. " " .. tostring(param2))

                        ScriptExpectTrue(typeid(param1) == typeid(0), "Type of param1 must be "..tostring(typeid(0)))
                        ScriptExpectTrue(typeid(param2) == typeid(EntityId()), "Type of param2 must be "..tostring(typeid(EntityId())))
                        ScriptExpectTrue(param1 == 2, "The first parameter must be 2")
                        ScriptExpectTrue(param2 == EntityId(23456), "The received entity Id must match the one sent")

                        ScriptTrace("BroadcastMethod0 Called")

                        return true
                    end,

                    BroadcastMethod1 = function(self)
                        ScriptTrace("BroadcastMethod1 Called")
                    end
                }

                local scriptEventDefinition = ScriptEvent("Script_Broadcast")
                local method0 = scriptEventDefinition:AddMethod("BroadcastMethod0", typeid(false))
                method0:AddParameter("Param0", typeid(0))
                method0:AddParameter("Param1", typeid(EntityId()))

                scriptEventDefinition:AddMethod("BroadcastMethod1")

                scriptEventDefinition:Register()

                scriptEventHandler = Script_Broadcast.Connect(luaScriptEventBroadcast)

                local returnValue = Script_Broadcast.Broadcast.BroadcastMethod0(2, EntityId(23456))
                ScriptExpectTrue(returnValue, "BroadcastMethod0's return value must be true [ScriptEventRefactor_LuaScriptEventBroadcast]")

                -- Broadcast an event without return or parameters
                Script_Broadcast.Broadcast.BroadcastMethod1()
            )";

        AZ::ScriptContext script;
        script.BindTo(m_behaviorContext);
        script.Execute(luaCode);
        script.GarbageCollect();
    }

    //////////////////////////////////////////////////////////////////////////]
    TEST_F(ScriptEventsTestFixture, ScriptEventRefactor_LuaVersionedProperties)
    {
        // Tests the VersionedProperty's Lua API
        const char luaCode[] =
            R"(
                local versionProperty0 = VersionedProperty("Hello")
                versionProperty0:Set("World")

                ScriptExpectTrue(versionProperty0:Get() == "World", "Version property should match the latest version (i.e. World).")

                local versionedNumberProperty = VersionedProperty(1234)
                versionedNumberProperty:Set(4321)
                versionedNumberProperty:Set(5555)

                ScriptExpectTrue(versionedNumberProperty:Get() == 5555, "Number must match latest version")

                local versionedEntityIDProperty = VersionedProperty(EntityId())
                versionedEntityIDProperty:Set(EntityId(123))
                versionedEntityIDProperty:Set(EntityId(321))

                ScriptExpectTrue(versionedEntityIDProperty:Get() == EntityId(321), "EntityId must match latest version")
            )";

        AZ::ScriptContext script;
        script.BindTo(m_behaviorContext);
        script.Execute(luaCode);
        script.GarbageCollect();

    }

    //////////////////////////////////////////////////////////////////////////]
    class ScriptEventHandlerHook
    {
    public:

        static void OnEventGenericHook(void* userData, const char* eventName, int eventIndex, AZ::BehaviorValueParameter* result, int numParameters, AZ::BehaviorValueParameter* parameters)
        {
            ScriptEventHandlerHook* handler(reinterpret_cast<ScriptEventHandlerHook*>(userData));
            handler->OnEvent(eventName, eventIndex, result, numParameters, parameters);

        }

        // This is the actual handler that will be called if the event is sent or broadcast
        void OnEvent(const char* eventName, const int eventIndex, [[maybe_unused]] AZ::BehaviorValueParameter* result, const int numParameters, AZ::BehaviorValueParameter* parameters)
        {
            EXPECT_EQ(eventIndex, 0);
            EXPECT_EQ(numParameters, 1);

            for (int parameterIndex(0); parameterIndex < numParameters; ++parameterIndex)
            {
                const auto& value = *(parameters + parameterIndex);
                EXPECT_EQ(value.m_typeId, azrtti_typeid<AZ::EntityId>());
            }

            EXPECT_TRUE(true) << "Received Event: " << eventName;
        }
    };

    class AssetEventHandler
        : public AZ::Data::AssetBus::Handler
    {

    public:

        using Callback = AZStd::function<void()>;

        AssetEventHandler(AZ::Data::AssetId assetId, Callback onReady= []() {}, Callback onSaved = []() {})
            : m_assetId(assetId)
            , m_onReadyCallback(onReady)
            , m_onSavedCallback(onSaved)
            , m_ready(0)
            , m_saved(0)
            , m_unloaded(0)
        {
        }

        ~AssetEventHandler()
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
        }


        bool IsDone()
        {
            AZ::Data::AssetBus::ExecuteQueuedEvents();
            return !AZ::Data::AssetBus::Handler::BusIsConnected() || m_ready == 1 || m_saved == 1;
        }

        void OnAssetMoved(AZ::Data::Asset<AZ::Data::AssetData>, void*) override
        {
            EXPECT_TRUE(false);
        }

        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData>) override
        {

            EXPECT_TRUE(false);
        }
        
        void OnAssetUnloaded(const AZ::Data::AssetId, const AZ::Data::AssetType) override
        {
            m_unloaded++;
            AZ::Data::AssetBus::Handler::BusDisconnect();
        }
        
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> assetData) override
        {
            EXPECT_TRUE(false);
            AZ::Data::AssetBus::Handler::BusDisconnect();
        }

        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData>) override
        {
            m_ready++;

            AZ::Data::AssetBus::Handler::BusDisconnect();
            AZ::Data::AssetManager::Instance().DispatchEvents();

            m_onReadyCallback();
        }

        void OnAssetSaved(AZ::Data::Asset<AZ::Data::AssetData> asset, [[maybe_unused]] bool isSuccessful) override
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
            AZ::Data::AssetManager::Instance().DispatchEvents();

            m_saved++;
            m_onSavedCallback();
        }

        AZStd::atomic_int m_saved;
        AZStd::atomic_int m_ready;
        AZStd::atomic_int m_unloaded;

        AZ::Data::AssetId m_assetId;
        Callback m_onReadyCallback;
        Callback m_onSavedCallback;
    };


    void WaitForAssetSystem(AZStd::function<bool()> condition)
    {
        while (!condition())
        {
            AZ::Data::AssetBus::ExecuteQueuedEvents();
            AZ::SystemTickBus::ExecuteQueuedEvents();

            AZ::Data::AssetManager::Instance().DispatchEvents();
            AZStd::this_thread::yield();
        }
    }

    TEST_F(ScriptEventsTestFixture, ScriptEventRefactor_BehaviorContextBinding)
    {
        // Tests the C++ API for Script Event definition, this is meant for testing the core code only
        // Script Events are designed as a data side feature and should not be created using C++

        using namespace AZ;
        using namespace ScriptEvents;
        using namespace ScriptEventData;

        const AZStd::string scriptEventName = "SCRIPTEVENT";

        ScriptEvent definition;
        definition.GetNameProperty().Set(scriptEventName);
        definition.GetAddressTypeProperty().Set(azrtti_typeid<AZ::EntityId>());

        Method& method0 = definition.NewMethod();
        method0.GetNameProperty().Set("Method");

        // Necessary because when working in the Editor, a change to the property will trigger a backup of the property prior to 
        // creating the new version, it's not really intuitive in the context of this test and API, but it's meant as an editor
        // side feature more so than a code feature
        method0.GetNameProperty().OnPropertyChange();
        auto& method1name = method0.GetNameProperty().NewVersion();
        method1name.Set("NewMethodName");

        Parameter& parameter0 = method0.NewParameter();
        parameter0.GetNameProperty().Set("Parameter");
        parameter0.GetTooltipProperty().Set("A simple numeric parameter");
        parameter0.GetTypeProperty().Set(azrtti_typeid<AZ::EntityId>());

        parameter0.GetNameProperty().OnPropertyChange(); // See comment above
        auto& parameterNameV1 = parameter0.GetNameProperty().NewVersion();
        parameterNameV1.Set("RenamedParameter");

        AZ::Uuid assetId = AZ::Uuid("{5B933982-7741-47B4-9060-945A6DFF1D75}");

        // Create an asset out of our Script Event
        const AZ::Data::AssetType type = AZ::AzTypeInfo<ScriptEvents::ScriptEventsAsset>::Uuid();
        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().CreateAsset(assetId, type, AZ::Data::AssetLoadBehavior::Default);
        
        ScriptEvents::ScriptEventsAsset* scriptAsset = assetData.Get();
        scriptAsset->m_definition = definition;

        EXPECT_TRUE(assetData.Save());
        
        AssetEventHandler assetHandler(assetId);
        assetHandler.BusConnect(assetId);

        WaitForAssetSystem([&]() { return assetHandler.IsDone(); });

        assetHandler.BusDisconnect();

        ScriptEvents::Internal::ScriptEventRegistration scriptEventV0;

        assetData = AZ::Data::AssetManager::Instance().FindOrCreateAsset<ScriptEvents::ScriptEventsAsset>(assetId, AZ::Data::AssetLoadBehavior::Default);

        scriptEventV0.CompleteRegistration(assetData);

        WaitForAssetSystem([&]() { return scriptEventV0.IsReady(); });

        // Install the handler
        AZ::BehaviorEBusHandler* handler = nullptr;
        AZ::BehaviorEBus* behaviorEbus = scriptEventV0.GetBehaviorBus();
        EXPECT_TRUE(behaviorEbus->m_createHandler->InvokeResult(handler));

        ScriptEventHandlerHook scriptEventHandler;
        EXPECT_TRUE(handler->InstallGenericHook(method0.GetName().data(), &ScriptEventHandlerHook::OnEventGenericHook, &scriptEventHandler));

        // Randomly chosen address using EntityId as address type
        AZ::BehaviorValueParameter addressParameter;
        AZ::EntityId address = AZ::EntityId(0x12345);
        addressParameter.Set(&address);

        // Connect the handler to an address
        EXPECT_TRUE(handler->Connect(&addressParameter));

        // Now, having defined a ScriptEvent and installed a handler, test sending an Event and Broadcasting
        AZ::BehaviorMethod* behaviorMethod0 = nullptr;
        if (scriptEventV0.GetMethod(method0.GetName(), behaviorMethod0))
        {
            const AZ::BehaviorParameter* address2 = behaviorMethod0->GetArgument(0);
            EXPECT_EQ(address2->m_typeId, azrtti_typeid<AZ::EntityId>());

            const AZ::BehaviorParameter* argument = behaviorMethod0->GetArgument(1);
            if (argument)
            {
                EXPECT_EQ(argument->m_typeId, azrtti_typeid<AZ::EntityId>());
            }

            AZStd::array<AZ::BehaviorValueParameter, 2> params;
            AZ::BehaviorValueParameter* paramFirst(params.begin());
            AZ::BehaviorValueParameter* paramIter = paramFirst;

            // Set the values
            AZ::EntityId value = AZ::EntityId(0x12345);
            paramIter->Set(&value); // Bus address
            ++paramIter;
            AZ::EntityId value2 = AZ::EntityId(0x20000);
            paramIter->Set(&value2); // Some payload (first parameter) can be any entity

            for (size_t argIndex(0), sentinel(behaviorMethod0->GetNumArguments() - 1); argIndex != sentinel; ++argIndex)
            {
                if (const AZ::BehaviorParameter* argument2 = behaviorMethod0->GetArgument(argIndex))
                {
                    const AZStd::string argumentTypeName = argument2->m_typeId.ToString<AZStd::string>();
                    const AZStd::string* argumentNamePtr = behaviorMethod0->GetArgumentName(argIndex);
                    const AZStd::string argName = argumentNamePtr && !argumentNamePtr->empty()
                        ? *argumentNamePtr
                        : (AZStd::string::format("%s:%zu", argumentTypeName.c_str(), argIndex));

                    AZ_TracePrintf("Script Events", "(%d): %s : %s\n", argIndex, argName.c_str(), argumentTypeName.c_str());
                }
            }

            // This is the behavior of sending a Script Event, will be handled by any connected handlers
            EXPECT_TRUE(behaviorMethod0->Call(paramFirst, aznumeric_cast<unsigned int>(params.size())));
        }
        else
        {
            ADD_FAILURE() << "The Script Event for " << scriptEventName.c_str() << " does not exist";
        }

        handler->Disconnect();
        AZ::SystemTickBus::Handler* systemTickBus = &scriptEventV0;
        systemTickBus->BusDisconnect();

        EXPECT_TRUE(behaviorEbus->m_destroyHandler->Invoke(handler));

        AssetEventHandler assetHandler2(assetId, []() {}, []() {});
        assetHandler2.BusConnect(assetId);

        scriptAsset = {};
        assetData = {};

        WaitForAssetSystem([&]() { return assetHandler2.m_unloaded == 1; });

    }

    //////////////////////////////////////////////////////////////////////////]

    TEST_F(ScriptEventsTestFixture, ScriptEventRefactor_SerializationAndVersioning)
    {
        // Tests serialization to file of the Script Event definition format and 
        // associated data types (i.e. VersionedProperty)

        using namespace AZ;
        using namespace ScriptEvents;
        using namespace ScriptEventData;

        ScriptEvent definition;
        AZStd::string scriptEventName = "__SCRIPT_EVENT_NAME__";
        definition.SetVersion(0);
        definition.GetNameProperty().Set(scriptEventName);
        definition.GetTooltipProperty().Set("This is an example script event.");

        Method& method0 = definition.NewMethod();
        method0.GetNameProperty().Set("__METHOD__0__");
        method0.GetTooltipProperty().Set("This is an example method");

        Parameter& parameter0 = method0.NewParameter();
        parameter0.GetNameProperty().Set("__PARAMETER__0__");
        parameter0.GetTooltipProperty().Set("A simple numeric parameter");
        parameter0.GetTypeProperty().Set(azrtti_typeid<int>());

        const char* renamedParameter = "__RENAMED_PARAMETER__0__";
        // Necessary because when working in the Editor, a change to the property will trigger a backup of the property prior to 
        // creating the new version, it's not really intuitive in the context of this test and API, but it's meant as an editor
        // side feature moreso than a code feature
        method0.GetNameProperty().OnPropertyChange();
        VersionedProperty& parameterName = parameter0.GetNameProperty().NewVersion();
        parameterName.Set(renamedParameter);

        const char* renamedMethod = "__METHOD__1__";

        method0.GetNameProperty().OnPropertyChange(); // See comment above
        VersionedProperty& methodName = method0.GetNameProperty().NewVersion();
        methodName.Set(renamedMethod);

        const AZStd::string& latestMethodName = method0.GetName();
        EXPECT_STREQ(renamedMethod, latestMethodName.c_str());

        const AZStd::string& latestParameterdName = parameter0.GetName();
        EXPECT_STREQ(renamedParameter, latestParameterdName.c_str());


        //////////////////////////////////////////////////////////////////////////
        /// Serialize the data
        AZStd::vector<char> xmlBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > xmlStream(&xmlBuffer);
        ObjectStream* xmlObjStream = ObjectStream::Create(&xmlStream, *m_serializeContext, ObjectStream::ST_XML);
        xmlObjStream->WriteClass(&definition);
        xmlObjStream->Finalize();

        AZ::IO::SystemFile tmpOut;
        tmpOut.Open("ScriptEvents_SerializationTest_Full.xml", AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
        tmpOut.Write(xmlStream.GetData()->data(), xmlStream.GetLength());
        tmpOut.Close();

        FlattenVersionedPropertiesInObject(m_serializeContext, &definition);

        xmlBuffer.clear();
        xmlStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        xmlObjStream = ObjectStream::Create(&xmlStream, *m_serializeContext, ObjectStream::ST_XML);
        xmlObjStream->WriteClass(&definition);
        xmlObjStream->Finalize();

        tmpOut.Open("ScriptEvents_SerializationTest_Flat.xml", AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
        tmpOut.Write(xmlStream.GetData()->data(), xmlStream.GetLength());
        tmpOut.Close();

        // Create the asset
        const AZ::Uuid& assetId = AZ::Uuid("{0B4F3716-59D4-4BA6-8982-9C7CCB91C113}");
        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().CreateAsset<ScriptEvents::ScriptEventsAsset>(assetId, AZ::Data::AssetLoadBehavior::Default);
        ScriptEvents::ScriptEventsAsset* scriptAsset = assetData.Get();
        scriptAsset->m_definition = definition;

        EXPECT_TRUE(assetData.Save());

        AssetEventHandler assetHandler(assetId);
        assetHandler.BusConnect(assetId);

        WaitForAssetSystem([&]() { return assetHandler.IsDone(); });

        AZ::IO::FileIOStream outFileStream("ScriptEvents_TestAsset.xml", AZ::IO::OpenMode::ModeWrite);
        if (outFileStream.IsOpen())
        {
            EXPECT_TRUE(AZ::Utils::SaveObjectToStream<ScriptEvents::ScriptEventsAsset>(outFileStream,
                AZ::ObjectStream::ST_XML,
                assetData.Get(),
                m_serializeContext));
        }

        AssetEventHandler assetHandler2(assetId, []() {}, []() {});
        assetHandler2.BusConnect(assetId);

        assetData = {};
        scriptAsset = {};

        WaitForAssetSystem([&]() { return assetHandler2.m_unloaded == 1; });

        auto onSaved = [&assetId]()
        {
            AZStd::string scriptEventName = "__SCRIPT_EVENT_NAME__";
            const char* renamedMethod = "__METHOD__1__";

            // Load the asset
            AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(assetId, AZ::Data::AssetLoadBehavior::Default);

            ScriptEvents::ScriptEventsAsset* loadedScriptAsset = assetData.GetAs<ScriptEvents::ScriptEventsAsset>();

            EXPECT_TRUE(loadedScriptAsset);

            const ScriptEvents::ScriptEvent& loadedDefinition = loadedScriptAsset->m_definition;

            EXPECT_EQ(loadedDefinition.GetVersion(), 0);
            EXPECT_STREQ(loadedDefinition.GetName().data(), scriptEventName.c_str());

            ScriptEvents::Method method;
            bool foundMethod = loadedDefinition.FindMethod(renamedMethod, method);
            EXPECT_TRUE(foundMethod);
            EXPECT_EQ(method.GetNameProperty().GetVersion(), 1);

            AssetEventHandler assetHandler(assetData.GetId());
            assetHandler.BusConnect(assetId);
            assetData = {};
            WaitForAssetSystem([&]() { return assetHandler.m_unloaded == 1; });
            assetHandler.BusDisconnect();

        };

        AssetEventHandler assetHandler3(assetData.GetId(), []() {}, onSaved);
        WaitForAssetSystem([&]() { return assetHandler3.IsDone(); });
        assetHandler3.BusDisconnect();


        auto verifyAsset = AZ::Data::AssetManager::Instance().FindAsset(assetId, AZ::Data::AssetLoadBehavior::Default);
        EXPECT_FALSE(verifyAsset);

    }
}
