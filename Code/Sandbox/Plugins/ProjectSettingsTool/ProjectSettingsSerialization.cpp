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
#include "ProjectSettingsSerialization.h"

#include "PlatformSettings_common.h"

#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

// Needed to overwrite ios icons and launchscreens
#include <Util/FileUtil.h>

namespace ProjectSettingsTool
{
    using XmlNode = AZ::rapidxml::xml_node<char>;

    const char* stringStr = "string";
    const char* arrayStr = "array";
    const char* trueStr = "true";
    const char* noDocumentError = "No json or xml document to use for Project Settings Tool serialization.";

    Serializer::Serializer(AzToolsFramework::InstanceDataNode* root)
        : m_root(root)
        , m_jsonDoc(nullptr)
        , m_jsonRoot(nullptr)
        , m_idString(AZ::SerializeTypeInfo<AZStd::string>::GetUuid())
        , m_idInt(AZ::SerializeTypeInfo<int>::GetUuid())
        , m_idBool(AZ::SerializeTypeInfo<bool>::GetUuid())
    {}

    Serializer::Serializer(AzToolsFramework::InstanceDataNode* root, rapidjson::Document* doc, rapidjson::Value* jsonRoot)
        : Serializer(root)
    {
        SetDocumentRoot(doc);
        if (jsonRoot != nullptr)
        {
            SetJsonRoot(jsonRoot);
        }
        else
        {
            SetJsonRoot(doc);
        }
    }

    Serializer::Serializer(AzToolsFramework::InstanceDataNode* root, AZStd::unique_ptr<PlistDictionary> dict)
        : Serializer(root)
    {
        SetDocumentRoot(AZStd::move(dict));
    }

    void Serializer::SetDocumentRoot(rapidjson::Document* doc)
    {
        m_jsonDoc = doc;
    }

    void Serializer::SetJsonRoot(rapidjson::Value* jsonRoot)
    {
        m_jsonRoot = jsonRoot;
    }

    void Serializer::SetDocumentRoot(AZStd::unique_ptr<PlistDictionary> dict)
    {
        m_plistDict = AZStd::move(dict);
    }

    bool Serializer::UiEqualToSettings() const
    {
        if (m_jsonRoot != nullptr)
        {
            return UiEqualToJson(m_jsonRoot);
        }
        else if (m_plistDict)
        {
            return UiEqualToPlist(m_root);
        }
        else
        {
            AZ_Assert(false, noDocumentError);
            return false;
        }
    }

    void Serializer::LoadFromSettings()
    {
        if (m_jsonRoot != nullptr)
        {
            LoadFromSettings(m_jsonRoot);
        }
        else if (m_plistDict)
        {
            LoadFromSettings(m_root);
        }
        else
        {
            AZ_Assert(false, noDocumentError);
        }
    }

    void Serializer::SaveToSettings()
    {
        if (m_jsonRoot != nullptr)
        {
            SaveToSettings(m_jsonRoot);
        }
        else if (m_plistDict)
        {
            SaveToSettings(m_root);
        }
        else
        {
            AZ_Assert(false, noDocumentError);
        }
    }

    bool Serializer::UiEqualToJson(rapidjson::Value* root) const
    {
        return UiEqualToJson(root, m_root);
    }

    void Serializer::LoadFromSettings(rapidjson::Value* root)
    {
        LoadFromSettings(root, m_root);
    }

    void Serializer::SaveToSettings(rapidjson::Value* root)
    {
        // This value needs to be an object to add members
        if (!root->IsObject())
        {
            root->SetObject();
        }
        SaveToSettings(root, m_root);
    }

