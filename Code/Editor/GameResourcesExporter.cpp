/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "GameResourcesExporter.h"

// Qt
#include <QFileDialog>

// Editor
#include "UsedResources.h"
#include "GameEngine.h"
#include "WaitProgress.h"


//////////////////////////////////////////////////////////////////////////
// Static data.
//////////////////////////////////////////////////////////////////////////
CGameResourcesExporter::Files CGameResourcesExporter::m_files;

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::ChooseDirectoryAndSave()
{
    ChooseDirectory();
    if (!m_path.isEmpty())
    {
        Save(m_path);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::ChooseDirectory()
{
    m_path = QFileDialog::getExistingDirectory(nullptr, QObject::tr("Choose Target (root/PrimaryCD) Folder"));
}

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::GatherAllLoadedResources()
{
    m_files.clear();
    m_files.reserve(100000);        // count from GetResourceList, GetFilesFromObjects ...  is unknown

    auto pResList = gEnv->pCryPak->GetResourceList(AZ::IO::IArchive::RFOM_Level);
    {
        for (const char* filename = pResList->GetFirst(); filename; filename = pResList->GetNext())
        {
            m_files.push_back(filename);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::SetUsedResources(CUsedResources& resources)
{
    for (CUsedResources::TResourceFiles::const_iterator it = resources.files.begin(); it != resources.files.end(); it++)
    {
        m_files.push_back(*it);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameResourcesExporter::Save(const QString& outputDirectory)
{
    CMemoryBlock data;

    int numFiles = m_files.size();

    CLogFile::WriteLine("===========================================================================");
    CLogFile::FormatLine("Exporting Level %s resources, %d files", GetIEditor()->GetGameEngine()->GetLevelName().toUtf8().data(), numFiles);
    CLogFile::WriteLine("===========================================================================");

    // Needed files.
    CWaitProgress wait("Exporting Resources");
    for (int i = 0; i < numFiles; i++)
    {
        QString srcFilename = m_files[i];
        if (!wait.Step((i * 100) / numFiles))
        {
            break;
        }
        wait.SetText(srcFilename.toUtf8().data());

        CLogFile::WriteLine(srcFilename.toUtf8().data());

        CCryFile file;
        if (file.Open(srcFilename.toUtf8().data(), "rb"))
        {
            // Save this file in target folder.
            QString trgFilename = Path::Make(outputDirectory, srcFilename);
            int fsize = static_cast<int>(file.GetLength());
            if (fsize > data.GetSize())
            {
                data.Allocate(fsize + 16);
            }
            // Read data.
            file.ReadRaw(data.GetBuffer(), fsize);

            // Save this data to target file.
            QString trgFileDir = Path::GetPath(trgFilename);
            CFileUtil::CreateDirectory(trgFileDir.toUtf8().data());
            // Create a file.
            FILE* trgFile = nullptr;
            azfopen(&trgFile, trgFilename.toUtf8().data(), "wb");
            if (trgFile)
            {
                // Save data to new file.
                fwrite(data.GetBuffer(), fsize, 1, trgFile);
                fclose(trgFile);
            }
        }
    }
    CLogFile::WriteLine("===========================================================================");
    m_files.clear();
}
