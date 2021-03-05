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

#include "ProjectSettingsTool_precompiled.h"
#include "PlistDictionary.h"


namespace ProjectSettingsTool
{
    using namespace AZ;
    using XmlDocument = rapidxml::xml_document<char>;
    using XmlNode = rapidxml::xml_node<char>;

    PlistDictionary::PlistDictionary(XmlDocument* plist)
        : m_document(plist)
        , m_dict(plist->first_node("plist")->first_node("dict"))
    {
    }

    XmlNode* PlistDictionary::MakeNode(const char* name, const char* value)
    {
        return m_document->allocate_node
        (
            rapidxml::node_element,
            m_document->allocate_string(name),
            m_document->allocate_string(value)
        );
    }

    XmlNode* PlistDictionary::MakeNode()
    {
        return m_document->allocate_node(rapidxml::node_element);
    }

    XmlNode* PlistDictionary::GetPropertyKeyNode(const char* key)
    {
        XmlNode* keyNode = m_dict->first_node("key");
        // Look for the key in pList's interesting structure
        while (keyNode)
        {
            if (strcmp(keyNode->value(), key) == 0)
            {
                break;
            }

            keyNode = keyNode->next_sibling("key");
        }

        // Returns key if found otherwise nullptr
        return keyNode;
    }

    XmlNode* PlistDictionary::GetPropertyValueNode(const char* key)
    {
        XmlNode* keyNode = GetPropertyKeyNode(key);

        // Key found return data node
        if (keyNode != nullptr)
        {
            return keyNode->next_sibling();
        }
        // Failed to find key return nullptr
         return nullptr;
    }

    XmlNode* PlistDictionary::AddProperty(const char* key)
    {
        m_dict->append_node(MakeNode("key", key));
        XmlNode* data = MakeNode();
        m_dict->append_node(data);
        return data;
    }

    void PlistDictionary::RemoveProperty(const char* key)
    {
        XmlNode* keyNode = GetPropertyKeyNode(key);

        if (keyNode != nullptr)
        {
            XmlNode* dataNode = keyNode->next_sibling();

            m_dict->remove_node(keyNode);
            m_dict->remove_node(dataNode);
        }
    }

    const char* PlistDictionary::GetPropertyValue(const char* key)
    {
        XmlNode* valueNode = GetPropertyValueNode(key);
        return valueNode != nullptr ? GetPropertyValue(valueNode) : nullptr;
    }

    const char* PlistDictionary::GetPropertyValue(XmlNode* node)
    {
        return node->value_size() > 0 ? node->value() : nullptr;
    }

    const char* PlistDictionary::GetPropertyValueName(const char* key)
    {
        XmlNode* valueNode = GetPropertyValueNode(key);
        return valueNode != nullptr ? GetPropertyValueName(valueNode) : nullptr;
    }

    const char* PlistDictionary::GetPropertyValueName(XmlNode* node)
    {
        return node->name_size() > 0 ? node->name() : nullptr;
    }

    XmlNode* PlistDictionary::SetPropertyValue(const char* key, const char* newValue)
    {
        XmlNode* dataNode = GetPropertyValueNode(key);
        if (dataNode == nullptr)
        {
            dataNode = AddProperty(key);
            SetPropertyValueName(dataNode, "string");
        }
        SetPropertyValue(dataNode, newValue);

        return dataNode;
    }

    void PlistDictionary::SetPropertyValue(XmlNode* node, const char* newValue)
    {
        node->value(m_document->allocate_string(newValue));
    }

    XmlNode* PlistDictionary::SetPropertyValueName(const char* key, const char* newName)
    {
        XmlNode* dataNode = GetPropertyValueNode(key);
        if (dataNode == nullptr)
        {
            dataNode = AddProperty(key);
        }
        SetPropertyValueName(dataNode, newName);

        return dataNode;
    }

    void PlistDictionary::SetPropertyValueName(XmlNode* node, const char* newName)
    {
        node->name(m_document->allocate_string(newName));
    }

    bool PlistDictionary::ContainsValidDict(XmlDocument* plist)
    {
        XmlNode* node = plist->first_node("plist");
        if (node != nullptr)
        {
            node = node->first_node("dict");
            if (node != nullptr)
            {
                return true;
            }
        }

        return false;
    }
} // namespace ProjectSettingsTool