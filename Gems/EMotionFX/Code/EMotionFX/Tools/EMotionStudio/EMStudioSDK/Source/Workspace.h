/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/RTTI/RTTI.h>
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Endian.h>
#include <MCore/Source/CommandGroup.h>
#include "EMStudioConfig.h"
#endif


namespace EMStudio
{
    class EMSTUDIO_API Workspace
    {
        MCORE_MEMORYOBJECTCATEGORY(Workspace, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        AZ_RTTI(Workspace, "{D6572E20-C504-426A-88FF-5D2AEA830BB2}")
        Workspace();
        virtual ~Workspace();

        bool Save(const char* filename = nullptr, bool updateFileName = true, bool updateDirtyFlag = true);
        bool Load(const char* filename, MCore::CommandGroup* commandGroup);

        static const char* GetFileExtension()                               { return ".emfxworkspace"; }

        void Reset();

        void SetFilename(const char* filename)                              { m_filename = filename; m_dirtyFlag = true; }
        const AZStd::string& GetFilenameString() const                      { return m_filename; }
        const char* GetFilename() const                                     { return m_filename.c_str(); }

        /**
         * Set the dirty flag which indicates whether the user has made changes to the motion. This indicator should be set to true
         * when the user changed something like adding a motion event. When the user saves the motion, the indicator is usually set to false.
         * @param dirty The dirty flag.
         */
        void SetDirtyFlag(bool dirty);

        /**
         * Get the dirty flag which indicates whether the user has made changes to the motion. This indicator is set to true
         * when the user changed something like adding a motion event. When the user saves the motion, the indicator is usually set to false.
         * @return The dirty flag.
         */
        bool GetDirtyFlag() const;

    private:
        void AddFile(AZStd::string* inOutCommands, const char* command, const AZStd::string& filename, const char* additionalParameters = nullptr) const;
        bool SaveToFile(const char* filename) const;

        AZStd::string   m_filename;
        bool            m_dirtyFlag;
    };
} // namespace EMStudio
