/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_GAMERESOURCESEXPORTER_H
#define CRYINCLUDE_EDITOR_GAMERESOURCESEXPORTER_H
#pragma once

class CUsedResources;
class CVarBlock;
struct IVariable;

/*! Implements exporting of all loaded resources to specified directory.
 *
 */
class CGameResourcesExporter
{
public:
    void ChooseDirectoryAndSave();
    void ChooseDirectory();
    void Save(const QString& outputDirectory);

    void GatherAllLoadedResources();
    void SetUsedResources(CUsedResources& resources);

private:
    typedef QStringList Files;
    static Files m_files;

    QString m_path;

    //////////////////////////////////////////////////////////////////////////
    // Functions that gather files from editor subsystems.
    //////////////////////////////////////////////////////////////////////////
    void GetFilesFromVarBlock(CVarBlock* pVB);
    void GetFilesFromVariable(IVariable* pVar);
};

#endif // CRYINCLUDE_EDITOR_GAMERESOURCESEXPORTER_H

