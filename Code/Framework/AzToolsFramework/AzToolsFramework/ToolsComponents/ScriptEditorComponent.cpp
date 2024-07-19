
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ToolsComponents/ScriptEditorComponent.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/EBus/Results.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/std/sort.h>
#include <AzCore/Script/ScriptContextDebug.h>

extern "C" {
#include<Lua/lualib.h>
#include<Lua/lauxlib.h>
}

namespace AZ
{
    namespace Edit
    {
        class AttributeDynamicScriptValue
            : public Attribute
        {
        public:
            AZ_RTTI(AttributeDynamicScriptValue, "{46803928-11c9-4176-b2fe-2f0aed99bfeb}", Attribute); 
            AZ_CLASS_ALLOCATOR(AttributeDynamicScriptValue, AZ::SystemAllocator)

            AttributeDynamicScriptValue(const DynamicSerializableField& value)
                : m_value(value) {}
            ~AttributeDynamicScriptValue() override 
            { 
                m_value.DestroyData();
            }

            template<class T>
            bool Get(T& value) const
            {
                // We deal only with exact types no base types, etc. 
                // We can do that if needed, but introduces lots of complications
                // which we are not convinced they are worth it.
                //if (dhtypeid(AZStd::remove_pointer<T>::type) == m_value.m_typeId)
                if ( AzTypeInfo<T>::Uuid() == m_value.m_typeId)
                {
                    GetValue(value, AZStd::is_pointer<T>::type());
                    return true;
                }
                return false;
            }

            template<class T>
            void GetValue(T& value, AZStd::false_type /*AZStd::is_pointer<T>::type()*/) { value = *reinterpret_cast<T*>(m_value.m_data); }
            template<class T>
            void GetValue(T& value, AZStd::true_type /*AZStd::is_pointer<T>::type()*/) { value = reinterpret_cast<T>(m_value.m_data); }

            DynamicSerializableField m_value;
        };
    } // namespace Edit
} // namespace AZ

namespace AzToolsFramework
{
    namespace Components
    {
        const char* AssetTypeName = "asset";
        const char* EntityRefName = "entity";
        const char* UiFieldName = "ui";
        const char* UiOrderValue = "order";
        const char* DescriptionFieldName = "description";

        bool ScriptEditorComponent::DoComponentsMatch(const ScriptEditorComponent* thisComponent, const ScriptEditorComponent* otherComponent)
        {
            // These are "matching" iff the script asset is the same between them
            return thisComponent->m_scriptAsset.GetId() == otherComponent->m_scriptAsset.GetId();
        }

        //=========================================================================
        // ~ScriptEditorComponent
        //=========================================================================
        ScriptEditorComponent::~ScriptEditorComponent()
        {
            ClearDataElements();
        }

        //=========================================================================
        // Init
        //=========================================================================
        void ScriptEditorComponent::Init()
        {
            // Ensure m_scriptComponent's asset reference is up to date on deserialize.
            SetScript(m_scriptAsset);
        }

        //=========================================================================
        // Activate
        //=========================================================================
        void ScriptEditorComponent::Activate()
        {
            // Setup the context
            m_scriptComponent.Init();

            if (m_scriptAsset.GetId().IsValid())
            {
                // Re-retrieve the asset in case it was reloaded while we were inactive.
                m_scriptAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::ScriptAsset>(m_scriptAsset.GetId(), m_scriptAsset.GetAutoLoadBehavior());
                SetScript(m_scriptAsset);

                AZ::Data::AssetBus::Handler::BusConnect(m_scriptAsset.GetId());
                m_scriptAsset.QueueLoad();
            }
            else
            {
                SetScript({ nullptr, AZ::Data::AssetLoadBehavior::Default });
            }
        }

        //=========================================================================
        // Deactivate
        //=========================================================================
        void ScriptEditorComponent::Deactivate()
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
            ClearDataElements();
        }

        //=========================================================================
        // CacheString
        //=========================================================================
        const char* ScriptEditorComponent::CacheString(const char* str)
        {
            if (str == nullptr)
            {
                return nullptr;
            }

            return m_cachedStrings.insert(AZStd::make_pair(str, AZStd::string(str))).first->second.c_str();
        }

        //=========================================================================
        // LoadDefaultAsset
        //=========================================================================
        bool ScriptEditorComponent::LoadDefaultAsset(AZ::ScriptDataContext& sdc, int valueIndex, const char* name, AzFramework::ScriptPropertyGroup& group, ElementInfo& elementInfo)
        {
            LSV_BEGIN(sdc.GetNativeContext(), 0);

            (void)elementInfo;

            //Test to see if in fact is an asset, and then if so assign it.
            AZ::ScriptDataContext arrayTable;
            if (sdc.IsTable(valueIndex) && sdc.InspectTable(valueIndex, arrayTable))
            {
                const char* fieldName;
                int fieldIndex;
                int elementIndex;

                int count = 0;

                while (arrayTable.InspectNextElement(elementIndex, fieldName, fieldIndex))
                {
                    if (!arrayTable.IsString(elementIndex) || fieldName == nullptr || strcmp(fieldName, AssetTypeName))
                    {
                        return false;
                    }

                    ++count;
                }

                if (count == 1)
                {
                    group.m_properties.emplace_back(aznew AZ::ScriptPropertyAsset(name));
                    return true;
                }
            }

            return false;
        }

