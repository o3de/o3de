/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Editor/Settings/EditorSettingsManager.h>

#include <AzCore/Component/ComponentApplication.h>

#include <AzToolsFramework/Editor/Settings/EditorSettingsContext.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

namespace AzToolsFramework
{
    EditorSettingsManager::EditorSettingsManager()
    {
        AZ::Interface<EditorSettingsInterface>::Register(this);
    }

    void EditorSettingsManager::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorSettingPropertyValue>()
                ->Field("Name", &EditorSettingPropertyValue::m_name)
                ->Field("Description", &EditorSettingPropertyValue::m_description)
                ;

            serializeContext->Class<EditorSettingPropertyInt, EditorSettingPropertyValue>()
                ->Field("Value", &EditorSettingPropertyInt::m_value)
                ;

            serializeContext->Class<EditorSettingPropertyGroup>()
                ->Field("Name", &EditorSettingPropertyGroup::m_name)
                ->Field("Properties", &EditorSettingPropertyGroup::m_properties)
                ->Field("Groups", &EditorSettingPropertyGroup::m_groups)
                ;

            serializeContext->Class<EditorSettingsBlock>()
                ->Field("Name", &EditorSettingsBlock::m_name)
                ->Field("Elements", &EditorSettingsBlock::m_settingValues);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorSettingPropertyValue>("Editor Settings Property", "Base class for editor settings properties")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "EditorSettingProperty's class attributes.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                editContext->Class<EditorSettingPropertyInt>("Editor Settings Property (int)", "An editor setting int property")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "EditorSettingPropertyGroup's class attributes.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(0, &EditorSettingPropertyInt::m_value, "m_value", "Int")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &EditorSettingPropertyValue::m_name)
                        ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &EditorSettingPropertyValue::m_description)
                    ;

                editContext->Class<EditorSettingPropertyGroup>("Editor Setting Property group", "This is an editor setting property group")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "EditorSettingPropertyGroup's class attributes.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorSettingPropertyGroup::m_properties, "m_properties", "Properties in this property group")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(0, &EditorSettingPropertyGroup::m_groups, "m_groups", "Subgroups in this property group")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                editContext->Class<EditorSettingsBlock>("Editor Settings Block", "Dynamically show Editor Settings")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "EditorSettingBlock's class attributes.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorSettingsBlock::m_settingValues, "Elements", "")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &EditorSettingsBlock::m_name)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    //->SetDynamicEditDataProvider(&EditorSettingsBlock::GetSettingPropertyEditData)
                ;
            }
        }
    }

    void EditorSettingsManager::Start()
    {
        AZ::ReflectionEnvironment::GetReflectionManager()->AddReflectContext<EditorSettingsContext>();

        m_settingsRegistry = AZ::SettingsRegistry::Get();
        m_settingsOriginTracker = new EditorSettingsOriginTracker(*m_settingsRegistry);
    }

    EditorSettingsManager::~EditorSettingsManager()
    {
        if (m_settingsDialog != nullptr)
        {
            delete m_settingsDialog;
        }

        AZ::Interface<EditorSettingsInterface>::Unregister(this);
        AZ::ReflectionEnvironment::GetReflectionManager()->RemoveReflectContext<EditorSettingsContext>();
    }

    void EditorSettingsManager::OpenEditorSettingsDialog()
    {
        if (!m_isSetup)
        {
            SetupSettings();
        }

        if (m_settingsDialog == nullptr)
        {
            m_settingsDialog = new EditorSettingsDialog(AzToolsFramework::GetActiveWindow());
        }

        m_settingsDialog->exec();
    }

    CategoryMap* EditorSettingsManager::GetSettingsBlocks()
    {
        return &m_settingItems;
    }

    void EditorSettingsManager::SetupSettings()
    {
        // Organize Settings from EditorSettingsContext into category/subcategory maps
        auto context = AZ::ReflectionEnvironment::GetReflectionManager()->GetReflectContext<EditorSettingsContext>();
        for (auto elem : context->GetSettingsArray())
        {
            if(m_settingItems[elem.GetCategory()].find(elem.GetSubCategory()) == m_settingItems[elem.GetCategory()].end())
            {
                // TODO - Remember to clean!
                m_settingItems[elem.GetCategory()][elem.GetSubCategory()] = new EditorSettingsBlock(elem.GetSubCategory());
            }

            m_settingItems[elem.GetCategory()][elem.GetSubCategory()]->AddProperty(elem);
        }

        m_isSetup = true;
    }
} // namespace AzToolsFramework::Editor
