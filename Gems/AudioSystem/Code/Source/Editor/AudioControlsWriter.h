/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/string/string_view.h>

#include <ACETypes.h>
#include <ATLCommon.h>
#include <AudioControl.h>

#include <QModelIndex>


class QStandardItemModel;

namespace AudioControls
{
    class CATLControlsModel;
    class IAudioSystemEditor;

    //-------------------------------------------------------------------------------------------//
    struct SLibraryScope
    {
        SLibraryScope()
        {
            XmlAllocator& xmlAlloc(AudioControls::s_xmlAllocator);
            m_nodes[eACET_TRIGGER] = xmlAlloc.allocate_node(AZ::rapidxml::node_element, Audio::ATLXmlTags::TriggersNodeTag);
            m_nodes[eACET_RTPC] = xmlAlloc.allocate_node(AZ::rapidxml::node_element, Audio::ATLXmlTags::RtpcsNodeTag);
            m_nodes[eACET_SWITCH] = xmlAlloc.allocate_node(AZ::rapidxml::node_element, Audio::ATLXmlTags::SwitchesNodeTag);
            m_nodes[eACET_SWITCH_STATE] = nullptr;
            m_nodes[eACET_ENVIRONMENT] = xmlAlloc.allocate_node(AZ::rapidxml::node_element, Audio::ATLXmlTags::EnvironmentsNodeTag);
            m_nodes[eACET_PRELOAD] = xmlAlloc.allocate_node(AZ::rapidxml::node_element, Audio::ATLXmlTags::PreloadsNodeTag);
        }

        AZ::rapidxml::xml_node<char>* m_nodes[eACET_NUM_TYPES];
        bool m_isDirty = false;
    };

    using TLibraryStorage = AZStd::map<AZStd::string, SLibraryScope>;

    //-------------------------------------------------------------------------------------------//
    class CAudioControlsWriter
    {
    public:
        CAudioControlsWriter(CATLControlsModel* atlModel, QStandardItemModel* layoutModel, IAudioSystemEditor* audioSystemImpl, FilepathSet& previousLibraryPaths);

    private:
        void WriteLibrary(const AZStd::string_view libraryName, QModelIndex root);
        void WriteItem(QModelIndex index, const AZStd::string& path, TLibraryStorage& library, bool isParentModified);
        void WriteControlToXml(AZ::rapidxml::xml_node<char>* node, CATLControl* control, const AZStd::string_view path);
        void WriteConnectionsToXml(AZ::rapidxml::xml_node<char>* node, CATLControl* control);
        bool IsItemModified(QModelIndex index);

        bool WriteXmlToFile(const AZStd::string_view filepath, AZ::rapidxml::xml_node<char>* rootNode);
        void CheckOutFile(const AZStd::string_view filepath);
        void DeleteLibraryFile(const AZStd::string_view filepath);

        CATLControlsModel* m_atlModel;
        QStandardItemModel* m_layoutModel;
        IAudioSystemEditor* m_audioSystemImpl;

        FilepathSet m_foundLibraryPaths;
    };

} // namespace AudioControls
