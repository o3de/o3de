/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "TestTypes.h"

#include <iostream>

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/lua/lua.h>
#include <AzCore/Script/ScriptContextDebug.h>

// Script Properties
#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/Script/ScriptPropertyTable.h>

#if !defined(AZCORE_EXCLUDE_LUA)

namespace UnitTest
{
    using namespace AZ;

    // ScriptPropertyTests
    class ScriptPropertyTest
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            // Need to initialize our variables here, instead of in the constructor to avoid some memory tracking issues.
            m_tableName = "tableName";

            m_nilName = "nilValue";            

            m_stringName = "stringValue";
            m_stringValue = "This is the value of the string value";
            
            m_numberName = "numberValue";
            m_numberValue = 1.5;
            
            m_booleanName = "booleanValue";
            m_booleanValue = false;
            
            m_numberArrayName = "numberArray";            
            m_numberArray.push_back(1.9);
            m_numberArray.push_back(2.8);
            m_numberArray.push_back(3.7);
            m_numberArray.push_back(4.6);
            m_numberArray.push_back(5.5);                       
            
            m_hashName = "hashArray";
            m_hashValues["first"] = 1.9;
            m_hashValues["foo"] = 2.8;
            m_hashValues["bar"] = 3.7;
            m_hashValues["baz"] = 4.6;
            m_hashValues["last"] = 5.5;

            m_behavior = aznew BehaviorContext();            

            m_script = aznew ScriptContext();
            m_script->BindTo(m_behavior);            

            // Create a table so we can inspect it using the DataContexts

            CreateTable();
            WriteTableValue(m_numberName,m_numberValue);
            WriteTableValue(m_stringName,m_stringValue.c_str());
            WriteTableValue(m_booleanName, m_booleanValue);

            WriteTableArray(m_numberArrayName, m_numberArray);
            WriteTable(m_hashName,m_hashValues);

