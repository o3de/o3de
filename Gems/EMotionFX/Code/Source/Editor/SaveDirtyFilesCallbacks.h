/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/SaveChangedFilesManager.h>

namespace EMStudio
{
    class SaveDirtyActorFilesCallback
        : public SaveDirtyFilesCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(SaveDirtyActorFilesCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        enum
        {
            TYPE_ID = 0x00000001
        };

    public:
        SaveDirtyActorFilesCallback() = default;
        ~SaveDirtyActorFilesCallback() = default;

        uint32 GetType() const override { return TYPE_ID; }
        uint32 GetPriority() const override { return 4; }
        bool GetIsPostProcessed() const override { return false; }

        const char* GetExtension() const override { return "actor"; }
        const char* GetFileType() const override { return "actor"; }
        const AZ::Uuid GetFileRttiType() const override { return azrtti_typeid<EMotionFX::Actor>(); }

        void GetDirtyFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects) override;
        int SaveDirtyFiles(const AZStd::vector<AZStd::string>& filenamesToSave, const AZStd::vector<ObjectPointer>& objects, MCore::CommandGroup* commandGroup) override;

        static int SaveDirtyActor(EMotionFX::Actor* actor, MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton = true);
    };

    class SaveDirtyMotionFilesCallback
        : public SaveDirtyFilesCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(SaveDirtyMotionFilesCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        SaveDirtyMotionFilesCallback() = default;
        virtual ~SaveDirtyMotionFilesCallback() = default;

        enum
        {
            TYPE_ID = 0x00000002
        };
        uint32 GetType() const override { return TYPE_ID; }
        uint32 GetPriority() const override { return 3; }
        bool GetIsPostProcessed() const override { return false; }

        const char* GetExtension() const override { return "motion"; }
        const char* GetFileType() const override { return "motion"; }
        const AZ::Uuid GetFileRttiType() const override { return azrtti_typeid<EMotionFX::Motion>(); }

        void GetDirtyFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects) override;
        int SaveDirtyFiles(const AZStd::vector<AZStd::string>& filenamesToSave, const AZStd::vector<ObjectPointer>& objects, MCore::CommandGroup* commandGroup) override;

        static int SaveDirtyMotion(EMotionFX::Motion* motion, [[maybe_unused]] MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton = true);
    };

    class SaveDirtyMotionSetFilesCallback
        : public SaveDirtyFilesCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(SaveDirtyMotionSetFilesCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        SaveDirtyMotionSetFilesCallback() = default;
        virtual ~SaveDirtyMotionSetFilesCallback() = default;

        enum
        {
            TYPE_ID = 0x00000003
        };
        uint32 GetType() const override { return TYPE_ID; }
        uint32 GetPriority() const override { return 2; }
        bool GetIsPostProcessed() const override { return false; }

        const char* GetExtension() const override { return "motionset"; }
        const char* GetFileType() const override { return "motion set"; }
        const AZ::Uuid GetFileRttiType() const override { return azrtti_typeid<EMotionFX::MotionSet>(); }

        void GetDirtyFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects) override;
        int SaveDirtyFiles(const AZStd::vector<AZStd::string>& filenamesToSave, const AZStd::vector<ObjectPointer>& objects, MCore::CommandGroup* commandGroup) override;

        static int SaveDirtyMotionSet(EMotionFX::MotionSet* motionSet, MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton = true);
    };

    class SaveDirtyAnimGraphFilesCallback
        : public SaveDirtyFilesCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(SaveDirtyFilesCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        SaveDirtyAnimGraphFilesCallback() = default;
        ~SaveDirtyAnimGraphFilesCallback() = default;

        enum
        {
            TYPE_ID = 0x00000004
        };
        uint32 GetType() const override { return TYPE_ID; }
        uint32 GetPriority() const override { return 1; }
        bool GetIsPostProcessed() const override { return false; }

        const char* GetExtension() const override { return "animgraph"; }
        const char* GetFileType() const override { return "anim graph"; }
        const AZ::Uuid GetFileRttiType() const override { return azrtti_typeid<EMotionFX::AnimGraph>(); }

        void GetDirtyFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects) override;
        int SaveDirtyFiles(const AZStd::vector<AZStd::string>& filenamesToSave, const AZStd::vector<ObjectPointer>& objects, MCore::CommandGroup* commandGroup) override;

        int SaveDirtyAnimGraph(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton = true);
    };

    class SaveDirtyWorkspaceCallback
        : public SaveDirtyFilesCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(SaveDirtyWorkspaceCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        SaveDirtyWorkspaceCallback() = default;
        ~SaveDirtyWorkspaceCallback() = default;

        enum
        {
            TYPE_ID = 0x00000005
        };
        uint32 GetType() const override { return TYPE_ID; }
        uint32 GetPriority() const override { return 0; }
        bool GetIsPostProcessed() const override { return false; }

        const char* GetExtension() const override { return "emfxworkspace"; }
        const char* GetFileType() const override { return "workspace"; }
        const AZ::Uuid GetFileRttiType() const override { return azrtti_typeid<EMStudio::Workspace>(); }

        void GetDirtyFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects) override;
        int SaveDirtyFiles(const AZStd::vector<AZStd::string>& filenamesToSave, const AZStd::vector<ObjectPointer>& objects, MCore::CommandGroup* commandGroup) override;
    };
} // namespace EMStudio
