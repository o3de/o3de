/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>

#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/RTTI/ReflectContext.h>

#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>

namespace ScriptCanvasTests
{
    using namespace ScriptCanvasEditor;

    class StringArray
    {
    public:
        AZ_TYPE_INFO(StringArray, "{0240E221-3800-4BD3-91F3-0304F097F9A7}");

        StringArray() = default;

        static AZStd::string StringArrayToString(AZStd::vector<AZStd::string> inputArray, AZStd::string_view separator = " ")
        {
            if (inputArray.empty())
            {
                return "";
            }

            AZStd::string value;
            auto currentIt = inputArray.begin();
            auto lastIt = inputArray.end();
            for (value = *currentIt; currentIt != lastIt; ++currentIt)
            {
                value += separator;
                value += *currentIt;
            }

            return value;
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<StringArray>()
                    ->Version(0)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<StringArray>("StringArray")
                    ->Method("StringArrayToString", &StringArray::StringArrayToString)
                    ->Method("Equal", [](const StringArray&, const StringArray&) -> bool {return true; }, { {} })
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                    ;
            }
        }
    };
}

using namespace ScriptCanvasTests;

TEST_F(ScriptCanvasTestFixture, CreateVariableTest)
{
    

    StringArray::Reflect(m_serializeContext);
    StringArray::Reflect(m_behaviorContext);
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    {
        using namespace ScriptCanvas;

        ScriptCanvas::ScriptCanvasId scriptCanvasId = AZ::Entity::MakeId();

        AZStd::unique_ptr<AZ::Entity> propertyEntity = AZStd::make_unique<AZ::Entity>("PropertyGraph");        

        propertyEntity->CreateComponent<GraphVariableManagerComponent>(scriptCanvasId);
        propertyEntity->Init();
        propertyEntity->Activate();

        auto vector3Datum1 = Datum(Data::Vector3Type(1.1f, 2.0f, 3.6f));
        auto vector3Datum2 = Datum(Data::Vector3Type(0.0f, -86.654f, 134.23f));
        auto vector4Datum = Datum(Data::Vector4Type(6.0f, 17.5f, -41.75f, 400.875f));

        TestBehaviorContextObject testObject;
        auto behaviorMatrix4x4Datum = Datum(testObject);

        auto stringArrayDatum = Datum(StringArray());

        AZ::Outcome<VariableId, AZStd::string> addPropertyOutcome(AZ::Failure(AZStd::string("Uninitialized")));
        GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, "FirstVector3", vector3Datum1, false);
        EXPECT_TRUE(addPropertyOutcome);
        EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

        addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
        GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, "SecondVector3", vector3Datum2, false);
        EXPECT_TRUE(addPropertyOutcome);
        EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

        addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
        GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, "FirstVector4", vector4Datum, false);
        EXPECT_TRUE(addPropertyOutcome);
        EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

        addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
        GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, "ProjectionMatrix", behaviorMatrix4x4Datum, false);
        EXPECT_TRUE(addPropertyOutcome);
        EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

        addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
        GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, "My String Array", stringArrayDatum, false);
        EXPECT_TRUE(addPropertyOutcome);
        EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

        AZStd::vector<AZ::Outcome<VariableId, AZStd::string>> addVariablesOutcome;
        AZStd::vector<AZStd::pair<AZStd::string_view, Datum>> datumsToAdd;
        datumsToAdd.emplace_back("FirstBoolean", Datum(true));
        datumsToAdd.emplace_back("FirstString", Datum(AZStd::string("Test")));
        GraphVariableManagerRequestBus::EventResult(addVariablesOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariables<decltype(datumsToAdd)::iterator>, datumsToAdd.begin(), datumsToAdd.end());
        EXPECT_EQ(2, addVariablesOutcome.size());
        EXPECT_TRUE(addVariablesOutcome[0]);
        EXPECT_TRUE(addVariablesOutcome[0].GetValue().IsValid());
        EXPECT_TRUE(addVariablesOutcome[1]);
        EXPECT_TRUE(addVariablesOutcome[1].GetValue().IsValid());

        propertyEntity.reset();
    }

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    StringArray::Reflect(m_serializeContext);
    StringArray::Reflect(m_behaviorContext);
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, AddVariableFailTest)
{
    using namespace ScriptCanvas;

    ScriptCanvasId scriptCanvasId = AZ::Entity::MakeId();

    AZStd::unique_ptr<AZ::Entity> propertyEntity = AZStd::make_unique<AZ::Entity>("PropertyGraph");
    propertyEntity->CreateComponent<GraphVariableManagerComponent>(scriptCanvasId);
    propertyEntity->Init();
    propertyEntity->Activate();

    auto vector3Datum1 = Datum(Data::Vector3Type(1.1f, 2.0f, 3.6f));
    auto vector3Datum2 = Datum(Data::Vector3Type(0.0f, -86.654f, 134.23f));

    const AZStd::string_view propertyName = "SameName";

    AZ::Outcome<VariableId, AZStd::string> addPropertyOutcome(AZ::Failure(AZStd::string("Uninitialized")));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, propertyName, vector3Datum1, false);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

    addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, propertyName, vector3Datum2, false);
    EXPECT_FALSE(addPropertyOutcome);

    propertyEntity.reset();
}

