/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/XML/rapidxml.h>

namespace ProjectSettingsTool
{
    using XmlDocument = AZ::rapidxml::xml_document<char>;
    using XmlNode = AZ::rapidxml::xml_node<char>;
     
    // Wraps a plist dict node with a friendly api to allow lookups and other actions in it
    class PlistDictionary
    {
    public:
        // Constructs the dictionary wrapper
        PlistDictionary(XmlDocument* plist);

        // Allocates a new node with given name and value
        XmlNode* MakeNode(const char* name, const char* value);
        XmlNode* MakeNode();

        // Returns pointer to property data node
        XmlNode* GetPropertyValueNode(const char* key);

        // Creates a new property and returns pointer to its value node
        XmlNode* AddProperty(const char* key);
        // Removes property from dictionary
        void RemoveProperty(const char* key);

        // Returns pointer to property data value or nullptr if it is empty
        const char* GetPropertyValue(const char* key);
        const char* GetPropertyValue(XmlNode* node);

        // Returns pointer to property data name or nullptr if it is empty
        const char* GetPropertyValueName(const char* key);
        const char* GetPropertyValueName(XmlNode* node);

        // Changes value of property data and creates it if it doesn't exist
        XmlNode* SetPropertyValue(const char* key, const char* newValue);
        void SetPropertyValue(XmlNode* node, const char* newValue);

        // Changes name of property data and creates it if it doesn't exist
        XmlNode* SetPropertyValueName(const char* key, const char* newName);
        void SetPropertyValueName(XmlNode* node, const char* newName);

        // Checks to make sure a plist file has a valid dictionary
        static bool ContainsValidDict(XmlDocument* plist);

    protected:
        // Returns pointer to property key node
        XmlNode* GetPropertyKeyNode(const char* key);


        // pList dictionary is found in
        XmlDocument* m_document;
        // The dictionary of properties
        XmlNode* m_dict;
    };
} // namespace ProjectSettingsTool
