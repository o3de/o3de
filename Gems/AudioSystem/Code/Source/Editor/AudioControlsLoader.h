/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