TEST_F(ScriptCanvasTestFixture, RemoveVariableTest)
{
    StringArray::Reflect(m_serializeContext);
    StringArray::Reflect(m_behaviorContext);
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    {
        using namespace ScriptCanvas;

        ScriptCanvasId scriptCanvasId = AZ::Entity::MakeId();

        AZStd::unique_ptr<AZ::Entity> propertyEntity = AZStd::make_unique<AZ::Entity>("PropertyGraph");
        propertyEntity->CreateComponent<GraphVariableManagerComponent>(scriptCanvasId);
        propertyEntity->Init();
        propertyEntity->Activate();

        auto vector3Datum1 = Datum(Data::Vector3Type(1.1f, 2.0f, 3.6f));
        auto vector3Datum2 = Datum(Data::Vector3Type(0.0f, -86.654f, 134.23f));
        auto vector4Datum = Datum(Data::Vector4Type(6.0f, 17.5f, -41.75f, 400.875f));

        TestBehaviorContextObject testObject;
        auto behaviorMatrix4x4Datum = Datum(testObject);

        auto stringArrayDatum = Datum(StringArray());

        size_t numVariablesAdded = 0U;
        AZ::Outcome<VariableId, AZStd::string> addPropertyOutcome(AZ::Failure(AZStd::string("Uninitialized")));
        GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, "FirstVector3", vector3Datum1, false);
        EXPECT_TRUE(addPropertyOutcome);
        EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
        ++numVariablesAdded;

        addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
        GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, "SecondVector3", vector3Datum2, false);
        EXPECT_TRUE(addPropertyOutcome);
        EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
        ++numVariablesAdded;

        addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
        GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, "FirstVector4", vector4Datum, false);
        EXPECT_TRUE(addPropertyOutcome);
        EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
        ++numVariablesAdded;

        addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
        GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, "ProjectionMatrix", behaviorMatrix4x4Datum, false);
        EXPECT_TRUE(addPropertyOutcome);
        EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
        ++numVariablesAdded;

        addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
        GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, "My String Array", stringArrayDatum, false);
        EXPECT_TRUE(addPropertyOutcome);
        EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
        const VariableId stringArrayId = addPropertyOutcome.GetValue();
        ++numVariablesAdded;

        AZStd::vector<AZ::Outcome<VariableId, AZStd::string>> addVariablesOutcome;
        AZStd::vector<AZStd::pair<AZStd::string_view, Datum>> datumsToAdd;
        datumsToAdd.emplace_back("FirstBoolean", Datum(true));
        datumsToAdd.emplace_back("FirstString", Datum(AZStd::string("Test")));
        GraphVariableManagerRequestBus::EventResult(addVariablesOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariables<decltype(datumsToAdd)::iterator>, datumsToAdd.begin(), datumsToAdd.end());
        EXPECT_EQ(2, addVariablesOutcome.size());
        EXPECT_TRUE(addVariablesOutcome[0]);
        EXPECT_TRUE(addVariablesOutcome[0].GetValue().IsValid());
        EXPECT_TRUE(addVariablesOutcome[1]);
        EXPECT_TRUE(addVariablesOutcome[1].GetValue().IsValid());
        numVariablesAdded += addVariablesOutcome.size();

        const AZStd::unordered_map<VariableId, GraphVariable>* properties = nullptr;
        GraphVariableManagerRequestBus::EventResult(properties, scriptCanvasId, &GraphVariableManagerRequests::GetVariables);
        ASSERT_NE(nullptr, properties);
        EXPECT_EQ(numVariablesAdded, (*properties).size());

        {
            // Remove Property By Id
            bool removePropertyResult = false;
            GraphVariableManagerRequestBus::EventResult(removePropertyResult, scriptCanvasId, &GraphVariableManagerRequests::RemoveVariable, stringArrayId);
            EXPECT_TRUE(removePropertyResult);

            properties = {};
            GraphVariableManagerRequestBus::EventResult(properties, scriptCanvasId, &GraphVariableManagerRequests::GetVariables);
            ASSERT_NE(nullptr, properties);
            EXPECT_EQ(numVariablesAdded, (*properties).size() + 1);

            // Attempt to remove already removed property
            GraphVariableManagerRequestBus::EventResult(removePropertyResult, scriptCanvasId, &GraphVariableManagerRequests::RemoveVariable, stringArrayId);
            EXPECT_FALSE(removePropertyResult);
        }

        {
            // Remove Property by name
            size_t numVariablesRemoved = 0U;
            GraphVariableManagerRequestBus::EventResult(numVariablesRemoved, scriptCanvasId, &GraphVariableManagerRequests::RemoveVariableByName, "ProjectionMatrix");
            EXPECT_EQ(1U, numVariablesRemoved);

            properties = {};
            GraphVariableManagerRequestBus::EventResult(properties, scriptCanvasId, &GraphVariableManagerRequests::GetVariables);
            ASSERT_NE(nullptr, properties);
            EXPECT_EQ(numVariablesAdded, (*properties).size() + 2);

            // Attempt to remove property again.
            GraphVariableManagerRequestBus::EventResult(numVariablesRemoved, scriptCanvasId, &GraphVariableManagerRequests::RemoveVariableByName, "ProjectionMatrix");
            EXPECT_EQ(0U, numVariablesRemoved);
        }

        {
            // Re-add removed Property
            addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
            GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, "ProjectionMatrix", behaviorMatrix4x4Datum, false);
            EXPECT_TRUE(addPropertyOutcome);
            EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

            properties = {};
            GraphVariableManagerRequestBus::EventResult(properties, scriptCanvasId, &GraphVariableManagerRequests::GetVariables);
            EXPECT_EQ(numVariablesAdded, (*properties).size() + 1);
        }

        propertyEntity.reset();
    }

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    StringArray::Reflect(m_serializeContext);
    StringArray::Reflect(m_behaviorContext);
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, FindVariableTest)
{
    using namespace ScriptCanvas;

    ScriptCanvasId scriptCanvasId = AZ::Entity::MakeId();

    AZStd::unique_ptr<AZ::Entity> propertyEntity = AZStd::make_unique<AZ::Entity>("PropertyGraph");
    propertyEntity->CreateComponent<GraphVariableManagerComponent>(scriptCanvasId);
    propertyEntity->Init();
    propertyEntity->Activate();

    auto stringVariableDatum = Datum(Data::StringType("SABCDQPE"));

    const AZStd::string_view propertyName = "StringProperty";

    AZ::Outcome<VariableId, AZStd::string> addPropertyOutcome(AZ::Failure(AZStd::string("Uninitialized")));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, propertyName, stringVariableDatum, false);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
    const VariableId stringVariableId = addPropertyOutcome.GetValue();

    GraphVariable* variableByName = nullptr;
    {
        // Find Property by name
        GraphVariableManagerRequestBus::EventResult(variableByName, scriptCanvasId, &GraphVariableManagerRequests::FindVariable, propertyName);
        ASSERT_NE(nullptr, variableByName);
        EXPECT_EQ(variableByName->GetVariableId(), stringVariableId);
        EXPECT_EQ(stringVariableDatum, (*variableByName->GetDatum()));
    }

    GraphVariable* variableById = nullptr;    
    {
        // Find Property by id
        GraphVariableManagerRequestBus::EventResult(variableById, scriptCanvasId, &GraphVariableManagerRequests::FindVariableById, stringVariableId);
        ASSERT_NE(nullptr, variableById);                
        EXPECT_EQ(stringVariableDatum, (*variableById->GetDatum()));
    }

    {
        // Remove Property
        size_t numVariablesRemoved = false;
        GraphVariableManagerRequestBus::EventResult(numVariablesRemoved, scriptCanvasId, &GraphVariableManagerRequests::RemoveVariableByName, propertyName);
        EXPECT_EQ(1U, numVariablesRemoved);
    }

    {
        // Attempt to re-lookup property
        GraphVariable* propertyVariable = nullptr;
        GraphVariableManagerRequestBus::EventResult(propertyVariable, scriptCanvasId, &GraphVariableManagerRequests::FindVariable, propertyName);
        EXPECT_EQ(nullptr, propertyVariable);

        GraphVariable* stringVariable = nullptr;
        GraphVariableManagerRequestBus::EventResult(stringVariable, scriptCanvasId, &GraphVariableManagerRequests::FindVariableById, stringVariableId);
        EXPECT_EQ(nullptr, stringVariable);
    }

    propertyEntity.reset();
}

