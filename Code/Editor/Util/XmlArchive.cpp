/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "XmlArchive.h"

// Editor
#include "PakFile.h"

//////////////////////////////////////////////////////////////////////////
// CXmlArchive

//////////////////////////////////////////////////////////////////////////
bool CXmlArchive::SaveToPak([[maybe_unused]] const QString& levelPath, CPakFile& pakFile)
{
    _smart_ptr<IXmlStringData> pXmlStrData = root->getXMLData(5000000);

    // Save xml file.
    QString xmlFilename = "level.editor_xml";
    pakFile.UpdateFile(xmlFilename.toUtf8().data(), (void*)pXmlStrData->GetString(), static_cast<int>(pXmlStrData->GetStringLength()));

    if (pakFile.GetArchive())
    {
        CLogFile::FormatLine("Saving pak file %.*s", AZ_STRING_ARG(pakFile.GetArchive()->GetFullPath().Native()));
    }

    pNamedData->Save(pakFile);
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CXmlArchive::LoadFromPak(const QString& levelPath, CPakFile& pakFile)
{
    QString xmlFilename = QDir(levelPath).absoluteFilePath("level.editor_xml");
    root = XmlHelpers::LoadXmlFromFile(xmlFilename.toUtf8().data());
    if (!root)
    {
        return false;
    }

    return pNamedData->Load(levelPath, pakFile);
}
