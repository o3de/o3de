/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Editor/Settings/EditorSettingsBlock.h>

namespace AzToolsFramework
{
    void EditorSettingPropertyValue::SetName(AZStd::string_view name)
    {
        m_name = name;
    }

    EditorSettingPropertyGroup* EditorSettingPropertyGroup::GetGroup(const char* groupName)
    {
        for (EditorSettingPropertyGroup& subGroup : m_groups)
        {
            if (subGroup.m_name == groupName)
            {
                return &subGroup;
            }
        }
        return nullptr;
    }

    EditorSettingPropertyValue* EditorSettingPropertyGroup::GetProperty(const char* propertyName)
    {
        for (EditorSettingPropertyValue* property : m_properties)
        {
            if (property->m_name == propertyName)
            {
                return property;
            }
        }
        return nullptr;
    }

    void EditorSettingPropertyGroup::Clear()
    {
        for (EditorSettingPropertyValue* property : m_properties)
        {
            delete property;
        }
        m_properties.clear();
        m_groups.clear();
    }

    EditorSettingPropertyGroup& EditorSettingPropertyGroup::operator=(EditorSettingPropertyGroup&& rhs)
    {
        m_name.swap(rhs.m_name);
        m_properties.swap(rhs.m_properties);
        m_groups.swap(rhs.m_groups);

        return *this;
    }

    EditorSettingPropertyGroup::~EditorSettingPropertyGroup()
    {
        Clear();
    }

    EditorSettingsBlock::EditorSettingsBlock()
    {
    }

    EditorSettingsBlock::EditorSettingsBlock(AZStd::string_view name)
    {
        m_name = name;
    }

    EditorSettingsBlock::~EditorSettingsBlock()
    {
    }

    void EditorSettingsBlock::AddProperty(EditorSettingProperty settingItem)
    {
        EditorSettingPropertyValue* valuePtr;

        if (settingItem.GetTypeId() == azrtti_typeid<int>())
        {
            valuePtr = new EditorSettingPropertyInt(0);
        }
        /*
        else if (settingItem.GetTypeId() == azrtti_typeid<AZStd::string>())
        {
            valuePtr = new EditorSettingPropertyString("");
        }
        */
        else
        {
            // TODO - Type not supported - return;
            return;
        }

        valuePtr->SetName(settingItem.GetName());
        m_settingValues.m_properties.push_back(valuePtr);
        m_settingProperties.emplace_back(AZStd::move(settingItem));
        m_settingPropertyValueMap.insert({ valuePtr, &m_settingProperties.back() });
    }

    /// Dynamic edit data provider function.  We use this to dynamically override the edit context for each element in the EditorSettingsBlock.
    /// This enables every setting to integrate custom ranges and UI controls.
    /// handlerPtr: pointer to the object whose edit data registered the handler (i.e. the class instance pointer)
    /// elementPtr: pointer to the sub-member of handlePtr that we are querying edit data for (i.e. the member variable)
    /// elementType: uuid of the specific class type of the elementPtr
    /// The function can either return a pointer to the ElementData to use, or nullptr to use the default one.
    const AZ::Edit::ElementData* EditorSettingsBlock::GetSettingPropertyEditData(
        const void* handlerPtr, const void* elementPtr, const AZ::Uuid& /*elementType*/)
    {
        // TODO - Check elementType?

        const EditorSettingsBlock* blockInstance = reinterpret_cast<const EditorSettingsBlock*>(handlerPtr);
        if (blockInstance && elementPtr != &(blockInstance->m_settingValues)
            && blockInstance->m_settingPropertyValueMap.find(elementPtr) != blockInstance->m_settingPropertyValueMap.end())
        {
            return blockInstance->m_settingPropertyValueMap.at(elementPtr)->GetEditData();
        }
        
        return nullptr;
    }

} // namespace AzToolsFramework::Editor