        //=========================================================================
        // LoadDefaultEntityRef
        //=========================================================================
        bool ScriptEditorComponent::LoadDefaultEntityRef(AZ::ScriptDataContext& sdc, int valueIndex, const char* name, AzFramework::ScriptPropertyGroup& group, ElementInfo& elementInfo)
        {
            LSV_BEGIN(sdc.GetNativeContext(), 0);

            AZ::ScriptDataContext arrayTable;
            if (sdc.IsTable(valueIndex) && sdc.InspectTable(valueIndex, arrayTable))
            {
                const char* fieldName;
                int fieldIndex;
                int elementIndex;
                
                bool isEntity = false;

                const size_t entityRefNameLength = strlen(EntityRefName);
                const size_t descriptionFieldNameLength = strlen(DescriptionFieldName);
                while (arrayTable.InspectNextElement(elementIndex, fieldName, fieldIndex))
                {
                    if (fieldName == nullptr || 0 != strncmp(fieldName, EntityRefName, entityRefNameLength))
                    {
                        if (fieldName && 0 == azstrnicmp(fieldName, DescriptionFieldName, descriptionFieldNameLength))
                        {
                            if (arrayTable.IsString(elementIndex))
                            {
                                arrayTable.ReadValue(elementIndex, elementInfo.m_editData.m_description);
                            }
                        }

                        continue;
                    }
                    else
                    {
                        isEntity = true;
                    }
                }

                if (isEntity)
                {
                    (void)group;
                    AZ::Data::AssetInfo scriptInfo;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(scriptInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, m_scriptAsset.GetId());
                    AZ_Warning("Script Component", false, "%s: The syntax \"%s = { entity = [value], ... }\" for declaring entity references has been deprecated, please use \"%s = { default = EntityId(), ... }\" instead.", scriptInfo.m_relativePath.c_str(), name, name);

                    // Create an EntityId instance
                    const AZ::SerializeContext* context = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                    const AZ::SerializeContext::ClassData* entityIdClassData = context->FindClassData(azrtti_typeid<AZ::EntityId>());
                    AZ_Assert(entityIdClassData && entityIdClassData->m_factory, "AZ::EntityId is missing ClassData or factory in the SerializeContext");
                    AZ::EntityId* entityId = static_cast<AZ::EntityId*>(entityIdClassData->m_factory->Create(name));

                    // Create new property
                    AZ::DynamicSerializableField field;
                    field.Set(entityId);
                    group.m_properties.emplace_back(aznew AZ::ScriptPropertyGenericClass(name, field));
                    return true;
                }
            }

            return false;
        }

        //=========================================================================
        // LoadAttribute
        //=========================================================================
        bool ScriptEditorComponent::LoadAttribute(AZ::ScriptDataContext& sdc, int valueIndex, const char* name, AZ::Edit::ElementData& ed, AZ::ScriptProperty* prop)
        {
            LSV_BEGIN(sdc.GetNativeContext(), 0);

            /////////////////////////////////////////////////////////////////////////////////
            // This is an example that you can do you the custom OnUnhandledAttribute message 
            // check for data element properties, not real attributes.
            if (azstricmp(name, "enumValues") == 0)
            {
                if (azrtti_istypeof<AZ::ScriptPropertyString>(prop))
                {
                    return LoadEnumValuesString(sdc, valueIndex, ed);
                }
                else if (azrtti_istypeof<AZ::ScriptPropertyNumber>(prop))
                {
                    return LoadEnumValuesDouble(sdc, valueIndex, ed);
                }
                else
                {
                    AZ_Warning("Script", false, "enumValues must have the same type as the default value! Currently we support numbers and strings!");
                    return false;
                }
            }
            /////////////////////////////////////////////////////////////////////////////////

            // just attribute key/value pairs
            if (sdc.IsNumber(valueIndex))
            {
                double value;
                sdc.ReadValue(valueIndex, value);
                ed.m_attributes.push_back(AZ::Edit::AttributePair(AZ::Crc32(name), aznew AZ::Edit::AttributeData<double>(value)));
            }
            else if (sdc.IsBoolean(valueIndex))
            {
                bool value;
                sdc.ReadValue(valueIndex, value);
                ed.m_attributes.push_back(AZ::Edit::AttributePair(AZ::Crc32(name), aznew AZ::Edit::AttributeData<bool>(value)));
            }
            else if (sdc.IsString(valueIndex))
            {
                const char* value = nullptr;
                sdc.ReadValue(valueIndex, value);
                value = CacheString(value);
                ed.m_attributes.push_back(AZ::Edit::AttributePair(AZ::Crc32(name), aznew AZ::Edit::AttributeData<const char*>(value)));
            }
            else if (sdc.IsRegisteredClass(valueIndex))
            {
                AZ::DynamicSerializableField value;
                value.m_data = AZ::Internal::LuaAnyClassFromStack(sdc.GetNativeContext(), sdc.GetStartIndex() + valueIndex, &value.m_typeId);
                if (value.m_data)
                {
                    ed.m_attributes.push_back(AZ::Edit::AttributePair(AZ::Crc32(name), aznew AZ::Edit::AttributeDynamicScriptValue(value)));
                }
            }
            else if (sdc.IsFunction(valueIndex))
            {
                // TODO we need a new LUA Function attribute AttributeDynamicScriptFunction TODO when we have a use case
                // We need to cache the script context and function name (of cache the function in the VM, which should not be called often)
                AZ_Assert(false, "Function as property attribute is not implemented yet!");
            }
            else
            {
                return false;
            }

            return true;
        }

