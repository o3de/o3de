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

#pragma once

#include <AzCore/XML/rapidxml.h>

namespace ProjectSettingsTool
{
    // Wraps a plist dict node with a friendly api to allow lookups and other actions in it
    class PlistDictionary
    {
    public:
        // Constructs the dictionary wrapper
        PlistDictionary(AZ::rapidxml::xml_document<char>* plist);

        // Allocates a new node with given name and value
        AZ::rapidxml::xml_node<char>* MakeNode(const char* name, const char* value);
        AZ::rapidxml::xml_node<char>* MakeNode();

        // Returns pointer to property data node
        AZ::rapidxml::xml_node<char>* GetPropertyValueNode(const char* key);

        // Creates a new property and returns pointer to its value node
        AZ::rapidxml::xml_node<char>* AddProperty(const char* key);
        // Removes property from dictionary
        void RemoveProperty(const char* key);

        // Returns pointer to property data value or nullptr if it is empty
        const char* GetPropertyValue(const char* key);
        const char* GetPropertyValue(AZ::rapidxml::xml_node<char>* node);

        // Returns pointer to property data name or nullptr if it is empty
        const char* GetPropertyValueName(const char* key);
        const char* GetPropertyValueName(AZ::rapidxml::xml_node<char>* node);

        // Changes value of property data and creates it if it doesn't exist
        AZ::rapidxml::xml_node<char>* SetPropertyValue(const char* key, const char* newValue);
        void SetPropertyValue(AZ::rapidxml::xml_node<char>* node, const char* newValue);

        // Changes name of property data and creates it if it doesn't exist
        AZ::rapidxml::xml_node<char>* SetPropertyValueName(const char* key, const char* newName);
        void SetPropertyValueName(AZ::rapidxml::xml_node<char>* node, const char* newName);

        // Checks to make sure a plist file has a valid dictionary
        static bool ContainsValidDict(AZ::rapidxml::xml_document<char>* plist);

    protected:
        // Returns pointer to property key node
        AZ::rapidxml::xml_node<char>* GetPropertyKeyNode(const char* key);


        // pList dictionary is found in
        AZ::rapidxml::xml_document<char>* m_document;
        // The dictionary of properties
        AZ::rapidxml::xml_node<char>* m_dict;
    };
} // namespace ProjectSettingsTool