    bool Serializer::UiEqualToJson(rapidjson::Value* root, AzToolsFramework::InstanceDataNode* node) const
    {
        const AZ::SerializeContext::ClassData* baseMeta = node->GetClassMetadata();
        if (baseMeta)
        {
            for (AzToolsFramework::InstanceDataNode& childNode : node->GetChildren())
            {
                auto childMeta = childNode.GetElementMetadata();
                if (childMeta)
                {
                    bool noDocElement = true;
                    rapidjson::Value::MemberIterator jsonMember;
                    const char* propertyName = childMeta->m_name;

                    // If there is a json object try to find the member
                    if (root != nullptr)
                    {
                        jsonMember = root->FindMember(propertyName);

                        if (jsonMember != root->MemberEnd())
                        {
                            noDocElement = false;
                        }
                    }

                    AZ::Uuid type = childMeta->m_typeId;
                    if (m_idString == type)
                    {
                        AZStd::string uiValue;
                        childNode.Read(uiValue);

                        if (!uiValue.empty() && !noDocElement)
                        {
                            if (jsonMember->value.IsString())
                            {
                                if (uiValue != jsonMember->value.GetString())
                                {
                                    return false;
                                }
                            }
                            else
                            {
                                return false;
                            }
                        }
                        else if (uiValue.empty() != noDocElement)
                        {
                            return false;
                        }
                    }
                    else if (m_idInt == type)
                    {
                        int uiValue;
                        childNode.Read(uiValue);

                        if (!noDocElement)
                        {
                            if (jsonMember->value.IsInt())
                            {
                                if (uiValue != jsonMember->value.GetInt())
                                {
                                    return false;
                                }
                            }
                            else
                            {
                                return false;
                            }
                        }
                        else
                        {
                            return false;
                        }
                    }
                    else if (m_idBool == type)
                    {
                        bool uiValue;
                        childNode.Read(uiValue);

                        if (uiValue)
                        {
                            if (!noDocElement)
                            {
                                if (jsonMember->value.IsString())
                                {
                                    AZStd::string expectedJsonValue = trueStr;
                                    if (expectedJsonValue != jsonMember->value.GetString())
                                    {
                                        return false;
                                    }
                                }
                                else
                                {
                                    return false;
                                }
                            }
                            else
                            {
                                return false;
                            }
                        }
                        else if (!noDocElement)
                        {
                            return false;
                        }
                    }
                    // Should be a class with members instead of base data type
                    else
                    {
                        rapidjson::Value* jsonNode = nullptr;
                        if (!noDocElement)
                        {
                            jsonNode = &jsonMember->value;
                        }
                        // Drill into class
                        if (!UiEqualToJson(jsonNode, &childNode))
                        {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    bool Serializer::UiEqualToPlist(AzToolsFramework::InstanceDataNode* node) const
    {
        const AZ::SerializeContext::ClassData* baseMeta = node->GetClassMetadata();
        if (baseMeta)
        {
            for (AzToolsFramework::InstanceDataNode& childNode : node->GetChildren())
            {
                auto childMeta = childNode.GetElementMetadata();
                if (childMeta)
                {
                    bool noDocElement = true;
                    const char* propertyName = childMeta->m_name;
                    XmlNode* plistNode = m_plistDict->GetPropertyValueNode(propertyName);

                    if (plistNode != nullptr)
                    {
                        noDocElement = false;
                    }

                    AZ::Uuid type = childMeta->m_typeId;
                    if (m_idString == type)
                    {
                        AZStd::string uiValue;
                        childNode.Read(uiValue);

                        if (!uiValue.empty() && !noDocElement)
                        {
                            if (AZStd::string(plistNode->name()) == stringStr)
                            {
                                if (uiValue != plistNode->value())
                                {
                                    return false;
                                }
                            }
                            else
                            {
                                return false;
                            }
                        }
                        else if (uiValue.empty() != noDocElement)
                        {
                            return false;
                        }
                    }
                    else if (m_idBool == type)
                    {
                        bool uiValue;
                        childNode.Read(uiValue);

                        if (uiValue)
                        {
                            if (!noDocElement)
                            {
                                if (AZStd::string(plistNode->name()) != trueStr)
                                {
                                    return false;
                                }
                            }
                            else
                            {
                                return false;
                            }
                        }
                        else if (!noDocElement)
                        {
                            return false;
                        }
                    }
                    else if (AZStd::string(childNode.GetClassMetadata()->m_name) == "IosOrientations")
                    {
                        // Enter even if nullptr to make sure all values should be false
                        if (plistNode == nullptr || AZStd::string(plistNode->name()) == arrayStr)
                        {
                            if (!UiEqualToPlistArray(plistNode, &childNode))
                            {
                                return false;
                            }
                        }
                        else
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (!UiEqualToPlistImages(&childNode))
                        {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    bool Serializer::UiEqualToPlistArray(AZ::rapidxml::xml_node<char>* array, AzToolsFramework::InstanceDataNode* node) const
    {
        const AZ::SerializeContext::ClassData* baseMeta = node->GetClassMetadata();
        if (baseMeta)
        {
            for (AzToolsFramework::InstanceDataNode& childNode : node->GetChildren())
            {
                const AZ::SerializeContext::ClassElement* childMeta = childNode.GetElementMetadata();
                if (childMeta)
                {
                    AZ::Uuid type = childMeta->m_typeId;
                    if (m_idBool == type)
                    {
                        const char* propertyName = childMeta->m_name;
                        bool uiValue;
                        childNode.Read(uiValue);

                        bool found = false;
                        if (array != nullptr)
                        {
                            for (XmlNode* plistNode = array->first_node(); plistNode != nullptr; plistNode = plistNode->next_sibling())
                            {
                                if (AZStd::string(plistNode->value()) == propertyName)
                                {
                                    if (!uiValue)
                                    {
                                        return false;
                                    }
                                    else
                                    {
                                        found = true;
                                    }
                                    break;
                                }
                            }
                        }
                        if (uiValue && !found)
                        {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    bool Serializer::UiEqualToPlistImages(AzToolsFramework::InstanceDataNode* node) const
    {
        const AZ::SerializeContext::ClassData* baseMeta = node->GetClassMetadata();
        if (baseMeta)
        {
            for (AzToolsFramework::InstanceDataNode& childNode : node->GetChildren())
            {
                auto childMeta = childNode.GetElementMetadata();
                if (childMeta)
                {
                    AZ::Uuid type = childMeta->m_typeId;

                    if (m_idString == type)
                    {
                        AZStd::string uiValue;
                        childNode.Read(uiValue);

                        if (!uiValue.empty())
                        {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    void Serializer::LoadFromSettings(rapidjson::Value* root, AzToolsFramework::InstanceDataNode* node)
    {
        const AZ::SerializeContext::ClassData* baseMeta = node->GetClassMetadata();
        if (baseMeta)
        {
            for (AzToolsFramework::InstanceDataNode& childNode : node->GetChildren())
            {
                auto childMeta = childNode.GetElementMetadata();
                if (childMeta)
                {
                    const char* propertyName = childMeta->m_name;
                    AZ::Uuid type = childMeta->m_typeId;

                    if (root != nullptr)
                    {
                        rapidjson::Value::MemberIterator jsonMember = root->FindMember(propertyName);

                        if (jsonMember != root->MemberEnd())
                        {
                            rapidjson::Value& jsonNode = jsonMember->value;

                            if (m_idString == type)
                            {
                                if (jsonNode.IsString())
                                {
                                    AZStd::string jsonValue = jsonNode.GetString();
                                    childNode.Write(jsonValue);
                                }
                            }
                            else if (m_idInt == type)
                            {
                                if (jsonNode.IsInt())
                                {
                                    int jsonValue = jsonNode.GetInt();
                                    childNode.Write(jsonValue);
                                }
                            }
                            else if (m_idBool == type)
                            {
                                if (jsonNode.IsString())
                                {
                                    bool value = false;
                                    AZStd::string jsonValue = jsonNode.GetString();

                                    if (jsonValue == trueStr)
                                    {
                                        value = true;
                                    }
                                    childNode.Write(value);
                                }
                            }
                            // Should be a class with members instead of base data type
                            else
                            {
                                if (jsonNode.IsObject())
                                {
                                    // Drill into class
                                    LoadFromSettings(&jsonNode, &childNode);
                                }
                            }
                        }
                        else
                        {
                            SetDefaults(childNode, type);
                        }
                    }
                    else
                    {
                        SetDefaults(childNode, type);
                    }
                }
            }
        }
    }

    void Serializer::SaveToSettings(rapidjson::Value* root, AzToolsFramework::InstanceDataNode* node)
    {
        const AZ::SerializeContext::ClassData* baseMeta = node->GetClassMetadata();
        if (baseMeta)
        {
            for (AzToolsFramework::InstanceDataNode& childNode : node->GetChildren())
            {
                auto childMeta = childNode.GetElementMetadata();
                if (childMeta)
                {
                    const char* propertyName = childMeta->m_name;
                    rapidjson::Value::MemberIterator jsonMember = root->FindMember(propertyName);

                    if (jsonMember == root->MemberEnd())
                    {
                        root->AddMember(rapidjson::Value(propertyName, m_jsonDoc->GetAllocator())
                            , rapidjson::Value(rapidjson::Type::kNullType)
                            , m_jsonDoc->GetAllocator());
                        jsonMember = root->FindMember(propertyName);
                    }
                    rapidjson::Value& jsonNode = jsonMember->value;

                    AZ::Uuid type = childMeta->m_typeId;
                    if (m_idString == type)
                    {
                        AZStd::string uiValue;
                        childNode.Read(uiValue);

                        if (!uiValue.empty())
                        {
                            jsonNode.SetString(uiValue.data(), m_jsonDoc->GetAllocator());
                        }
                        else
                        {
                            root->RemoveMember(propertyName);
                        }
                    }
                    else if (m_idInt == type)
                    {
                        int uiValue;
                        childNode.Read(uiValue);
                        jsonNode.SetInt(uiValue);
                    }
                    else if (m_idBool == type)
                    {
                        bool uiValue;
                        childNode.Read(uiValue);

                        if (uiValue)
                        {
                            jsonNode.SetString(trueStr, m_jsonDoc->GetAllocator());
                        }
                        else
                        {
                            root->RemoveMember(propertyName);
                        }
                    }
                    // Should be a class with members instead of base data type
                    else
                    {
                        // This value needs to be an object to add members
                        if (!jsonNode.IsObject())
                        {
                            jsonNode.SetObject();
                        }
                        // Drill into class
                        SaveToSettings(&jsonNode, &childNode);

                        if (jsonNode.ObjectEmpty())
                        {
                            root->RemoveMember(propertyName);
                        }
                    }
                }
            }
        }
    }

    void Serializer::LoadFromSettings(AzToolsFramework::InstanceDataNode* node)
    {
        const AZ::SerializeContext::ClassData* baseMeta = node->GetClassMetadata();
        if (baseMeta)
        {
            for (AzToolsFramework::InstanceDataNode& childNode : node->GetChildren())
            {
                const AZ::SerializeContext::ClassElement* childMeta = childNode.GetElementMetadata();
                if (childMeta)
                {
                    const char* propertyName = childMeta->m_name;
                    XmlNode* plistNode = m_plistDict->GetPropertyValueNode(propertyName);
                    AZ::Uuid type = childMeta->m_typeId;

                    if (plistNode != nullptr)
                    {
                        if (m_idString == type)
                        {
                            if (AZStd::string(plistNode->name()) == stringStr)
                            {
                                AZStd::string plistValue = plistNode->value();
                                childNode.Write(plistValue);
                            }
                        }
                        else if (m_idBool == type)
                        {
                            if (AZStd::string(plistNode->name()) == trueStr)
                            {
                                childNode.Write(true);
                            }
                        }
                        else if (AZStd::string(childNode.GetClassMetadata()->m_name) == "IosOrientations")
                        {
                            // Make sure it seems like an array in plist as well
                            if (AZStd::string(plistNode->name()) == arrayStr)
                            {
                                LoadOrientations(plistNode, &childNode);
                            }
                        }
                        else
                        {
                            SetClassToDefaults(&childNode);
                        }
                    }
                    else
                    {
                        SetDefaults(childNode, type);
                    }
                }
            }
        }
    }

    void Serializer::LoadOrientations(XmlNode* array, AzToolsFramework::InstanceDataNode* node)
    {
        const AZ::SerializeContext::ClassData* baseMeta = node->GetClassMetadata();
        if (baseMeta)
        {
            for (AzToolsFramework::InstanceDataNode& childNode : node->GetChildren())
            {
                const AZ::SerializeContext::ClassElement* childMeta = childNode.GetElementMetadata();
                if (childMeta)
                {
                    const char* propertyName = childMeta->m_name;

                    bool found = false;
                    if (array != nullptr)
                    {
                        for (XmlNode* plistNode = array->first_node(); plistNode != nullptr; plistNode = plistNode->next_sibling())
                        {
                            if (AZStd::string(plistNode->value()) == propertyName)
                            {
                                AZ::Uuid type = childMeta->m_typeId;

                                if (m_idBool == type)
                                {
                                    if (AZStd::string(plistNode->name()) == stringStr)
                                    {
                                        childNode.Write(true);
                                        found = true;
                                    }
                                }
                                else
                                {
                                    AZ_Assert(false, "Unsupported type \"%s\" found in array.", childMeta->m_editData->m_name)
                                }
                                break;
                            }
                        }
                    }
                    if (!found)
                    {
                        childNode.Write(false);
                    }
                }
            }
        }
    }

    void Serializer::SetClassToDefaults(AzToolsFramework::InstanceDataNode* node)
    {
        const AZ::SerializeContext::ClassData* baseMeta = node->GetClassMetadata();
        if (baseMeta)
        {
            for (AzToolsFramework::InstanceDataNode& childNode : node->GetChildren())
            {
                auto childMeta = childNode.GetElementMetadata();
                if (childMeta)
                {
                    AZ::Uuid type = childMeta->m_typeId;

                    SetDefaults(childNode, type);
                }
            }
        }
    }

    void Serializer::SetDefaults(AzToolsFramework::InstanceDataNode& node, const AZ::Uuid& type)
    {
        if (m_idString == type)
        {
            node.Write(AZStd::string(""));
            return;
        }
        else if (m_idBool == type)
        {
            node.Write(false);
            return;
        }
        else if (m_idInt == type)
        {
            node.Write(0);
            return;
        }

        const AZ::SerializeContext::ClassData* classMeta = node.GetClassMetadata();

        if (AZStd::string(node.GetClassMetadata()->m_name) == "IosOrientations")
        {
            LoadOrientations(nullptr, &node);
        }
        else
        {
            SetClassToDefaults(&node);
        }
    }

    void Serializer::SaveToSettings(AzToolsFramework::InstanceDataNode* node)
    {
        const AZ::SerializeContext::ClassData* baseMeta = node->GetClassMetadata();
        if (baseMeta)
        {
            for (AzToolsFramework::InstanceDataNode& childNode : node->GetChildren())
            {
                auto childMeta = childNode.GetElementMetadata();
                if (childMeta)
                {
                    const char* propertyName = childMeta->m_name;

                    AZ::Uuid type = childMeta->m_typeId;

                    if (m_idString == type)
                    {
                        AZStd::string uiValue;
                        childNode.Read(uiValue);

                        if (!uiValue.empty())
                        {
                            m_plistDict->SetPropertyValue(propertyName, uiValue.data());
                        }
                        else
                        {
                            m_plistDict->RemoveProperty(propertyName);
                        }
                    }
                    else if (m_idBool == type)
                    {
                        bool uiValue;
                        childNode.Read(uiValue);

                        if (uiValue)
                        {
                            m_plistDict->SetPropertyValueName(propertyName, trueStr);
                            m_plistDict->SetPropertyValue(propertyName, "");
                        }
                        else
                        {
                            m_plistDict->RemoveProperty(propertyName);
                        }
                    }
                    else if (AZStd::string(childNode.GetClassMetadata()->m_name) == "IosOrientations")
                    {
                        XmlNode* plistNode = m_plistDict->GetPropertyValueNode(propertyName);
                        if (plistNode == nullptr)
                        {
                            plistNode = m_plistDict->SetPropertyValueName(propertyName, arrayStr);
                        }
                        // Make sure the plist says this is an array type
                        if (AZStd::string(plistNode->name()) == arrayStr)
                        {
                            if (!SaveOrientations(plistNode, &childNode))
                            {
                                m_plistDict->RemoveProperty(propertyName);
                            }
                        }
                    }
                    //Assume this is a class with image overrides
                    else
                    {
                        OverwriteImages(&childNode);
                    }
                }
            }
        }
    }

    bool Serializer::SaveOrientations(XmlNode* array, AzToolsFramework::InstanceDataNode* node)
    {
        bool anyEnabled = false;

        const AZ::SerializeContext::ClassData* baseMeta = node->GetClassMetadata();
        if (baseMeta)
        {
            for (AzToolsFramework::InstanceDataNode& childNode : node->GetChildren())
            {
                const AZ::SerializeContext::ClassElement* childMeta = childNode.GetElementMetadata();
                if (childMeta)
                {
                    AZ::Uuid type = childMeta->m_typeId;
                    if (m_idBool == type)
                    {
                        const char* propertyName = childMeta->m_name;
                        bool uiValue;
                        childNode.Read(uiValue);

                        if (uiValue)
                        {
                            anyEnabled = true;
                        }

                        bool found = false;
                        for (XmlNode* plistNode = array->first_node(); plistNode != nullptr; plistNode = plistNode->next_sibling())
                        {
                            if (AZStd::string(plistNode->value()) == propertyName)
                            {
                                if (!uiValue)
                                {
                                    array->remove_node(plistNode);
                                }
                                else
                                {
                                    found = true;
                                }
                                break;
                            }
                        }
                        if (uiValue && !found)
                        {
                            array->append_node(m_plistDict->MakeNode(stringStr, propertyName));
                        }
                    }
                }
            }
        }

        return anyEnabled;
    }

    void Serializer::OverwriteImages(AzToolsFramework::InstanceDataNode* node)
    {
        const AZ::SerializeContext::ClassData* baseMeta = node->GetClassMetadata();
        if (baseMeta)
        {
            for (AzToolsFramework::InstanceDataNode& childNode : node->GetChildren())
            {
                auto childMeta = childNode.GetElementMetadata();
                if (childMeta)
                {
                    AZ::Uuid type = childMeta->m_typeId;

                    if (m_idString == type)
                    {
                        AZStd::string uiValue;
                        childNode.Read(uiValue);

                        if (!uiValue.empty())
                        {
                            AZ::Edit::ElementData* childEditMeta = childMeta->m_editData;

                            if (childEditMeta != nullptr)
                            {
                                AZ::AttributeId handlerType = childEditMeta->m_elementId;

                                // Special handling for iOS image overrides, the source images must be overwritten.
                                if (handlerType == Handlers::ImagePreview)
                                {
                                    AZ::Edit::Attribute* defaultPathAttr = childEditMeta->FindAttribute(Attributes::DefaultPath);
                                    if (defaultPathAttr != nullptr)
                                    {
                                        AzToolsFramework::PropertyAttributeReader reader(defaultPathAttr->GetContextData(), defaultPathAttr);
                                        AZStd::string defaultPath;
                                        if (reader.Read<AZStd::string>(defaultPath))
                                        {
                                            QString defaultPathQString = defaultPath.data();
                                            CFileUtil::OverwriteFile(defaultPathQString);
                                            CFileUtil::CopyFile(QString(uiValue.data()), defaultPathQString);
                                            // Clear the property so this isn't overwritten again for no reason
                                            childNode.Write(AZStd::string(""));
                                        }
                                    }
                                    else
                                    {
                                        AZ_Assert(false, "Could not find default path for \"%s\". Cannot override image.", childMeta->m_name)
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        AZ_Assert(false, "Unsupported type \"%s\" found in what should be image overrides.", childMeta->m_editData->m_name);
                    }
                }
            }
        }
    }
}   // ProjectSettingsTool