        //=========================================================================
        // LoadAttribute
        //=========================================================================
        bool ScriptEditorComponent::LoadEnumValuesDouble(AZ::ScriptDataContext& sdc, int valueIndex, AZ::Edit::ElementData& ed)
        {
            LSV_BEGIN(sdc.GetNativeContext(), 0);

            typedef AZStd::pair<double, AZStd::string> EnumPairType;
            bool isValidValues = false;
            if (sdc.IsTable(valueIndex))
            {
                AZ::ScriptDataContext enumValuesTable;
                if (sdc.InspectTable(valueIndex, enumValuesTable))
                {
                    const char* fieldName;
                    int fieldIndex;
                    int enumIndex;
                    while (enumValuesTable.InspectNextElement(enumIndex, fieldName, fieldIndex))
                    {
                        bool isValidValue = false;
                        EnumPairType enumValue(0, "Enum Value");
                        if (enumValuesTable.IsNumber(enumIndex))
                        {
                            double value;
                            enumValuesTable.ReadValue(enumIndex, value);
                            enumValue.first = value;
                            isValidValue = true;
                        }
                        else if (enumValuesTable.IsTable(enumIndex))
                        {
                            AZ::ScriptDataContext valueTable;
                            if (enumValuesTable.InspectTable(enumIndex, valueTable))
                            {
                                const char* tableFieldName;
                                int tableFieldIndex;
                                int enumValueIndex;
                                while (valueTable.InspectNextElement(enumValueIndex, tableFieldName, tableFieldIndex))
                                {
                                    if (valueTable.IsNumber(enumValueIndex) && tableFieldIndex == 1)
                                    {
                                        double value;
                                        valueTable.ReadValue(enumValueIndex, value);
                                        enumValue.first = value;
                                        isValidValue = true;
                                    }
                                    else if (valueTable.IsString(enumValueIndex) && tableFieldIndex == 2)
                                    {
                                        const char* value = nullptr;
                                        valueTable.ReadValue(enumValueIndex, value);
                                        enumValue.second = value;
                                    }
                                }
                            }
                        }

                        AZ_Warning("Script", isValidValue, "Attribute enumValues can be table containing either be a number (default description is used) or\
                                                                                                                                                                                                                                   a table (value is at index 1 and description is at index 2). Example: EnumValues = { { 1, 'My value 1'}, 2 }\n");
                        if (isValidValue)
                        {
                            isValidValues = true;
                            ed.m_attributes.push_back(AZ::Edit::AttributePair(AZ::Edit::InternalAttributes::EnumValue, aznew AZ::Edit::AttributeData<EnumPairType>(enumValue)));
                        }
                    }
                }
            }

            return isValidValues;
        }

        //=========================================================================
        // LoadAttribute
        //=========================================================================
        bool ScriptEditorComponent::LoadEnumValuesString(AZ::ScriptDataContext& sdc, int valueIndex, AZ::Edit::ElementData& ed)
        {
            LSV_BEGIN(sdc.GetNativeContext(), 0);

            typedef AZStd::pair<AZStd::string, AZStd::string> EnumPairType;
            bool isValidValues = false;
            if (sdc.IsTable(valueIndex))
            {
                AZ::ScriptDataContext enumValuesTable;
                if (sdc.InspectTable(valueIndex, enumValuesTable))
                {
                    const char* fieldName;
                    int fieldIndex;
                    int enumIndex;
                    while (enumValuesTable.InspectNextElement(enumIndex, fieldName, fieldIndex))
                    {
                        bool isValidValue = false;
                        EnumPairType enumValue("", "Enum Value");
                        if (enumValuesTable.IsString(enumIndex))
                        {
                            const char* value = nullptr;
                            enumValuesTable.ReadValue(enumIndex, value);
                            enumValue.first = value;
                            isValidValue = true;
                        }
                        else if (enumValuesTable.IsTable(enumIndex))
                        {
                            AZ::ScriptDataContext valueTable;
                            if (enumValuesTable.InspectTable(enumIndex, valueTable))
                            {
                                const char* tableFieldName = nullptr;
                                int tableFieldIndex;
                                int enumValueIndex;
                                while (valueTable.InspectNextElement(enumValueIndex, tableFieldName, tableFieldIndex))
                                {
                                    if (valueTable.IsString(enumValueIndex) && tableFieldIndex == 1)
                                    {
                                        const char* value = nullptr;
                                        valueTable.ReadValue(enumValueIndex, value);
                                        enumValue.first = value;
                                        isValidValue = true;
                                    }
                                    else if (valueTable.IsString(enumValueIndex) && tableFieldIndex == 2)
                                    {
                                        const char* value = nullptr;
                                        valueTable.ReadValue(enumValueIndex, value);
                                        enumValue.second = value;
                                    }
                                }
                            }
                        }

                        AZ_Warning("Script", isValidValue, "Attribute enumValues can be table containing either be a string (default description is used) or\
                            a table (value is at index 1 and description is at index 2). Example: EnumValues = { { 'String', 'My string value'}, 'String2' }\n");
                        if (isValidValue)
                        {
                            isValidValues = true;
                            ed.m_attributes.push_back(AZ::Edit::AttributePair(AZ::Edit::InternalAttributes::EnumValue, aznew AZ::Edit::AttributeData<EnumPairType>(enumValue)));
                        }
                    }
                }
            }

            return isValidValues;
        }

