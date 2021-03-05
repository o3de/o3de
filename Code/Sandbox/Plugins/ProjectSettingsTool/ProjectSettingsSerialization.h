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

#include "PlistDictionary.h"

#include <AzCore/JSON/document.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AzToolsFramework
{
    class InstanceDataNode;
}

namespace ProjectSettingsTool
{
    class Serializer
    {
    public:
        Serializer(AzToolsFramework::InstanceDataNode* root, rapidjson::Document* doc, rapidjson::Value* jsonRoot = nullptr);
        Serializer(AzToolsFramework::InstanceDataNode* root, AZStd::unique_ptr<PlistDictionary> dict);
        // Sets Json document
        void SetDocumentRoot(rapidjson::Document* doc);
        // Sets Json root
        void SetJsonRoot(rapidjson::Value* jsonRoot);
        // Sets pList dictionary
        void SetDocumentRoot(AZStd::unique_ptr<PlistDictionary> dict);
        // Returns true if all properties in ui are equal to settings
        bool UiEqualToSettings() const;
        // Loads properties into the ui from the settings
        void LoadFromSettings();
        // Saves properties from the ui to the settings
        void SaveToSettings();
        bool UiEqualToJson(rapidjson::Value* root) const;
        void LoadFromSettings(rapidjson::Value* root);
        void SaveToSettings(rapidjson::Value* root);


    protected:
        bool UiEqualToJson(rapidjson::Value* root, AzToolsFramework::InstanceDataNode* node) const;
        bool UiEqualToPlist(AzToolsFramework::InstanceDataNode* node) const;
        bool UiEqualToPlistArray(AZ::rapidxml::xml_node<char>* array, AzToolsFramework::InstanceDataNode* node) const;
        bool UiEqualToPlistImages(AzToolsFramework::InstanceDataNode* node) const;
        void LoadFromSettings(rapidjson::Value* root, AzToolsFramework::InstanceDataNode* node);
        void LoadFromSettings(AzToolsFramework::InstanceDataNode* node);
        void LoadOrientations(AZ::rapidxml::xml_node<char>* array, AzToolsFramework::InstanceDataNode* node);
        void SetDefaults(AzToolsFramework::InstanceDataNode& node, const AZ::Uuid& type);
        void SetClassToDefaults(AzToolsFramework::InstanceDataNode* node);
        void SaveToSettings(rapidjson::Value* root, AzToolsFramework::InstanceDataNode* node);
        void SaveToSettings(AzToolsFramework::InstanceDataNode* node);
        bool SaveOrientations(AZ::rapidxml::xml_node<char>* array, AzToolsFramework::InstanceDataNode* node);
        void OverwriteImages(AzToolsFramework::InstanceDataNode* node);

        // The RPE root relative to the document's root
        AzToolsFramework::InstanceDataNode* m_root;
        // The Json document if using Json for this RPE
        rapidjson::Document* m_jsonDoc;
        // The root of the json for this serializer
        rapidjson::Value* m_jsonRoot;
        // The pList Dictionary wrapper if using a pList for this RPE
        AZStd::unique_ptr<PlistDictionary> m_plistDict;

    private:
        Serializer(AzToolsFramework::InstanceDataNode* root);

        // Uuid for AZStd::string
        const AZ::Uuid m_idString;
        // Uuid for int
        const AZ::Uuid m_idInt;
        // Uuid for bool
        const AZ::Uuid m_idBool;
    };
}   // ProjectSettingsTool
