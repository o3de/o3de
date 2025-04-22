/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GraphCanvas/Types/ConstructPresets.h>

#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Utils/ConversionUtils.h>
#include <GraphCanvas/Utils/GraphUtils.h>

#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>

namespace GraphCanvas
{
    ////////////////////
    // ConstructPreset
    ////////////////////

    void ConstructPreset::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<ConstructPreset>()
                ->Version(1)
                ->Field("DisplayName", &ConstructPreset::m_displayName)
                ->Field("Preset", &ConstructPreset::m_dataPreset)
            ;
        }
    }

    void ConstructPreset::ApplyPreset(const AZ::EntityId& entityId) const
    {
        if (IsValidEntityForPreset(entityId))
        {
            EntitySaveDataRequestBus::Event(entityId, &EntitySaveDataRequests::ReadSaveData, m_dataPreset);
        }
    }

    const EntitySaveDataContainer& ConstructPreset::GetPresetData() const
    {
        return m_dataPreset;
    }

    void ConstructPreset::SetDisplayName(AZStd::string_view displayName)
    {
        m_displayName = displayName;
    }

    const AZStd::string& ConstructPreset::GetDisplayName() const
    {
        return m_displayName;
    }

    //////////////////
    // CommentPreset
    //////////////////

    bool CommentPreset::IsValidEntityForPreset(const AZ::EntityId& entityId) const
    {
        return GraphUtils::IsComment(entityId);
    }

    QPixmap* CommentPreset::GetDisplayIcon(const EditorId& editorId) const
    {
        const EntitySaveDataContainer& dataContainer = GetPresetData();

        CommentNodeTextSaveData* saveData = dataContainer.FindSaveDataAs<CommentNodeTextSaveData>();

        QColor color = ConversionUtils::AZToQColor(saveData->m_backgroundColor);

        QPixmap* pixmap = nullptr;
        StyleManagerRequestBus::EventResult(pixmap, editorId, &StyleManagerRequests::CreateIcon, color, "CommentPresetIcon");

        return pixmap;
    }

    ////////////////////
    // NodeGroupPreset
    ////////////////////

    bool NodeGroupPreset::IsValidEntityForPreset(const AZ::EntityId& entityId) const
    {
        return GraphUtils::IsNodeGroup(entityId);
    }

    QPixmap* NodeGroupPreset::GetDisplayIcon(const EditorId& editorId) const
    {
        const EntitySaveDataContainer& dataContainer = GetPresetData();

        CommentNodeTextSaveData* saveData = dataContainer.FindSaveDataAs<CommentNodeTextSaveData>();

        QColor color = ConversionUtils::AZToQColor(saveData->m_backgroundColor);

        QPixmap* pixmap = nullptr;
        StyleManagerRequestBus::EventResult(pixmap, editorId, &StyleManagerRequests::CreateIcon, color, "NodeGroupPresetIcon");

        return pixmap;
    }

    //////////////////////////////
    // ConstructTypePresetBucket
    //////////////////////////////

    void ConstructTypePresetBucket::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<ConstructTypePresetBucket>()
                ->Version(1)
                ->Field("DefaultPreset", &ConstructTypePresetBucket::m_defaultPreset)
                ->Field("Presets", &ConstructTypePresetBucket::m_presets)
            ;
        }
    }

    ConstructTypePresetBucket::ConstructTypePresetBucket()
        : m_defaultPreset(0)
    {

    }

    void ConstructTypePresetBucket::SetDefaultPreset(int index)
    {
        if (index < 0 || index >= GetPresetCount())
        {
            index = 0;
        }

        m_defaultPreset = index;
    }

    int ConstructTypePresetBucket::GetDefaultPresetIndex() const
    {
        return m_defaultPreset;
    }

    int ConstructTypePresetBucket::GetPresetCount() const
    {
        return static_cast<int>(m_presets.size());
    }

    const AZStd::vector< AZStd::shared_ptr<ConstructPreset> >& ConstructTypePresetBucket::GetPresets() const
    {
        return m_presets;
    }

    AZStd::shared_ptr<ConstructPreset> ConstructTypePresetBucket::FindPreset(int index) const
    {
        if (index < 0 || index > m_presets.size())
        {
            return nullptr;
        }

        return m_presets[index];
    }    

    AZStd::shared_ptr<ConstructPreset> ConstructTypePresetBucket::GetDefaultPreset() const
    {
        return FindPreset(m_defaultPreset);
    }

    AZStd::shared_ptr<ConstructPreset> ConstructTypePresetBucket::CreateNewPreset(AZStd::string_view displayName)
    {
        AZStd::shared_ptr<ConstructPreset> retVal = nullptr;

        ConstructPreset* preset = CreateEmptyPreset();

        preset->m_displayName = displayName;

        EntitySaveDataContainer* dataContainer = &preset->m_dataPreset;

        ConfigurePresetDefaults(dataContainer);

        if (!dataContainer->IsEmpty())
        {            
            AddPreset(preset);

            if (!m_presets.empty())
            {
                retVal = m_presets.back();
            }
        }
        else
        {            
            delete preset;
        }

        return retVal;
    }

    bool ConstructTypePresetBucket::CreatePresetFrom(const AZ::EntityId& elementId, AZStd::string_view displayName)
    {
        bool isValid = false;
        ConstructPreset* preset = CreateEmptyPreset();
        preset->m_displayName = displayName;

        if (preset->IsValidEntityForPreset(elementId))
        {            
            EntitySaveDataContainer* dataContainer = &preset->m_dataPreset;

            EntitySaveDataRequestBus::Event(elementId, &EntitySaveDataRequests::WriteSaveData, (*dataContainer));

            AZStd::unordered_set< AZ::Uuid > allowablePresetTypes;

            ConfigureAllowableSaveTypes(allowablePresetTypes);

            dataContainer->RemoveAll(allowablePresetTypes);

            if (!dataContainer->IsEmpty())
            {            
                isValid = true;

                DeconfigurePresetsFromEntity(dataContainer);

                AddPreset(preset);
            }
        }

        if (!isValid)
        {
            delete preset;
        }

        return isValid;
    }

    void ConstructTypePresetBucket::ClearPresets()
    {
        m_defaultPreset = 0;
        m_presets.clear();

        CreateNewPreset(GetDefaultName());
    }

    void ConstructTypePresetBucket::RemovePreset(int preset)
    {
        if (preset < 0 || preset >= GetPresetCount())
        {
            return;
        }

        if (m_defaultPreset > preset)
        {
            m_defaultPreset -= 1;
        }
        else if (m_defaultPreset == preset)
        {
            m_defaultPreset = 0;
        }

        m_presets.erase(m_presets.begin() + preset);

        if (m_presets.empty())
        {
            CreateNewPreset(GetDefaultName());
        }
    }

    bool ConstructTypePresetBucket::RemovePreset(AZStd::shared_ptr< ConstructPreset > preset)
    {
        bool removedPreset = false;
        for (int i=0; i < static_cast<int>(m_presets.size()); ++i)
        {
            if (m_presets[i] == preset)
            {
                removedPreset = true;
                RemovePreset(i);
                break;
            }
        }

        return removedPreset;
    }

    void ConstructTypePresetBucket::SetEditorId(const EditorId& editorId)
    {
        m_editorId = editorId;
    }

    void ConstructTypePresetBucket::AddPreset(ConstructPreset* preset)
    {
        m_presets.emplace_back(preset);
    }

    void ConstructTypePresetBucket::ApplyPreset(const AZ::EntityId& entityId, int index)
    {
        AZStd::shared_ptr<ConstructPreset> preset = FindPreset(index);

        if (preset)
        {
            preset->ApplyPreset(entityId);
        }
    }

    AZStd::string ConstructTypePresetBucket::GetDefaultName() const
    {
        return "Base";
    }

    ////////////////////////
    // CommentPresetBucket
    ////////////////////////

    ConstructType CommentPresetBucket::GetConstructType() const
    {
        return ConstructType::CommentNode;
    }

    AZStd::string CommentPresetBucket::GetDefaultName() const
    {
        return "Note";
    }

    ConstructPreset* CommentPresetBucket::CreateEmptyPreset()
    {
        return aznew CommentPreset();
    }   

    void CommentPresetBucket::ConfigurePresetDefaults(EntitySaveDataContainer* presetData)
    {
        CommentNodeTextSaveData* saveData = presetData->FindCreateSaveData<CommentNodeTextSaveData>();
        saveData->m_fontConfiguration.InitializePixelSize();
        saveData->m_backgroundColor = AZ::Color(0.98f, 0.97f, 0.65f, 1.0f);
    }

    void CommentPresetBucket::ConfigureAllowableSaveTypes(AZStd::unordered_set< AZ::Uuid >& allowableSaveTypes)
    {
        allowableSaveTypes.insert(EntitySaveDataContainer::GetDataTypeKey<CommentNodeTextSaveData>());
    }

    void CommentPresetBucket::DeconfigurePresetsFromEntity(EntitySaveDataContainer* presetData)
    {
        CommentNodeTextSaveData* saveData = presetData->FindSaveDataAs<CommentNodeTextSaveData>();

        if (saveData)
        {
            saveData->m_comment = "";
        }
    }

    //////////////////////////
    // NodeGroupPresetBucket
    //////////////////////////

    ConstructType NodeGroupPresetBucket::GetConstructType() const
    {
        return ConstructType::NodeGroup;
    }

    AZStd::string NodeGroupPresetBucket::GetDefaultName() const
    {
        return "General";
    }

    ConstructPreset* NodeGroupPresetBucket::CreateEmptyPreset()
    {
        return aznew NodeGroupPreset();
    }

    void NodeGroupPresetBucket::ConfigurePresetDefaults(EntitySaveDataContainer* presetData)
    {
        auto saveData = presetData->FindCreateSaveData<CommentNodeTextSaveData>();
        saveData->m_fontConfiguration.InitializePixelSize();
        saveData->m_backgroundColor = AZ::Color(0.98f, 0.97f, 0.65f, 1.0f);
    }

    void NodeGroupPresetBucket::ConfigureAllowableSaveTypes(AZStd::unordered_set< AZ::Uuid >& allowableSaveTypes)
    {
        allowableSaveTypes.insert(EntitySaveDataContainer::GetDataTypeKey<CommentNodeTextSaveData>());
    }

    void NodeGroupPresetBucket::DeconfigurePresetsFromEntity(EntitySaveDataContainer* presetData)
    {
        CommentNodeTextSaveData* saveData = presetData->FindSaveDataAs<CommentNodeTextSaveData>();

        if (saveData)
        {
            saveData->m_comment = "";
        }
    }

    ///////////////////////////
    // EditorConstructPresets
    ///////////////////////////

    void EditorConstructPresets::Reflect(AZ::ReflectContext* reflectContext)
    {
        ConstructPreset::Reflect(reflectContext);
        ConstructTypePresetBucket::Reflect(reflectContext);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<EditorConstructPresets>()
                ->Version(1)
                ->Field("PresetMapping", &EditorConstructPresets::m_presetMapping)
            ;

            serializeContext->Class<CommentPreset, ConstructPreset>()
                ->Version(1)
            ;

            serializeContext->Class<NodeGroupPreset, ConstructPreset>()
                ->Version(1)                
            ;

            serializeContext->Class<CommentPresetBucket, ConstructTypePresetBucket>()
                ->Version(1)
            ;

            serializeContext->Class<NodeGroupPresetBucket, ConstructTypePresetBucket>()
                ->Version(1)
            ;
        }
    }

    EditorConstructPresets::EditorConstructPresets()
    {
    }

    EditorConstructPresets::~EditorConstructPresets()
    {
    }

    void EditorConstructPresets::SetEditorId(EditorId editorId)
    {
        if (m_editorId != editorId)
        {
            m_editorId = editorId;

            if (IsEmpty())
            {
                Initialize();
            }

            AssetEditorPresetNotificationBus::Event(m_editorId, &AssetEditorPresetNotifications::OnPresetsChanged);
        }
    }

    void EditorConstructPresets::Initialize()
    {        
        RegisterPresetBucket<CommentPresetBucket>();
        InitializeConstructType(ConstructType::CommentNode);
        
        RegisterPresetBucket<NodeGroupPresetBucket>();
        InitializeConstructType(ConstructType::NodeGroup);
    }

    bool EditorConstructPresets::IsEmpty() const
    {
        return m_presetMapping.empty();
    }

    void EditorConstructPresets::CreatePresetFrom(const AZ::EntityId& entityId, AZStd::string_view displayName)
    {
        for (auto mapPair : m_presetMapping)
        {
            if (mapPair.second->CreatePresetFrom(entityId, displayName))
            {
                AssetEditorPresetNotificationBus::Event(m_editorId, &AssetEditorPresetNotifications::OnConstructPresetsChanged, mapPair.second->GetConstructType());
                break;
            }
        }
    }    

    void EditorConstructPresets::RemovePresets(const AZStd::vector< AZStd::shared_ptr< ConstructPreset> >& presets)
    {
        AZStd::unordered_set< ConstructType > constructTypes;

        for (auto preset : presets)
        {
            for (auto bucketPair : m_presetMapping)
            {
                if (bucketPair.second->RemovePreset(preset))
                {
                    constructTypes.insert(bucketPair.second->GetConstructType());
                    break;
                }
            }
        }

        for (auto constructType : constructTypes)
        {            
            AssetEditorPresetNotificationBus::Event(m_editorId, &AssetEditorPresetNotifications::OnConstructPresetsChanged, constructType);
        }
    }

    const ConstructTypePresetBucket* EditorConstructPresets::FindPresetBucket(ConstructType constructType) const
    {
        auto mapIter = m_presetMapping.find(constructType);

        if (mapIter != m_presetMapping.end())
        {
            return mapIter->second.get();
        }

        return nullptr;
    }

    void EditorConstructPresets::SetDefaultPreset(ConstructType constructType, int presetIndex)
    {
        ConstructTypePresetBucket* typeBucket = ModPresetBucket(constructType);

        if (typeBucket)
        {
            typeBucket->SetDefaultPreset(presetIndex);
        }
    }

    ConstructTypePresetBucket* EditorConstructPresets::ModPresetBucket(ConstructType constructType)
    {
        auto mapIter = m_presetMapping.find(constructType);

        if (mapIter != m_presetMapping.end())
        {
            return mapIter->second.get();
        }

        return nullptr;
    }

    void EditorConstructPresets::InitializeConstructType(ConstructType /*constructType*/)
    {
    }
}
