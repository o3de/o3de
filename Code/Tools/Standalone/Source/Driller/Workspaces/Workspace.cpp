/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/ObjectStream.h>
#include "Workspace.h"

namespace Driller
{
    // this is called when the settings are loaded from file.  Since the root element is just a workspace settings provider object,
    // we verify the cast and return it:
    static void OnObjectLoaded(void* classPtr, const AZ::Uuid& classId, const AZ::SerializeContext* sc, WorkspaceSettingsProvider** target)
    {
        AZ_Assert(classPtr, "classPtr is NULL!");
        AZ_Assert(target, "You must pass the target to OnObjectLoaded");
        WorkspaceSettingsProvider* container = sc->Cast<WorkspaceSettingsProvider*>(classPtr, classId);
        AZ_Assert(container, "Failed to cast classPtr to WorkspaceSettingsProvider!");

        *target = container;
    }

    // remember to delete your setting objects on shutdown, since you own them!
    WorkspaceSettingsProvider::~WorkspaceSettingsProvider()
    {
        for (auto it = m_WorkspaceSaveData.begin(); it != m_WorkspaceSaveData.end(); ++it)
        {
            delete it->second;
        }
    }

    /// Given a filename, attempt to deserialize a WorkspaceSettingsProvider object from it, using the DH objectstream:
    WorkspaceSettingsProvider* WorkspaceSettingsProvider::CreateFromFile(const AZStd::string& filename)
    {
        using namespace AZ;

        SerializeContext* sc = NULL;
        EBUS_EVENT_RESULT(sc, ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(sc, "Can't retrieve application's serialization context!");

        WorkspaceSettingsProvider* resultItem = NULL;
        IO::FileIOStream readStream(filename.c_str(), AZ::IO::OpenMode::ModeRead);
        if (readStream.IsOpen())
        {
            ObjectStream::ClassReadyCB readyCB(AZStd::bind(&OnObjectLoaded, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, &resultItem));
            if (!ObjectStream::LoadBlocking(&readStream, *sc, readyCB))
            {
                AZ_TracePrintf("Driller", "<div severity=\"error\">Failed to deserialize the workspace file: '%s' </div>", filename.c_str());
            }
        }
        else
        {
            AZ_TracePrintf("Driller", "<div severity=\"error\">Failed to open the given filename in order to read it in as a workspace: '%s' </div>", filename.c_str());
        }

        // use the serializer to consume filename.
        return resultItem;
    }

    /// serializes this object into the given filename for later retrieval.
    bool WorkspaceSettingsProvider::WriteToFile(const AZStd::string& filename)
    {
        using namespace AZ;

        SerializeContext* sc = NULL;
        EBUS_EVENT_RESULT(sc, ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(sc, "Can't retrieve application's serialization context!");

        /// note:  Will probably throw an error if it fails.  Should check all these things before we call in here.
        IO::FileIOStream writeStream(filename.c_str(), AZ::IO::OpenMode::ModeWrite);

        if (!writeStream.IsOpen())
        {
            // (will have already called AZ_Error)
            return false;
        }

        ObjectStream* objStream = ObjectStream::Create(&writeStream, *sc, ObjectStream::ST_XML);
        bool writtenOk = objStream->WriteClass(this);
        if (!writtenOk)
        {
            AZ_TracePrintf("Driller", "<div severity=\"error\">Failed to write the workspace object to the workspace file: '%s' </div>", filename.c_str());
            return false;
        }
        bool streamOk = objStream->Finalize();
        if (!streamOk)
        {
            AZ_TracePrintf("Driller", "<div severity=\"error\">Failed to finalize the workspace file: '%s' </div>", filename.c_str());
            return false;
        }

        return true;
    }

    void WorkspaceSettingsProvider::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<WorkspaceSettingsProvider>()
                ->Version(2)
                ->Field("m_WorkspaceSaveData", &WorkspaceSettingsProvider::m_WorkspaceSaveData);
        }
    }
}