            m_script->InspectTable(m_tableName.c_str(),m_dataContext);            
        }

        void TearDown() override
        {
            m_dataContext = ScriptDataContext();

            delete m_script;
            m_script = nullptr;

            delete m_behavior;
            m_behavior = nullptr;            

            LeakDetectionFixture::TearDown();
        }

        void CreateTable()
        {
            std::stringstream inputStream;

            inputStream << m_tableName.c_str();
            inputStream << " = { }";

            m_script->Execute(inputStream.str().c_str());
        }

        template<typename T>
        void WriteTableValue(AZStd::string& name, T value)
        {
            std::stringstream inputStream;

            inputStream << m_tableName.c_str();
            inputStream << ".";
            inputStream << name.c_str();
            inputStream << " = ";
            inputStream << value;

            m_script->Execute(inputStream.str().c_str());
        }

        template<>
        void WriteTableValue<bool>(AZStd::string& name, bool value)
        {   
            std::stringstream inputStream;

            inputStream << m_tableName.c_str();
            inputStream << ".";
            inputStream << name.c_str();            
            inputStream << " = ";

            if (value)
            {
                inputStream << "true";
            }
            else
            {
                inputStream << "false";
            }

            m_script->Execute(inputStream.str().c_str());
        }

        template<>
        void WriteTableValue<const char*>(AZStd::string& name, const char* value)
        {   
            std::stringstream inputStream;

            inputStream << m_tableName.c_str();
            inputStream << ".";
            inputStream << name.c_str();            
            inputStream << " = ";
            inputStream << "\"";
            inputStream << value;
            inputStream << "\"";

            m_script->Execute(inputStream.str().c_str());
        }

        template<typename T>
        void WriteTableArray(AZStd::string& name, AZStd::vector<T>& valueArray)
        {
            std::stringstream inputStream;

            inputStream << m_tableName.c_str();
            inputStream << ".";
            inputStream << name.c_str();            
            inputStream << " = {";            

            for (unsigned int i=0; i < valueArray.size(); ++i)
            {
                if (i != 0)
                {
                    inputStream << ",";
                }

                inputStream << valueArray[i];
            }

            inputStream << "}";

            m_script->Execute(inputStream.str().c_str());
        }
        
        template<typename T>
        void WriteTable(AZStd::string& name, AZStd::unordered_map<AZStd::string, T>& valueMap)
        {
            std::stringstream inputStream;
            
            inputStream << m_tableName.c_str();
            inputStream << ".";
            inputStream << name.c_str();
            inputStream << " = {";

            bool skipOnce = true;
            
            for (auto& mapPair : valueMap)
            {
                if (!skipOnce)
                {
                    inputStream << " ,";
                }
                skipOnce = false;
                
                inputStream << mapPair.first.c_str();
                inputStream << " = ";
                inputStream << mapPair.second;                
            }
            
            inputStream << "}";

            m_script->Execute(inputStream.str().c_str());
        }
        
        // Overall table name
        AZStd::string m_tableName;

        // Nil
        AZStd::string m_nilName;

        // String
        AZStd::string m_stringName;
        AZStd::string m_stringValue;

        // number
        AZStd::string m_numberName;
        double m_numberValue = 1.5;

        // boolean
        AZStd::string m_booleanName;
        bool m_booleanValue = false;

        // Number Array
        AZStd::string m_numberArrayName;
        AZStd::vector<double> m_numberArray;
        
        // Table
        AZStd::string m_hashName;
        AZStd::unordered_map<AZStd::string, double> m_hashValues;

        BehaviorContext* m_behavior = nullptr;
        ScriptContext* m_script = nullptr;

        ScriptDataContext m_dataContext;
    };

    TEST_F(ScriptPropertyTest, ScriptPropertyTest_ParseTest_PrimitiveTypes)
    {        
        ScriptProperty* scriptProperty = m_dataContext.ConstructTableScriptProperty(m_numberName.c_str());

        ASSERT_NE(scriptProperty, nullptr);
        EXPECT_TRUE(azrtti_istypeof<ScriptPropertyNumber>(scriptProperty));        

        if (azrtti_istypeof<ScriptPropertyNumber>(scriptProperty))
        {
            bool isClose = IsClose(m_numberValue, static_cast<ScriptPropertyNumber*>(scriptProperty)->m_value, 0.001);
            EXPECT_EQ(isClose, true);
        }

        delete scriptProperty;
        scriptProperty = nullptr;
        
        scriptProperty = m_dataContext.ConstructTableScriptProperty(m_stringName.c_str());

        ASSERT_NE(scriptProperty, nullptr);
        EXPECT_TRUE(azrtti_istypeof<ScriptPropertyString>(scriptProperty));
        
        if (azrtti_istypeof<ScriptPropertyString>(scriptProperty))
        {
            EXPECT_EQ(m_stringValue.compare(static_cast<ScriptPropertyString*>(scriptProperty)->m_value.c_str()), 0);
        }

        delete scriptProperty;
        scriptProperty = nullptr;        
        
        scriptProperty = m_dataContext.ConstructTableScriptProperty(m_booleanName.c_str());

        ASSERT_NE(scriptProperty, nullptr);
        EXPECT_TRUE(azrtti_istypeof<ScriptPropertyBoolean>(scriptProperty));        

        if (azrtti_istypeof<ScriptPropertyBoolean>(scriptProperty))        
        {
            EXPECT_EQ(m_booleanValue, static_cast<ScriptPropertyBoolean*>(scriptProperty)->m_value);
        }

        delete scriptProperty;
        scriptProperty = nullptr;
        
        scriptProperty = m_dataContext.ConstructTableScriptProperty(m_nilName.c_str());

        EXPECT_NE(scriptProperty,nullptr);
        EXPECT_TRUE(azrtti_istypeof<ScriptPropertyNil>(scriptProperty));

        delete scriptProperty;
        scriptProperty = nullptr;
    }
    
    TEST_F(ScriptPropertyTest, ScriptPropertyTest_ParseTest_HashMapAsTable)
    {
        const bool k_restrictToPropertyArrays = false;

        ScriptProperty* scriptProperty = m_dataContext.ConstructTableScriptProperty(m_hashName.c_str(), k_restrictToPropertyArrays);

        ASSERT_NE(scriptProperty, nullptr);
        EXPECT_TRUE(azrtti_istypeof<ScriptPropertyTable>(scriptProperty));

        if (azrtti_istypeof<ScriptPropertyTable>(scriptProperty))
        {
            ScriptPropertyNumber scriptPropertyNumber;            
            ScriptPropertyTable* scriptTable = static_cast<ScriptPropertyTable*>(scriptProperty);
            
            for (auto& mapPair : m_hashValues)
            {
                const AZStd::string& keyString = mapPair.first;
                
                ScriptProperty* mapProperty = scriptTable->FindTableValue(keyString);
                
                ASSERT_NE(mapProperty,nullptr);
                EXPECT_TRUE(azrtti_istypeof<ScriptPropertyNumber>(mapProperty));
                
                if (azrtti_istypeof<ScriptPropertyNumber>(mapProperty))
                {
                    EXPECT_EQ(IsClose(mapPair.second,static_cast<ScriptPropertyNumber*>(mapProperty)->m_value,0.001),true);
                }
            }
        }

        delete scriptProperty;
        scriptProperty = nullptr;
    }
    

    TEST_F(ScriptPropertyTest, ScriptPropertyTest_ParseTest_PropertyArrayAsTable)
    {
        const bool k_restrictToPropertyArrays = false;

        ScriptProperty* scriptProperty = m_dataContext.ConstructTableScriptProperty(m_numberArrayName.c_str(), k_restrictToPropertyArrays);

        ASSERT_NE(scriptProperty, nullptr);
        EXPECT_TRUE(azrtti_istypeof<ScriptPropertyTable>(scriptProperty));

        if (azrtti_istypeof<ScriptPropertyTable>(scriptProperty))
        {
            ScriptPropertyNumber scriptPropertyNumber;
            ScriptPropertyTable* scriptTable = static_cast<ScriptPropertyTable*>(scriptProperty);

            for (unsigned int i=0; i < m_numberArray.size(); ++i)
            {
                // Offset to deal with lua 1 indexing things.                
                ScriptProperty* mapProperty = scriptTable->FindTableValue((i+ 1));

                ASSERT_NE(mapProperty, nullptr);
                EXPECT_TRUE(azrtti_istypeof<ScriptPropertyNumber>(mapProperty));                

                if (azrtti_typeid(mapProperty) == ScriptPropertyNumber::RTTI_Type())
                {
                    EXPECT_EQ(IsClose(m_numberArray[i],static_cast<ScriptPropertyNumber*>(mapProperty)->m_value,0.001),true);
                }
            }
        }

        delete scriptProperty;
        scriptProperty = nullptr;
    }

    TEST_F(ScriptPropertyTest, ScriptPropertyTest_ParseTest_PropertyArray)
    {
        const bool k_restrictToPropertyArrays = true;

        ScriptProperty* scriptProperty = m_dataContext.ConstructTableScriptProperty(m_numberArrayName.c_str(),k_restrictToPropertyArrays);

        ASSERT_NE(scriptProperty, nullptr);
        EXPECT_TRUE(azrtti_istypeof<ScriptPropertyNumberArray>(scriptProperty));

        if (azrtti_istypeof<ScriptPropertyNumberArray>(scriptProperty))
        {
            AZStd::vector<double>& valueArray = static_cast<ScriptPropertyNumberArray*>(scriptProperty)->m_values;

            ASSERT_EQ(valueArray.size(), m_numberArray.size());

            for (unsigned int i=0; i < valueArray.size(); ++i)
            {
                EXPECT_EQ(true, IsClose(valueArray[i],m_numberArray[i],0.001));
            }
        }
    }
}
#endif // #if !defined(AZCORE_EXCLUDE_LUA)
