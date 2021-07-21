/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzCore/Math/Vector3.h>

#include <string>
#include <vector>

namespace Metastream
{
    class DataCache
    {
    public:
        DataCache() {}
        void AddToCache(const std::string & tableName, const std::string & key, const char *value);
        void AddToCache(const std::string & tableName, const std::string & key, bool value);
        void AddToCache(const std::string & tableName, const std::string & key, const AZ::Vector3& value);
        void AddToCache(const std::string & tableName, const std::string & key, double value);
        void AddToCache(const std::string & tableName, const std::string & key, AZ::u64 value);
        void AddToCache(const std::string & tableName, const std::string & key, AZ::s64 value);
        void AddToArray(const std::string & tableName, const std::string & arrayName, const char* value);
        void AddToArray(const std::string & tableName, const std::string & arrayName, bool value);
        void AddToArray(const std::string & tableName, const std::string & arrayName, const AZ::Vector3& value);
        void AddToArray(const std::string & tableName, const std::string & arrayName, double value);
        void AddToArray(const std::string & tableName, const std::string & arrayName, AZ::u64 value);
        void AddToArray(const std::string & tableName, const std::string & arrayName, AZ::s64 value);
        void AddToObject(const std::string & tableName, const std::string & objName, const std::string & key, const char* value);
        void AddToObject(const std::string & tableName, const std::string & objName, const std::string & key, bool value);
        void AddToObject(const std::string & tableName, const std::string & objName, const std::string & key, const AZ::Vector3& value);
        void AddToObject(const std::string & tableName, const std::string & objName, const std::string & key, double value);
        void AddToObject(const std::string & tableName, const std::string & objName, const std::string & key, AZ::u64 value);
        void AddToObject(const std::string & tableName, const std::string & objName, const std::string & key, AZ::s64 value);

        void AddArrayToCache(const std::string & tableName, const std::string & key, const std::string & arrayName);
        void AddObjectToCache(const std::string & tableName, const std::string & key, const std::string & objectName);
        void AddArrayToObject(const std::string & tableName, const std::string & destObjName, const std::string & key, const std::string & srcArrayName);
        void AddObjectToArray(const std::string & tableName, const std::string & destArrayName, const std::string & srcObjectName);
        void AddObjectToObject(const std::string & tableName, const std::string & destObjName, const std::string & key, const std::string & srcObjName);

        std::string GetDatabasesJSON() const;
        std::string GetTableKeysJSON(const std::string& tableName) const;
        std::string GetTableKeyValuesJSON(const std::string& tableName, const std::vector<std::string>& keyList) const;
        
        void ClearCache();
    
    private:
        class Document
        {
            public:
                Document();
                virtual ~Document();

                std::string GetKeysJSON() const;
                std::string GetKeyValuesJSON(const std::vector<std::string>& keyList) const;
                std::string GetJSON() const;
                
                void Add(const std::string & key, rapidjson::Value & value);
                void AddToArray(const std::string & arrayName, rapidjson::Value & value);
                void AddToObject(const std::string & objName, const std::string & keyName, rapidjson::Value & value);
                void AddArray(const std::string & key, const std::string & arrayName);
                void AddObject(const std::string & key, const std::string & objectName);
                void AddArrayToObject(const std::string & destObjName, const std::string & key, const std::string & srcArrayName);
                void AddObjectToObject(const std::string & destObjName, const std::string & key, const std::string & srcObjName);
                void AddObjectToArray(const std::string & destArrayName, const std::string & srcObjectName);

                rapidjson::Value ToJson(const std::string & value);
                rapidjson::Value ToJson(const char * value);
                rapidjson::Value ToJson(bool value);
                rapidjson::Value ToJson(const AZ::Vector3& value);
                rapidjson::Value ToJson(double value);
                rapidjson::Value ToJson(AZ::u64 value);
                rapidjson::Value ToJson(AZ::s64 value);

            private:
                typedef AZStd::shared_ptr<rapidjson::Value> rapidJsonValuePtr;
                typedef AZStd::map<std::string, rapidJsonValuePtr> JsonValueMap;
                enum class ValueType {Array, Object};
                rapidJsonValuePtr FindValue(const std::string & name, ValueType type);
                void RemoveValue(const std::string &objectName, ValueType type);
        
            private:
                mutable AZStd::mutex                    m_mutex;
                mutable AZStd::mutex                    m_mutexJsonArray;
                rapidjson::Document                     m_jsonDoc;
                rapidjson::Document::AllocatorType &    m_allocator;
                JsonValueMap                            m_jsonValues;
        };

        typedef AZStd::shared_ptr<Document> DocumentPtr;
        typedef AZStd::map<std::string, DocumentPtr> Database;
        
        DocumentPtr FindDoc(const std::string & tableName);

    private:
        mutable AZStd::mutex    m_mutexDatabase;
        Database                m_database;
    };

} // namespace Metastream
