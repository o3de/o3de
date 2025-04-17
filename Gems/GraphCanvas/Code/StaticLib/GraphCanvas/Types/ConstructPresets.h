/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QPixmap>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Types/Types.h>

namespace GraphCanvas
{
    // Generic Preset. Will use the container to gate the application process.
    class ConstructPreset
    {
        friend class ConstructTypePresetBucket;
    public:
        AZ_RTTI(ConstructPreset, "{91FAF8E9-6EE9-4C75-B33F-5BA3B4A2066E}");
        AZ_CLASS_ALLOCATOR(ConstructPreset, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext);

        ConstructPreset() = default;
        virtual ~ConstructPreset() = default;
        
        virtual bool IsValidEntityForPreset(const AZ::EntityId& entityId) const = 0;
        void ApplyPreset(const AZ::EntityId& entityId) const;        

        const EntitySaveDataContainer& GetPresetData() const;

        void SetDisplayName(AZStd::string_view displayName);
        const AZStd::string& GetDisplayName() const;

        virtual QPixmap* GetDisplayIcon(const EditorId& editorId) const = 0;
        
    private:        

        AZStd::string           m_displayName;
        EntitySaveDataContainer m_dataPreset;
    };

    /////////////////
    // Preset Types
    /////////////////

    class CommentPreset
        : public ConstructPreset
    {
    public:
        AZ_RTTI(CommentPreset, "{7C2F56BB-6515-4F0F-BBDA-C4C6DBF2453F}", ConstructPreset);
        AZ_CLASS_ALLOCATOR(CommentPreset, AZ::SystemAllocator);

        CommentPreset() = default;
        ~CommentPreset() override = default;

        bool IsValidEntityForPreset(const AZ::EntityId& entityId) const override;
        QPixmap* GetDisplayIcon(const EditorId& editorId) const override;
    };

    class NodeGroupPreset
        : public ConstructPreset
    {
    public:
        AZ_RTTI(NodeGroupPreset, "{15958013-653B-4CA2-8FC9-5DEC48FFDFA4}", ConstructPreset);
        AZ_CLASS_ALLOCATOR(NodeGroupPreset, AZ::SystemAllocator);

        NodeGroupPreset() = default;
        ~NodeGroupPreset() override = default;

        bool IsValidEntityForPreset(const AZ::EntityId& entityId) const override;
        QPixmap* GetDisplayIcon(const EditorId& editorId) const override;
    };

    //////////////////////////
    // General Preset Bucket
    //////////////////////////

    class ConstructTypePresetBucket
    {
        friend class EditorConstructPresets;

    protected:
        ConstructTypePresetBucket();

    public:
        AZ_RTTI(ConstructTypePresetBucket, "{D6C8F55A-E0C3-4460-8BE2-773AB5BE6F2D}");
        AZ_CLASS_ALLOCATOR(ConstructTypePresetBucket, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        virtual ~ConstructTypePresetBucket() = default;

        virtual ConstructType GetConstructType() const = 0;

        void SetDefaultPreset(int index);
        int GetDefaultPresetIndex() const;

        int GetPresetCount() const;

        const AZStd::vector< AZStd::shared_ptr<ConstructPreset> >& GetPresets() const;
        AZStd::shared_ptr<ConstructPreset> FindPreset(int index) const;
        AZStd::shared_ptr<ConstructPreset> GetDefaultPreset() const;        

        AZStd::shared_ptr<ConstructPreset> CreateNewPreset(AZStd::string_view displayName = "");
        bool CreatePresetFrom(const AZ::EntityId& elementId, AZStd::string_view displayName = "");

        void ClearPresets();

        void RemovePreset(int preset);
        bool RemovePreset(AZStd::shared_ptr< ConstructPreset > preset);

        void SetEditorId(const EditorId& editorId);

    protected:        

        void AddPreset(ConstructPreset* presets);
        void ApplyPreset(const AZ::EntityId& entityId, int index);

        virtual AZStd::string GetDefaultName() const;
        virtual ConstructPreset* CreateEmptyPreset() = 0;

        virtual void ConfigurePresetDefaults(EntitySaveDataContainer* presetData) = 0;

        virtual void ConfigureAllowableSaveTypes(AZStd::unordered_set< AZ::Uuid >& allowableSaveTypes) = 0;
        virtual void DeconfigurePresetsFromEntity(EntitySaveDataContainer* preset) = 0;

    private:

        EditorId m_editorId;
        int m_defaultPreset;
        AZStd::vector< AZStd::shared_ptr<ConstructPreset> > m_presets;
    };