TEST_F(ScriptCanvasTestFixture, ModifyVariableTest)
{
    using namespace ScriptCanvas;

    ScriptCanvasId scriptCanvasId = AZ::Entity::MakeId();

    AZStd::unique_ptr<AZ::Entity> propertyEntity = AZStd::make_unique<AZ::Entity>("PropertyGraph");
    propertyEntity->CreateComponent<GraphVariableManagerComponent>(scriptCanvasId);
    propertyEntity->Init();
    propertyEntity->Activate();

    auto stringVariableDatum = Datum(Data::StringType("Test1"));

    const AZStd::string_view propertyName = "StringProperty";

    AZ::Outcome<VariableId, AZStd::string> addPropertyOutcome(AZ::Failure(AZStd::string("Uninitialized")));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, propertyName, stringVariableDatum, false);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
    const VariableId stringVariableId = addPropertyOutcome.GetValue();

    GraphVariable* propertyDatum = nullptr;
    GraphVariableManagerRequestBus::EventResult(propertyDatum, scriptCanvasId, &GraphVariableManagerRequests::FindVariable, propertyName);
    ASSERT_NE(nullptr, propertyDatum);

    // Modify the added property
    AZStd::string_view modifiedString = "High Functioning S... *<silenced>";

    {
        ModifiableDatumView datumView;
        propertyDatum->ConfigureDatumView(datumView);

        ASSERT_TRUE(datumView.IsValid());
        ASSERT_TRUE(datumView.GetDataType() == Data::Type::String());

        datumView.SetAs(ScriptCanvas::Data::StringType(modifiedString));
    }

    {
        // Re-lookup Property and test against modifiedString
        GraphVariable* stringVariable = nullptr;
        GraphVariableManagerRequestBus::EventResult(stringVariable, scriptCanvasId, &GraphVariableManagerRequests::FindVariableById, stringVariableId);
        ASSERT_NE(nullptr, stringVariable);

        ModifiableDatumView datumView;
        stringVariable->ConfigureDatumView(datumView);

        ASSERT_TRUE(datumView.IsValid());
        ASSERT_TRUE(datumView.GetDataType() == Data::Type::String());

        auto resultString = datumView.GetAs<Data::StringType>();
        EXPECT_EQ(modifiedString, (*resultString));
    }
}

