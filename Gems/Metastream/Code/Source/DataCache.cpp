/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Metastream_precompiled.h"
#include "DataCache.h"
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace Metastream
{

    void DataCache::AddToCache(const std::string& tableName, const std::string& key, const char* value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->Add(key, jsonValue);
    }

    void DataCache::AddToCache(const std::string& tableName, const std::string& key, bool value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->Add(key, jsonValue);
    }

    void DataCache::AddToCache(const std::string& tableName, const std::string& key, const AZ::Vector3& value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->Add(key, jsonValue);
    }

    void DataCache::AddToCache(const std::string& tableName, const std::string& key, double value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->Add(key, jsonValue);
    }

    void DataCache::AddToCache(const std::string& tableName, const std::string& key, AZ::u64 value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->Add(key, jsonValue);
    }

    void DataCache::AddToCache(const std::string& tableName, const std::string& key, AZ::s64 value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->Add(key, jsonValue);
    }

    void DataCache::AddToArray(const std::string& tableName, const std::string& arrayName, const char* value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->AddToArray(arrayName, jsonValue);
    }

    void DataCache::AddToArray(const std::string& tableName, const std::string& arrayName, bool value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->AddToArray(arrayName, jsonValue);
    }

    void DataCache::AddToArray(const std::string& tableName, const std::string& arrayName, const AZ::Vector3& value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->AddToArray(arrayName, jsonValue);
    }

    void DataCache::AddToArray(const std::string& tableName, const std::string& arrayName, double value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->AddToArray(arrayName, jsonValue);
    }

    void DataCache::AddToArray(const std::string& tableName, const std::string& arrayName, AZ::u64 value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->AddToArray(arrayName, jsonValue);
    }

    void DataCache::AddToArray(const std::string& tableName, const std::string& arrayName, AZ::s64 value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->AddToArray(arrayName, jsonValue);
    }

    void DataCache::AddToObject(const std::string& tableName, const std::string& objName, const std::string& key, const char* value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->AddToObject(objName, key, jsonValue);
    }

    void DataCache::AddToObject(const std::string& tableName, const std::string& objName, const std::string& key, bool value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->AddToObject(objName, key, jsonValue);
    }

    void DataCache::AddToObject(const std::string& tableName, const std::string& objName, const std::string& key, const AZ::Vector3& value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->AddToObject(objName, key, jsonValue);
    }

    void DataCache::AddToObject(const std::string& tableName, const std::string& objName, const std::string& key, double value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->AddToObject(objName, key, jsonValue);
    }

    void DataCache::AddToObject(const std::string& tableName, const std::string& objName, const std::string& key, AZ::u64 value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->AddToObject(objName, key, jsonValue);
    }

    void DataCache::AddToObject(const std::string& tableName, const std::string& objName, const std::string& key, AZ::s64 value)
    {
        DocumentPtr doc(FindDoc(tableName));

        auto jsonValue = doc->ToJson(value);
        doc->AddToObject(objName, key, jsonValue);
    }

    void DataCache::AddArrayToCache(const std::string& tableName, const std::string& key, const std::string& arrayName)
    {
        DocumentPtr doc(FindDoc(tableName));

        doc->AddArray(key, arrayName);
    }

    void DataCache::AddObjectToCache(const std::string& tableName, const std::string& key, const std::string& objectName)
    {
        DocumentPtr doc(FindDoc(tableName));

        doc->AddObject(key, objectName);
    }

    void DataCache::AddArrayToObject(const std::string& tableName, const std::string& destObjName, const std::string& key, const std::string& srcArrayName)
    {
        DocumentPtr doc(FindDoc(tableName));

        doc->AddArrayToObject(destObjName, key, srcArrayName);
    }

    void DataCache::AddObjectToObject(const std::string& tableName, const std::string& destObjName, const std::string& key, const std::string& srcObjName)
    {
        DocumentPtr doc(FindDoc(tableName));

        doc->AddObjectToObject(destObjName, key, srcObjName);
    }

    void DataCache::AddObjectToArray(const std::string& tableName, const std::string& destArrayName, const std::string& srcObjectName)
    {
        DocumentPtr doc(FindDoc(tableName));

        doc->AddObjectToArray(destArrayName, srcObjectName);
    }

    std::string DataCache::GetDatabasesJSON() const
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutexDatabase);
        rapidjson::Document jsonDoc;
        jsonDoc.SetObject();
        rapidjson::Value dbNames(rapidjson::kArrayType);

        for (const auto& i : m_database)
        {
            rapidjson::Value name;
            name.SetString(i.first.c_str(), jsonDoc.GetAllocator());
            dbNames.PushBack(name, jsonDoc.GetAllocator());
        }

        jsonDoc.AddMember("tables", dbNames, jsonDoc.GetAllocator());

        rapidjson::StringBuffer buffer;
        buffer.Clear();

        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        jsonDoc.Accept(writer);

        return std::string(buffer.GetString());
    }

    std::string DataCache::GetTableKeysJSON(const std::string& tableName) const
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutexDatabase);
        std::string json;

        auto it = m_database.find(tableName);

        if (it != m_database.end())
        {
            json = it->second->GetKeysJSON();
        }

        return json;
    }

    std::string DataCache::GetTableKeyValuesJSON(const std::string& tableName, const std::vector<std::string>& keyList) const
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutexDatabase);
        std::string json;

        auto it = m_database.find(tableName);

        if (it != m_database.end())
        {
            json = it->second->GetKeyValuesJSON(keyList);
        }

        return json;
    }


    void DataCache::ClearCache()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutexDatabase);

        m_database.clear();
    }

    DataCache::DocumentPtr DataCache::FindDoc(const std::string& tableName)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutexDatabase);
        DocumentPtr doc;

        auto docItr = m_database.find(tableName);
        if (docItr == m_database.end())
        {
            doc = AZStd::make_shared<Document>();
            m_database[tableName] = doc;
        }
        else
        {
            doc = docItr->second;
        }

        return doc;
    }


    DataCache::Document::Document()
        : m_jsonDoc()
        , m_allocator(m_jsonDoc.GetAllocator())
    {
        m_jsonDoc.SetObject();
    }

    DataCache::Document::~Document()
    {
        m_jsonValues.clear();
    }

    std::string DataCache::Document::GetKeysJSON() const
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        rapidjson::Document jsonDoc;
        jsonDoc.SetObject();
        rapidjson::Value keyNames(rapidjson::kArrayType);

        for (rapidjson::Value::ConstMemberIterator itr = m_jsonDoc.MemberBegin(); itr != m_jsonDoc.MemberEnd(); ++itr)
        {
            rapidjson::Value name;
            name.SetString(itr->name.GetString(), jsonDoc.GetAllocator());
            keyNames.PushBack(name, jsonDoc.GetAllocator());
        }

        jsonDoc.AddMember("keys", keyNames, jsonDoc.GetAllocator());

        rapidjson::StringBuffer buffer;
        buffer.Clear();

        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        jsonDoc.Accept(writer);

        return std::string(buffer.GetString());
    }

    std::string DataCache::Document::GetKeyValuesJSON(const std::vector<std::string>& keyList) const
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        rapidjson::StringBuffer buffer;
        buffer.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        if ((keyList.size() == 1) && (keyList[0] == "*"))
        {
            m_jsonDoc.Accept(writer);
        }
        else
        {
            rapidjson::Document jsonDoc;
            jsonDoc.SetObject();

            for (rapidjson::Value::ConstMemberIterator itr = m_jsonDoc.MemberBegin(); itr != m_jsonDoc.MemberEnd(); ++itr)
            {
                std::string keyName(itr->name.GetString());
                if (std::find(keyList.cbegin(), keyList.cend(), keyName) != keyList.cend())
                {
                    // do a deep copy for this key...
                    rapidjson::Value v;

                    v.CopyFrom(itr->value, jsonDoc.GetAllocator());

                    jsonDoc.AddMember(rapidjson::Value().SetString(itr->name.GetString(), jsonDoc.GetAllocator()), v, jsonDoc.GetAllocator());
                }
            }

            jsonDoc.Accept(writer);
        }

        return std::string(buffer.GetString());
    }

    std::string DataCache::Document::GetJSON() const
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        rapidjson::StringBuffer buffer;
        buffer.Clear();

        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        m_jsonDoc.Accept(writer);

        return std::string(buffer.GetString());
    }

    void DataCache::Document::Add(const std::string& key, rapidjson::Value& value)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        if (m_jsonDoc.HasMember(key.c_str()))
            m_jsonDoc.RemoveMember(ToJson(key));

        m_jsonDoc.AddMember(ToJson(key), value, m_allocator);
    }

    void DataCache::Document::AddToArray(const std::string& arrayName, rapidjson::Value& value)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutexJsonArray);
        rapidJsonValuePtr rapidValue(FindValue(arrayName, ValueType::Array));

        rapidValue->PushBack(value, m_allocator);
    }

    void DataCache::Document::AddToObject(const std::string& objectName, const std::string& key, rapidjson::Value& value)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutexJsonArray);
        rapidJsonValuePtr rapidValue(FindValue(objectName, ValueType::Object));

        if (rapidValue->HasMember(key.c_str()))
            rapidValue->RemoveMember(ToJson(key));

        rapidValue->AddMember(ToJson(key), value, m_allocator);
    }

    void DataCache::Document::AddArray(const std::string& key, const std::string& arrayName)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutexJsonArray);
        rapidJsonValuePtr rapidValue(FindValue(arrayName, ValueType::Array));

        Add(key, *rapidValue);

        RemoveValue(arrayName, ValueType::Array);
    }

    void DataCache::Document::AddObject(const std::string& key, const std::string& objectName)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutexJsonArray);
        rapidJsonValuePtr rapidValue(FindValue(objectName, ValueType::Object));

        Add(key, *rapidValue);

        RemoveValue(objectName, ValueType::Object);
    }

    void DataCache::Document::AddArrayToObject(const std::string& destObjName, const std::string& key, const std::string& srcArrayName)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutexJsonArray);
        rapidJsonValuePtr jsonDest(FindValue(destObjName, ValueType::Object));
        rapidJsonValuePtr jsonSrc(FindValue(srcArrayName, ValueType::Array));

        jsonDest->AddMember(ToJson(key), *jsonSrc, m_allocator);

        RemoveValue(srcArrayName, ValueType::Array);
    }

    void DataCache::Document::AddObjectToObject(const std::string& destObjName, const std::string& key, const std::string& srcObjName)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutexJsonArray);
        rapidJsonValuePtr jsonDest(FindValue(destObjName, ValueType::Object));
        rapidJsonValuePtr jsonSrc(FindValue(srcObjName, ValueType::Object));

        jsonDest->AddMember(ToJson(key), *jsonSrc, m_allocator);

        RemoveValue(srcObjName, ValueType::Object);
    }

    void DataCache::Document::AddObjectToArray(const std::string& destArrayName, const std::string& srcObjectName)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutexJsonArray);
        rapidJsonValuePtr jsonDest(FindValue(destArrayName, ValueType::Array));
        rapidJsonValuePtr jsonSrc(FindValue(srcObjectName, ValueType::Object));

        jsonDest->PushBack(*jsonSrc, m_allocator);

        RemoveValue(srcObjectName, ValueType::Object);
    }

    rapidjson::Value DataCache::Document::ToJson(const std::string& value)
    {
        rapidjson::Value v;
        v.SetString(value.c_str(), aznumeric_cast<rapidjson::SizeType>(value.size()), m_allocator);
        return v;
    }

    rapidjson::Value DataCache::Document::ToJson(const char* value)
    {
        rapidjson::Value v;
        v.SetString(value == nullptr ? "" : value, m_allocator);
        return v;
    }

    rapidjson::Value DataCache::Document::ToJson(bool value)
    {
        rapidjson::Value v;
        v.SetBool(value);
        return v;
    }

    rapidjson::Value DataCache::Document::ToJson(const AZ::Vector3& value)
    {
        rapidjson::Value v(rapidjson::kArrayType);

        v.PushBack(ToJson(value.GetX()), m_allocator);
        v.PushBack(ToJson(value.GetY()), m_allocator);
        v.PushBack(ToJson(value.GetZ()), m_allocator);

        return v;
    }

    rapidjson::Value DataCache::Document::ToJson(double value)
    {
        return rapidjson::Value(value);
    }

    rapidjson::Value DataCache::Document::ToJson(AZ::u64 value)
    {
        uint64_t uintValue = value;
        return rapidjson::Value(uintValue);
    }

    rapidjson::Value DataCache::Document::ToJson(AZ::s64 value)
    {
        int64_t intValue = value;
        return rapidjson::Value(intValue);
    }

    DataCache::Document::rapidJsonValuePtr DataCache::Document::FindValue(const std::string& name, ValueType type)
    {
        rapidJsonValuePtr valuePtr;
        std::string lookupName(name + std::string((type == ValueType::Array) ? "_Array" : "_Object"));

        auto itr = m_jsonValues.find(lookupName);
        if (itr == m_jsonValues.end())
        {
            valuePtr = AZStd::make_shared<rapidjson::Value>((type == ValueType::Array) ? rapidjson::kArrayType : rapidjson::kObjectType);
            m_jsonValues[lookupName] = valuePtr;
        }
        else
        {
            valuePtr = itr->second;
        }

        return valuePtr;
    }

    void DataCache::Document::RemoveValue(const std::string& name, ValueType type)
    {
        std::string lookupName(name + std::string((type == ValueType::Array) ? "_Array" : "_Object"));

        auto itr = m_jsonValues.find(lookupName);
        if (itr != m_jsonValues.end())
        {
            m_jsonValues.erase(itr);
        }
    }
}