    class CommentPresetBucket
        : public ConstructTypePresetBucket
    {
    public:
        AZ_RTTI(CommentPresetBucket, "{B0765DB2-1EC0-4C18-810F-2F01AF9E4984}", ConstructTypePresetBucket);
        AZ_CLASS_ALLOCATOR(CommentPresetBucket, AZ::SystemAllocator);

        CommentPresetBucket() = default;
        ~CommentPresetBucket() override = default;

        ConstructType GetConstructType() const override;

    protected:

        AZStd::string GetDefaultName() const override;
        ConstructPreset* CreateEmptyPreset() override;        

        void ConfigurePresetDefaults(EntitySaveDataContainer* presetData) override;

        void ConfigureAllowableSaveTypes(AZStd::unordered_set< AZ::Uuid >& allowableSaveTypes) override;
        void DeconfigurePresetsFromEntity(EntitySaveDataContainer* preset) override;
    };

    class NodeGroupPresetBucket
        : public ConstructTypePresetBucket
    {
    public:
        AZ_RTTI(NodeGroupPresetBucket, "{1ED223B3-7E1F-418E-9DBF-EB345FCD1333}", ConstructTypePresetBucket);
        AZ_CLASS_ALLOCATOR(NodeGroupPresetBucket, AZ::SystemAllocator);

        NodeGroupPresetBucket() = default;
        ~NodeGroupPresetBucket() override = default;

        ConstructType GetConstructType() const override;

    protected:

        AZStd::string GetDefaultName() const override;
        ConstructPreset* CreateEmptyPreset() override;
        
        void ConfigurePresetDefaults(EntitySaveDataContainer* presetData) override;

        void ConfigureAllowableSaveTypes(AZStd::unordered_set< AZ::Uuid >& allowableSaveTypes) override;
        void DeconfigurePresetsFromEntity(EntitySaveDataContainer* preset) override;
    };

    // TODO: Make this into an asset.
    class EditorConstructPresets
    {
    public:

        AZ_CLASS_ALLOCATOR(EditorConstructPresets, AZ::SystemAllocator);
        AZ_RTTI(EditorConstructPresets, "{3A975CAB-5CD8-4496-8B1F-092952B37953}");

        static void Reflect(AZ::ReflectContext* reflectContext);

        EditorConstructPresets();
        virtual ~EditorConstructPresets();

        void SetEditorId(EditorId editorId);

        bool IsEmpty() const;
        void Initialize();

        template<class PresetBucket>
        void RegisterPresetBucket()
        {
            AZStd::shared_ptr<ConstructTypePresetBucket> container = AZStd::make_shared<PresetBucket>();

            ConstructType constructType = container->GetConstructType();
            auto presetIter = m_presetMapping.find(container->GetConstructType());

            if (presetIter == m_presetMapping.end())
            {
                container->CreateNewPreset(container->GetDefaultName());
                container->SetDefaultPreset(0);

                m_presetMapping[constructType] = container;
            }
        }

        void CreatePresetFrom(const AZ::EntityId& entityId, AZStd::string_view displayName);
        void RemovePresets(const AZStd::vector< AZStd::shared_ptr< ConstructPreset> >& presets);

        const ConstructTypePresetBucket* FindPresetBucket(ConstructType constructType) const;

        void SetDefaultPreset(ConstructType constructType, int presetIndex);

        virtual void InitializeConstructType(ConstructType constructType);

    protected:

        ConstructTypePresetBucket* ModPresetBucket(ConstructType presets);

    private:

        EditorId                                                         m_editorId;
        AZStd::unordered_map< ConstructType, AZStd::shared_ptr<ConstructTypePresetBucket> > m_presetMapping;
    };
}