TEST_F(ScriptCanvasTestFixture, SerializationTest)
{
    StringArray::Reflect(m_serializeContext);
    StringArray::Reflect(m_behaviorContext);

    using namespace ScriptCanvas;

    {

        ScriptCanvasId scriptCanvasId = AZ::Entity::MakeId();

        AZStd::unique_ptr<AZ::Entity> propertyEntity = AZStd::make_unique<AZ::Entity>("PropertyGraph");        
        propertyEntity->CreateComponent<GraphVariableManagerComponent>(scriptCanvasId);
        propertyEntity->Init();
        propertyEntity->Activate();

        auto stringArrayDatum = Datum(StringArray());

        AZ::Outcome<VariableId, AZStd::string> addPropertyOutcome(AZ::Failure(AZStd::string("Uninitialized")));
        GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, "My String Array", stringArrayDatum, false);
        EXPECT_TRUE(addPropertyOutcome);
        EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

        GraphVariable* stringArrayVariable = nullptr;        
        GraphVariableManagerRequestBus::EventResult(stringArrayVariable, scriptCanvasId, &GraphVariableManagerRequests::FindVariable, "My String Array");
        ASSERT_NE(nullptr, stringArrayVariable);
        EXPECT_EQ(stringArrayDatum, (*stringArrayVariable->GetDatum()));

        const VariableId stringArrayVariableId = stringArrayVariable->GetVariableId();

        // Save Property Component Entity
        AZStd::vector<AZ::u8> binaryBuffer;
        AZ::IO::ByteContainerStream<decltype(binaryBuffer)> byteStream(&binaryBuffer);
        const bool objectSaved = AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, propertyEntity.get(), m_serializeContext);
        EXPECT_TRUE(objectSaved);

        // Delete the Property Component 
        propertyEntity.reset();        

        // Load Variable Component Entity
        {
            byteStream.Seek(0U, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            propertyEntity.reset(AZ::Utils::LoadObjectFromStream<AZ::Entity>(byteStream, m_serializeContext));
            ASSERT_TRUE(propertyEntity);
            propertyEntity->Init();
            propertyEntity->Activate();

            GraphVariableManagerComponent* component = propertyEntity->FindComponent<GraphVariableManagerComponent>();

            if (component)
            {
                component->ConfigureScriptCanvasId(scriptCanvasId);
            }
        }

        // Attempt to lookup the My String Array property after loading from object stream
        stringArrayVariable = nullptr;
        GraphVariableManagerRequestBus::EventResult(stringArrayVariable, scriptCanvasId, &GraphVariableManagerRequests::FindVariable, "My String Array");
        ASSERT_NE(nullptr, stringArrayVariable);
        EXPECT_EQ(stringArrayVariableId, stringArrayVariable->GetVariableId());

        auto identityMatrixDatum = Datum(Data::Matrix3x3Type::CreateIdentity());
        addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
        GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, "Super Matrix Bros", identityMatrixDatum, false);
        EXPECT_TRUE(addPropertyOutcome);
        EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

        GraphVariable* matrixVariable = nullptr;
        GraphVariableManagerRequestBus::EventResult(matrixVariable, scriptCanvasId, &GraphVariableManagerRequests::FindVariableById, addPropertyOutcome.GetValue());
        ASSERT_NE(nullptr, matrixVariable);
        
        const Datum* matrix3x3Datum = matrixVariable->GetDatum();
        EXPECT_EQ(identityMatrixDatum, (*matrix3x3Datum));

        propertyEntity.reset();
    }
    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    StringArray::Reflect(m_serializeContext);
    StringArray::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, Vector2AllNodes)
{
    RunUnitTestGraph("LY_SC_UnitTest_Vector2_AllNodes", ExecutionMode::Interpreted);
}

TEST_F(ScriptCanvasTestFixture, Vector3_GetNode)
{
    RunUnitTestGraph("LY_SC_UnitTest_Vector3_Variable_GetNode", ExecutionMode::Interpreted);
}

TEST_F(ScriptCanvasTestFixture, Vector3_SetNode)
{
    RunUnitTestGraph("LY_SC_UnitTest_Vector3_Variable_SetNode", ExecutionMode::Interpreted);
}
