/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <AzCore/std/containers/set.h>

#include <ACETypes.h>
#include <AudioControl.h>

#include <IXml.h>

#include <QString>

class QStandardItemModel;
class QStandardItem;

namespace AudioControls
{
    class CATLControlsModel;
    class IAudioSystemEditor;

    //-------------------------------------------------------------------------------------------//
    class CAudioControlsLoader
    {
    public:
        CAudioControlsLoader(CATLControlsModel* atlControlsModel, QStandardItemModel* layoutModel, IAudioSystemEditor* audioSystemImpl);
        const FilepathSet& GetLoadedFilenamesList();
        void LoadAll();
        void LoadControls();
        void LoadScopes();

    private:
        void LoadAllLibrariesInFolder(const AZStd::string_view folderPath, const AZStd::string_view level);
        void LoadControlsLibrary(XmlNodeRef rootNode, const AZStd::string_view filePath, const AZStd::string_view level, const AZStd::string_view fileName);
        CATLControl* LoadControl(XmlNodeRef node, QStandardItem* folderItem, const AZStd::string_view scope);

        void LoadPreloadConnections(XmlNodeRef node, CATLControl* control);
        void LoadConnections(XmlNodeRef rootNode, CATLControl* control);

        void CreateDefaultControls();
        QStandardItem* AddControl(CATLControl* control, QStandardItem* folderItem);

        void LoadScopesImpl(const AZStd::string_view levelsFolder);

        QStandardItem* AddUniqueFolderPath(QStandardItem* parentItem, const QString& path);
        QStandardItem* AddFolder(QStandardItem* parentItem, const QString& name);

        CATLControl* CreateInternalSwitchState(CATLControl* parentControl, const AZStd::string& switchName, const AZStd::string& stateName);

        CATLControlsModel* m_atlControlsModel;
        QStandardItemModel* m_layoutModel;
        IAudioSystemEditor* m_audioSystemImpl;
        FilepathSet m_loadedFilenames;
    };

} // namespace AudioControls
