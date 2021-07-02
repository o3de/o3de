/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/Workspace.h>
#include "EMStudioConfig.h"

#include <QString>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTableWidget>
#endif


namespace EMStudio
{
    class EMSTUDIO_API SaveDirtyFilesCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(SaveDirtyFilesCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        struct ObjectPointer
        {
            MCORE_MEMORYOBJECTCATEGORY(SaveDirtyFilesCallback::ObjectPointer, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

            ObjectPointer();

            EMotionFX::Actor *      mActor;
            EMotionFX::Motion*      mMotion;
            EMotionFX::MotionSet*   mMotionSet;
            EMotionFX::AnimGraph*   mAnimGraph;
            Workspace*              mWorkspace;
        };

        SaveDirtyFilesCallback();
        virtual ~SaveDirtyFilesCallback();

        virtual int SaveDirtyFiles(const AZStd::vector<AZStd::string>& filenamesToSave, const AZStd::vector<ObjectPointer>& objects, MCore::CommandGroup* commandGroup) = 0;
        virtual void GetDirtyFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects) = 0;
        virtual const char* GetExtension() const = 0;
        virtual const char* GetFileType() const = 0;
        virtual uint32 GetType() const = 0;
        virtual const AZ::Uuid GetFileRttiType() const = 0;
        virtual uint32 GetPriority() const = 0;
        virtual bool GetIsPostProcessed() const = 0;
    };


    class EMSTUDIO_API DirtyFileManager
    {
        MCORE_MEMORYOBJECTCATEGORY(DirtyFileManager, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        enum SaveDirtyFilesResult
        {
            FAILED,
            FINISHED,
            NOFILESTOSAVE,
            CANCELED
        };

        DirtyFileManager();
        ~DirtyFileManager();

        // dirty files callbacks
        void AddCallback(SaveDirtyFilesCallback* callback);
        void RemoveCallback(SaveDirtyFilesCallback* callback, bool delFromMem = true);
        SaveDirtyFilesCallback* GetCallback(size_t index) const     { return mSaveDirtyFilesCallbacks[index]; }
        size_t GetNumCallbacks() const                              { return mSaveDirtyFilesCallbacks.size(); }

        int SaveDirtyFiles(uint32 type = MCORE_INVALIDINDEX32, uint32 filter = MCORE_INVALIDINDEX32,
                QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Discard | QDialogButtonBox::Cancel
        );

        int SaveDirtyFiles(const AZStd::vector<AZ::TypeId>& typeIds, 
                QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Discard | QDialogButtonBox::Cancel
        );

        void SaveSettings();

    private:
        AZStd::vector<SaveDirtyFilesCallback*>  mSaveDirtyFilesCallbacks;

        int SaveDirtyFiles(const AZStd::vector<SaveDirtyFilesCallback*>& neededSaveDirtyFilesCallbacks, QDialogButtonBox::StandardButtons buttons);
    };


    class EMSTUDIO_API SaveDirtySettingsWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(SaveDirtySettingsWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        SaveDirtySettingsWindow(
                QWidget* parent,
                const AZStd::vector<AZStd::string>& dirtyFileNames,
                const AZStd::vector<SaveDirtyFilesCallback::ObjectPointer>& objects,
                QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Discard | QDialogButtonBox::Cancel
        );
        virtual ~SaveDirtySettingsWindow();

        bool GetSaveDirtyFiles()            { return mSaveDirtyFiles; }
        void GetSelectedFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<SaveDirtyFilesCallback::ObjectPointer>* outObjects);

    public slots:
        void OnSaveButton()                 { mSaveDirtyFiles = true;   emit accept(); }
        void OnSkipSavingButton()           { mSaveDirtyFiles = false;  emit accept(); }
        void OnCancelButton()               { mSaveDirtyFiles = false;  emit reject(); }

    private:
        QTableWidget*                                           mTableWidget;
        bool                                                    mSaveDirtyFiles;
        AZStd::vector<AZStd::string>                            mFileNames;
        AZStd::vector<SaveDirtyFilesCallback::ObjectPointer>    mObjects;
    };
} // namespace EMStudio