        void ScriptEditorComponent::LoadProperties(AZ::ScriptDataContext& sdc, AzFramework::ScriptPropertyGroup& group)
        {
            LSV_BEGIN(sdc.GetNativeContext(), -2);

            const bool restrictToPropertyArrays = true;

            const char* propertyName;
            int propertyFieldIndex;
            int propertyIndex;
            while (sdc.InspectNextElement(propertyIndex, propertyName, propertyFieldIndex))
            {
                if (propertyName == nullptr)
                {
                    continue;                          // skip index elements
                }
                ElementInfo ei;
                ei.m_editData.m_name = CacheString(propertyName);
                ei.m_editData.m_description = "";
                ei.m_editData.m_elementId = AZ::Edit::UIHandlers::Default;
                ei.m_sortOrder = FLT_MAX;
                bool isValidProperty = true;

                if (sdc.IsTable(propertyIndex))
                {
                    AZ::ScriptDataContext propertyTable;
                    if (sdc.InspectTable(propertyIndex, propertyTable))
                    {
                        int defaultValueIndex = 0;
                        if (propertyTable.PushTableElement(AzFramework::ScriptComponent::DefaultFieldName, &defaultValueIndex))  // Is this a value or a group
                        {
                            bool needToCreateProperty = false;
                            AZ::ScriptProperty* groupProperty = group.GetProperty(propertyName);
                            if (!groupProperty)
                            {
                                needToCreateProperty = true;
                            }
                            else
                            {
                                if (groupProperty->DoesTypeMatch(propertyTable, defaultValueIndex))
                                {
                                    needToCreateProperty = false;
                                }
                                else
                                {
                                    if (auto itr = AZStd::find(group.m_properties.begin(), group.m_properties.end(), groupProperty))
                                    {
                                        if (itr != group.m_properties.end())
                                        { 
                                            delete *itr;
                                            group.m_properties.erase(itr);
                                        }
                                    }
                                    needToCreateProperty = true;
                                }
                            }

                            if (needToCreateProperty)
                            {
                                if (AZ::ScriptProperty* scriptProperty = propertyTable.ConstructScriptProperty(defaultValueIndex, propertyName, restrictToPropertyArrays))
                                {
                                    group.m_properties.emplace_back(scriptProperty);
                                }
                                else
                                {
                                    AZ_Warning("Script", false, "We support only boolean, number, string and registered classes as properties. Check '%s'!", propertyName);
                                    isValidProperty = false;
                                }
                            }
                            else if (azrtti_istypeof<AZ::ScriptPropertyGenericClassArray>(groupProperty))
                            {
                                // If this is a GenericClassArray, check if it has an unset element type ID, construct a vanilla copy, and populate it from that
                                AZ::ScriptPropertyGenericClassArray* groupArrayProperty = azdynamic_cast<AZ::ScriptPropertyGenericClassArray*>(groupProperty);
                                if (groupArrayProperty->GetElementTypeUuid().IsNull())
                                {
                                    AZ::ScriptProperty* tempScriptProperty = propertyTable.ConstructScriptProperty(defaultValueIndex, propertyName, restrictToPropertyArrays);
                                    AZ::ScriptPropertyGenericClassArray* tempArrayProperty = azdynamic_cast<AZ::ScriptPropertyGenericClassArray*>(tempScriptProperty);
                                    if (tempArrayProperty)
                                    {
                                        groupArrayProperty->SetElementTypeUuid(tempArrayProperty->GetElementTypeUuid());
                                    }
                                    
                                    if(tempScriptProperty)
                                    {
                                        delete tempScriptProperty;
                                    }
                                }
                            }

                            // load all other elements as attributes
                            if (isValidProperty)
                            {
                                if (sdc.InspectTable(propertyIndex, propertyTable)) // restart table inspect
                                {
                                    const char* attrName = nullptr;
                                    int attrFieldIndex = 0; // We won't use this, it's just for arrays
                                    int attrIndex = 0;
                                    while (propertyTable.InspectNextElement(attrIndex, attrName, attrFieldIndex) && isValidProperty)
                                    {
                                        if (attrName == nullptr)
                                        {
                                            AZ_Warning("Script", false, "LUA Script Error - Malformed Entry in Property Table: %s", propertyName);
                                            isValidProperty = false;
                                            continue;   // Skip malformed properties
                                        }
                                        else if (strcmp(attrName, AzFramework::ScriptComponent::DefaultFieldName) == 0)
                                        {
                                            continue; // skip parsed fields
                                        }
                                        else if (strcmp(attrName, DescriptionFieldName) == 0)
                                        {
                                            // Handle 'description' field
                                            if (propertyTable.IsString(attrIndex))
                                            {
                                                const char* value = nullptr;
                                                propertyTable.ReadValue(attrIndex, value);
                                                ei.m_editData.m_description = value;
                                            }
                                        }
                                        else if (strcmp(attrName, UiFieldName) == 0)
                                        {
                                            // Handle 'ui' field
                                            // See: AZ::Edit::UIHandlers
                                            if (propertyTable.IsNumber(attrIndex))
                                            {
                                                AZ::u64 value = 0;
                                                propertyTable.ReadValue(attrIndex, value);
                                                ei.m_editData.m_elementId = aznumeric_cast<AZ::u32>(value);
                                            }
                                            else if (propertyTable.IsString(attrIndex))
                                            {
                                                const char* value = nullptr;
                                                propertyTable.ReadValue(attrIndex, value);
                                                ei.m_editData.m_elementId = AZ::Crc32(value);
                                            }
                                        }
                                        else if (strcmp(attrName, UiOrderValue) == 0)
                                        {
                                            // Handle ui sorting value
                                            if (propertyTable.IsNumber(attrIndex))
                                            {
                                                propertyTable.ReadValue(attrIndex, ei.m_sortOrder);
                                            }
                                        }
                                        else
                                        {
                                            LoadAttribute(propertyTable, attrIndex, attrName, ei.m_editData, group.m_properties.back());
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            // check if we have any elements with a key, otherwise it might be a value array m_property = { true, false }
                            bool isValueArray = true;
                            const char* elementName = nullptr;
                            int elementValueIndex = 0;
                            int elementArrayIndex = 0;
                            while (propertyTable.InspectNextElement(elementValueIndex, elementName, elementArrayIndex))
                            {
                                if (elementName != nullptr)
                                {
                                    isValueArray = false;
                                    break;
                                }
                            }

                            // Check first if this is an asset
                            if (!isValueArray && LoadDefaultAsset(sdc, propertyIndex, propertyName, group, ei))
                            {
                                // Load DefaultAsset adds the property if required, do nothing here
                            }
                            // Check if this is an entity ref
                            else if (!isValueArray && LoadDefaultEntityRef(sdc, propertyIndex, propertyName, group, ei))
                            {
                                // Load DefaultAsset adds the property if required, do nothing here
                            }
                            // If not this must be a group or a user error, we assume group
                            else if (!isValueArray && sdc.InspectTable(propertyIndex, propertyTable)) // restart table inspect
                            {
                                isValidProperty = false;
                                AzFramework::ScriptPropertyGroup* subGroup = group.GetGroup(propertyName);
                                if (subGroup == nullptr)
                                {
                                    group.m_groups.emplace_back();
                                    subGroup = &group.m_groups.back();
                                    subGroup->m_name = propertyName;
                                }
                                LoadProperties(propertyTable, *subGroup);
                            }
                            else
                            {
                                // This case will handle the syntax `Property = { EntityId(), }` of declaring arrays
                                if (AZ::ScriptProperty* scriptProperty = sdc.ConstructScriptProperty(propertyIndex, propertyName, restrictToPropertyArrays))
                                {
                                    group.m_properties.emplace_back(scriptProperty);
                                }
                                else
                                {
                                    AZ_Warning("Script", false, "We support only boolean, number, string and registered classes as properties %s!", propertyName);
                                    isValidProperty = false;
                                }
                            }
                        }
                    }
                }
                else
                {
                    if (!group.GetProperty(propertyName))
                    {
                        // handle value properties
                        if (AZ::ScriptProperty* scriptProperty = sdc.ConstructScriptProperty(propertyIndex, propertyName, restrictToPropertyArrays))
                        {
                            group.m_properties.emplace_back(scriptProperty);
                        }
                        else
                        {
                            AZ_Warning("Script", false, "We support only boolean, number, string and registered classes as properties %s!", propertyName);
                            isValidProperty = false;
                        }
                    }
                }

                if (isValidProperty)
                {
                    if (AZ::ScriptProperty* scriptProperty = group.GetProperty(propertyName))
                    {
                        // add the attributes to the map of default values
                        ei.m_uuid = scriptProperty->GetDataTypeUuid();
                        ei.m_isAttributeOwner = true;
                        m_dataElements.insert(AZStd::make_pair(scriptProperty->GetDataAddress(), ei));

                        // Also register to the script property itself, so friendly data can be displayed at its own level.
                        ei.m_uuid = azrtti_typeid(scriptProperty);
                        ei.m_isAttributeOwner = false;
                        m_dataElements.insert(AZStd::make_pair(scriptProperty, ei));
                    }
                }
            }
        }

        void ScriptEditorComponent::RemovedOldProperties(AzFramework::ScriptPropertyGroup& group)
        {
            for (auto it = group.m_properties.begin(); it != group.m_properties.end();)
            {
                AZ::ScriptProperty* prop = *it;
                if (m_dataElements.find(prop->GetDataAddress()) == m_dataElements.end())
                {
                    delete prop;
                    it = group.m_properties.erase(it); // we did not find a data element it must an old property
                }
                else
                {
                    ++it;
                }
            }

            for (auto it = group.m_groups.begin(); it != group.m_groups.end();)
            {
                AzFramework::ScriptPropertyGroup& subGroup = *it;
                RemovedOldProperties(subGroup);
                if (subGroup.m_properties.empty() && subGroup.m_groups.empty())
                {
                    it = group.m_groups.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void ScriptEditorComponent::SortProperties(AzFramework::ScriptPropertyGroup& group)
        {
            // sort properties
            AZStd::sort(group.m_properties.begin(), group.m_properties.end(),
                    [&](const AZ::ScriptProperty* lhs, const AZ::ScriptProperty* rhs) -> bool
            {
                auto lhsElementInfoPair = m_dataElements.find(lhs);
                auto rhsElementInfoPair = m_dataElements.find(rhs);
                AZ_Assert(lhsElementInfoPair != m_dataElements.end() && rhsElementInfoPair != m_dataElements.end(), "We have script properties that have do not have dataElements, this should not be possible!");
                if (lhsElementInfoPair->second.m_sortOrder == FLT_MAX && rhsElementInfoPair->second.m_sortOrder == FLT_MAX)
                {
                    // use alphabetical sort
                    return lhs->m_name < rhs->m_name;
                }
                else
                {
                    // sort based on the order defined by user
                    return lhsElementInfoPair->second.m_sortOrder < rhsElementInfoPair->second.m_sortOrder;
                }
            }
            );

            // process subgroups (should we sort groups? in general or alphabetically)
            for (AzFramework::ScriptPropertyGroup& subGroup : group.m_groups)
            {
                SortProperties(subGroup);
            }
        }

        void ScriptEditorComponent::LoadScript()
        {
            LSV_BEGIN(m_scriptComponent.m_context->NativeContext(), 0);

            // At this point we're loading the script to populate the properties.
            // Disable debugging during this stage as the game is not running yet.
            bool pausedBreakpoints = false;
            if (m_scriptComponent.m_context->GetDebugContext())
            {
                m_scriptComponent.m_context->GetDebugContext()->DisconnectHook();
                pausedBreakpoints = true;
            }

            if (m_scriptComponent.LoadInContext())
            {
                LoadProperties();
            }

            if (pausedBreakpoints)
            {
                m_scriptComponent.m_context->GetDebugContext()->ConnectHook();
            }

            InvalidatePropertyDisplay(Refresh_EntireTree);
            ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::AddDirtyEntity, GetEntityId());
        }

        void ScriptEditorComponent::LoadProperties()
        {
            ClearDataElements();

            AZ::ScriptContext* context = m_scriptComponent.m_context;
            LSV_BEGIN(context->NativeContext(), -1);

            bool success = false;
            AZ::ScriptSystemRequestBus::BroadcastResult(success, &AZ::ScriptSystemRequestBus::Events::Load, GetScript(), AZ::k_scriptLoadBinaryOrText, context->GetId());

            if (!success)
            {
                return;
            }

            AZ::ScriptDataContext stackContext;
            if (context->ReadStack(stackContext))
            {
                // Create reference to table, so that we may call InspectTableCached
                int value = lua_gettop(context->NativeContext());
                int ref = stackContext.CacheValue(value);

                // maintaining the same stack manipulations as in the previous states. Since cache value duplicates the target value
                // then creates a reference, vs destructively making something into a reference directly from the stack.
                lua_pop(context->NativeContext(),1);

                AZ::ScriptDataContext dc;
                if (context->InspectTableCached(ref, dc))
                {
                    int valueIndex;
                    if (dc.PushTableElement(m_scriptComponent.m_properties.m_name.c_str(), &valueIndex) &&
                        dc.IsTable(valueIndex))
                    {
                        AZ::ScriptDataContext propTable;
                        if (dc.InspectTable(valueIndex, propTable))
                        {
                            LoadProperties(propTable, m_scriptComponent.m_properties);
                        }
                    }
                }

                stackContext.ReleaseCached(ref);
            }

            // Pop the script table
            lua_pop(context->NativeContext(), 1);

            // TODO: Call this at the end of a session, instead of on load. This will allow 
            // users to quickly revert back to old properties instead of losing them right away.

            // Remove all old properties, every confirmed property will have
            // a corresponding Element data
            RemovedOldProperties(m_scriptComponent.m_properties);

            SortProperties(m_scriptComponent.m_properties);

            InvalidatePropertyDisplay(Refresh_EntireTree);
        }

        void ScriptEditorComponent::ClearDataElements()
        {
            for (auto it = m_dataElements.begin(); it != m_dataElements.end(); ++it)
            {
                if (it->second.m_isAttributeOwner)
                {
                    it->second.m_editData.ClearAttributes();
                }
            }

            m_dataElements.clear();

            // The display tree might still be holding onto pointers to our attributes that we just cleared above, so force a refresh to remove them.
            // However, only force the refresh if we have a valid entity.  If we don't have an entity, this component isn't currently being shown or
            // edited, so a refresh is at best superfluous, and at worst could cause a feedback loop of infinite refreshes.
            if (GetEntity())
            {
                InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);
            }
        }

        const AZ::Edit::ElementData* ScriptEditorComponent::GetDataElement(const void* element, const AZ::Uuid& typeUuid) const
        {
            auto it = m_dataElements.find(element);
            if (it != m_dataElements.end())
            {
                if (it->second.m_uuid == typeUuid)
                {
                    return &it->second.m_editData;
                }
            }
            return nullptr;
        }

        void ScriptEditorComponent::BuildGameEntity(AZ::Entity* gameEntity)
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!context)
            {
                AZ_Error("ScriptEditorComponent", false, "Can't get serialize context from component application.");
                return;
            }

            auto scriptComponent = context->CloneObject(&m_scriptComponent);
            // guarantee that the script is set to Preload
            scriptComponent->SetScript(scriptComponent->GetScript());
            gameEntity->AddComponent(scriptComponent);
        }

        void ScriptEditorComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
        {
            AZ::Data::Asset<AZ::ScriptAsset> asset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AZ::ScriptAsset>(assetId, m_scriptAsset.GetAutoLoadBehavior());
            if (asset)
            {
                SetScript(asset);
                ScriptHasChanged();
                AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, GetEntityId());
            }
        }

        AZ::u32 ScriptEditorComponent::ScriptHasChanged()
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();

            // Only clear properties and data elements if the asset we're changing to is not the same one we already had set on our scriptComponent
            // The only time we shouldn't do this is when someone has set the same script on the component through the editor
            if (m_scriptAsset != m_scriptComponent.GetScript())
            {
                m_scriptComponent.m_properties.Clear();
                ClearDataElements();
            }

            if (m_scriptAsset.GetId().IsValid())
            {
                SetScript(m_scriptAsset);

                AZ::Data::AssetBus::Handler::BusConnect(m_scriptAsset.GetId());
                m_scriptAsset.QueueLoad();
            }
            else
            {
                SetScript({nullptr, AZ::Data::AssetLoadBehavior::Default});
            }

            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        void ScriptEditorComponent::LaunchLuaEditor(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType&)
        {
            AZStd::string scriptFilename;
            if (assetId.IsValid())
            {
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(scriptFilename,
                    &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, assetId);
            }
            AzToolsFramework::EditorRequests::Bus::Broadcast(
                &AzToolsFramework::EditorRequests::Bus::Events::LaunchLuaEditor, scriptFilename.c_str());
        }

        void ScriptEditorComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            SetScript(asset);
            LoadScript();
        }

        void ScriptEditorComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            SetScript(asset);
            LoadScript();
        }

        void ScriptEditorComponent::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            // only notify for asset errors for the asset we care about.
#if defined(AZ_ENABLE_TRACING)
            if ((asset.GetId().IsValid()) && (asset == m_scriptAsset))
            {
                AZ_Error("Lua Script", false, "Failed to load asset for ScriptComponent: %s", m_scriptAsset.ToString<AZStd::string>().c_str());
            }
#else // else if AZ_ENABLE_TRACING is not currently defined...
            AZ_UNUSED(asset);
#endif
        }

        void ScriptEditorComponent::SetScript(const AZ::Data::Asset<AZ::ScriptAsset>& script) 
        {
            m_scriptAsset = script; 
            m_scriptComponent.SetScript(script);

            m_customName = "Lua Script";

            if (script)
            {
                 bool outcome = false;
                AZStd::string folderFoundIn;
                AZ::Data::AssetInfo assetInfo;

                AssetSystemRequestBus::BroadcastResult(outcome, &AssetSystemRequestBus::Events::GetSourceInfoBySourceUUID, m_scriptAsset.GetId().m_guid, assetInfo, folderFoundIn);

                if (outcome)
                {
                    AZStd::string name;
                    AzFramework::StringFunc::Path::GetFileName(assetInfo.m_relativePath.c_str(), name);
                    m_customName += AZStd::string::format(" - %s", name.c_str());
                }
            }
        }

        const AZ::Edit::ElementData* ScriptEditorComponent::GetScriptPropertyEditData(const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType)
        {
            const ScriptEditorComponent* owner = reinterpret_cast<const ScriptEditorComponent*>(handlerPtr);
            return owner->GetDataElement(elementPtr, elementType);
        }

        void ScriptEditorComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

            if (serializeContext)
            {
                // NOTE: We perform the reflection for AZ::ScriptComponent here because we are making use of the editor-only
                // PropertyVisibility in the editor reflection.
                AzFramework::ScriptComponent::Reflect(context);

                serializeContext->Class<ScriptEditorComponent, EditorComponentBase>()
                    ->Version(2)
                    ->Field("ScriptComponent", &ScriptEditorComponent::m_scriptComponent)
                    ->Field("ScriptAsset", &ScriptEditorComponent::m_scriptAsset);

                AZ::EditContext* ec = serializeContext->GetEditContext();
                if (ec)
                {
                    ec->Class<ScriptEditorComponent>("Lua Script", "The Lua Script component allows you to add arbitrary Lua logic to an entity in the form of a Lua script")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("CanvasUI"))
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &ScriptEditorComponent::m_customName)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/LuaScript.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/LuaScript.svg")
                        ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<AZ::ScriptAsset>::Uuid())
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Script.png")
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/scripting/lua-script/")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ScriptEditorComponent::m_scriptAsset, "Script", "Which script to use")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ScriptEditorComponent::ScriptHasChanged)
                            ->Attribute("BrowseIcon", ":/stylesheet/img/UI20/browse-edit-select-files.svg")
                            ->Attribute("EditButton", "")
                            ->Attribute("EditDescription", "Open in Lua Editor")
                            ->Attribute("EditCallback", &ScriptEditorComponent::LaunchLuaEditor)
                        ->DataElement(nullptr, &ScriptEditorComponent::m_scriptComponent, "Script properties", "The script template")
                        ->SetDynamicEditDataProvider(&ScriptEditorComponent::GetScriptPropertyEditData)
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    ec->Class<AzFramework::ScriptComponent>("Script Component", "Adding scripting functionality to the entity!")
                        ->DataElement(nullptr, &AzFramework::ScriptComponent::m_properties, "Properties", "Lua script properties")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(nullptr, &AzFramework::ScriptComponent::m_script, "Asset", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                            ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::NotPushable) // Only the editor-component's script asset needs to be slice-pushable.
                        ;

                    ec->Class<AzFramework::ScriptPropertyGroup>("Script Property group", "This is a script property group")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "ScriptPropertyGroup's class attributes.")->
                            Attribute(AZ::Edit::Attributes::NameLabelOverride, &AzFramework::ScriptPropertyGroup::m_name)->
                            Attribute(AZ::Edit::Attributes::AutoExpand, true)->
                        DataElement(nullptr, &AzFramework::ScriptPropertyGroup::m_properties, "m_properties", "Properties in this property group")->
                            Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)->
                            Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)->
                        DataElement(nullptr, &AzFramework::ScriptPropertyGroup::m_groups, "m_groups", "Subgroups in this property group")->
                            Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                    ec->Class<AZ::ScriptProperty>("Script Property", "Base class for script properties")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "ScriptPropertyGroup's class attributes.")->
                            Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                    ec->Class<AZ::ScriptPropertyBoolean>("Script Property (bool)", "A script boolean property")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "ScriptPropertyGroup's class attributes.")->
                            Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)->
                        DataElement(nullptr, &AZ::ScriptPropertyBoolean::m_value, "m_value", "A boolean")->
                            Attribute(AZ::Edit::Attributes::NameLabelOverride, &AZ::ScriptProperty::m_name);

                    ec->Class<AZ::ScriptPropertyNumber>("Script Property (number)", "A script number property")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "ScriptPropertyGroup's class attributes.")->
                            Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)->
                        DataElement(nullptr, &AZ::ScriptPropertyNumber::m_value, "m_value", "A number")->
                            Attribute(AZ::Edit::Attributes::NameLabelOverride, &AZ::ScriptProperty::m_name);

                    ec->Class<AZ::ScriptPropertyString>("Script Property (string)", "A script string property")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "ScriptPropertyGroup's class attributes.")->
                            Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)->
                        DataElement(nullptr, &AZ::ScriptPropertyString::m_value, "m_value", "A string")->
                            Attribute(AZ::Edit::Attributes::NameLabelOverride, &AZ::ScriptProperty::m_name);

                    ec->Class<AZ::ScriptPropertyGenericClass>("Script Property (object)", "A script object property")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "ScriptPropertyGroup's class attributes.")->
                            Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)->
                        DataElement(nullptr, &AZ::ScriptPropertyGenericClass::m_value, "m_value", "An object")->
                            Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                    ec->Class<AZ::ScriptPropertyBooleanArray>("Script Property Array(bool)", "A script bool array property")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "ScriptPropertyBooleanArray's class attributes.")->
                            Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)->
                        DataElement(nullptr, &AZ::ScriptPropertyBooleanArray::m_values, "m_value", "An object")->
                            Attribute(AZ::Edit::Attributes::NameLabelOverride, &AZ::ScriptProperty::m_name);

                    ec->Class<AZ::ScriptPropertyNumberArray>("Script Property Array(number)", "A script number array property")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "ScriptPropertyNumberArray's class attributes.")->
                            Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)->
                        DataElement(nullptr, &AZ::ScriptPropertyNumberArray::m_values, "m_value", "An object")->
                            Attribute(AZ::Edit::Attributes::NameLabelOverride, &AZ::ScriptProperty::m_name);

                    ec->Class<AZ::ScriptPropertyStringArray>("Script Property Array(string)", "A script string array property")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "ScriptPropertyStringArray's class attributes.")->
                            Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)->
                        DataElement(nullptr, &AZ::ScriptPropertyStringArray::m_values, "m_value", "An object")->
                            Attribute(AZ::Edit::Attributes::NameLabelOverride, &AZ::ScriptProperty::m_name);

                    ec->Class<AZ::ScriptPropertyGenericClassArray>("Script Property Array(object)", "A script object array property")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "ScriptPropertyGenericClassArray's class attributes.")->
                            Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)->
                            Attribute(AZ::Edit::Attributes::DynamicElementType, &AZ::ScriptPropertyGenericClassArray::GetElementTypeUuid)->
                        DataElement(nullptr, &AZ::ScriptPropertyGenericClassArray::m_values, "m_value", "An object")->
                            ElementAttribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)->
                            Attribute(AZ::Edit::Attributes::NameLabelOverride, &AZ::ScriptProperty::m_name);


                }
            }
        }
    } // namespace Component
} // namespace AzToolsFramework
