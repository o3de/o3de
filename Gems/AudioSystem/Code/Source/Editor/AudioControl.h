/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ACETypes.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/XML/rapidxml.h>

#include <IAudioConnection.h>
#include <IAudioSystemControl.h>

namespace AudioControls
{
    class CATLControlsModel;

    //-------------------------------------------------------------------------------------------//
    struct SRawConnectionData
    {
        SRawConnectionData(AZ::rapidxml::xml_node<char>* node, bool isValid)
        {
            m_xmlNode = DeepCopyNode(node);
            m_isValid = isValid;
        }

        AZ::rapidxml::xml_node<char>* m_xmlNode = nullptr;

        // indicates if the connection is valid for the currently loaded middleware
        bool m_isValid{ false };

    private:
        // Rapid XML provides a 'clone_node' utility that will copy an entire node tree,
        // but it only copies pointers of any strings in the node names and values.
        // This causes problems with storage of xml trees, as this class does, because strings
        // will be pointing into an xml document's file buffer that has gone out of scope.
        // This function is a rewritten version of 'clone_node' that does the deep copy of strings
        // into the new destination tree.
        [[nodiscard]] AZ::rapidxml::xml_node<char>* DeepCopyNode(AZ::rapidxml::xml_node<char>* srcNode)
        {
            if (!srcNode)
            {
                return nullptr;
            }

            XmlAllocator& xmlAlloc(AudioControls::s_xmlAllocator);
            AZ::rapidxml::xml_node<char>* destNode = xmlAlloc.allocate_node(srcNode->type());

            destNode->name(xmlAlloc.allocate_string(srcNode->name(), srcNode->name_size()), srcNode->name_size());
            destNode->value(xmlAlloc.allocate_string(srcNode->value(), srcNode->value_size()), srcNode->value_size());

            for (AZ::rapidxml::xml_node<char>* child = srcNode->first_node(); child != nullptr; child = child->next_sibling())
            {
                destNode->append_node(DeepCopyNode(child));
            }

            for (AZ::rapidxml::xml_attribute<char>* attr = srcNode->first_attribute(); attr != nullptr; attr = attr->next_attribute())
            {
                destNode->append_attribute(xmlAlloc.allocate_attribute(
                    xmlAlloc.allocate_string(attr->name(), attr->name_size()),
                    xmlAlloc.allocate_string(attr->value(), attr->value_size()),
                    attr->name_size(),
                    attr->value_size()
                ));
            }

            return destNode;
        }
    };

    using TXmlNodeList = AZStd::vector<SRawConnectionData>;

    //-------------------------------------------------------------------------------------------//
    class CATLControl
    {
        friend class CAudioControlsLoader;
        friend class CAudioControlsWriter;
        friend class CUndoControlModified;

    public:
        CATLControl() = default;
        CATLControl(const AZStd::string& controlName, CID id, EACEControlType type, CATLControlsModel* atlControlsModel);
        ~CATLControl();

        CID GetId() const;
        AZStd::string GetName() const;
        CATLControl* GetParent() const;
        AZStd::string GetScope() const;
        EACEControlType GetType() const;

        bool HasScope() const;
        bool IsAutoLoad() const;

        void SetAutoLoad(bool isAutoLoad);
        void SetName(const AZStd::string_view name);
        void SetParent(CATLControl* parent);
        void SetScope(const AZStd::string_view scope);

        size_t ChildCount() const
        {
            return m_children.size();
        }

        CATLControl* GetChild(size_t index) const
        {
            return (index < m_children.size() ? m_children[index] : nullptr);
        }

        void AddChild(CATLControl* childControl)
        {
            if (childControl)
            {
                m_children.push_back(childControl);
            }
        }

        void RemoveChild(CATLControl* childControl)
        {
            if (childControl)
            {
                m_children.erase(AZStd::remove(m_children.begin(), m_children.end(), childControl), m_children.end());
            }
        }

        size_t ConnectionCount() const;
        TConnectionPtr GetConnectionAt(size_t index) const;
        TConnectionPtr GetConnection(CID id) const;
        TConnectionPtr GetConnection(IAudioSystemControl* middlewareControl) const;

        void AddConnection(TConnectionPtr connection);
        void RemoveConnection(TConnectionPtr connection);
        void RemoveConnection(IAudioSystemControl* middlewareControl);
        void ClearConnections();
        void ReloadConnections();
        bool IsFullyConnected() const;

        void SignalControlAboutToBeModified();
        void SignalControlModified();
        void SignalConnectionAdded(IAudioSystemControl* middlewareControl);
        void SignalConnectionRemoved(IAudioSystemControl* middlewareControl);

        bool SwitchStateConnectionCheck(IAudioSystemControl* middlewareControl);

    private:
        void SetId(CID id);
        void SetType(EACEControlType type);

        CID m_id = ACE_INVALID_CID;
        EACEControlType m_type = eACET_TRIGGER;
        AZStd::string m_name;
        AZStd::string m_scope;

        AZStd::vector<TConnectionPtr> m_connectedControls;
        AZStd::vector<CATLControl*> m_children;

        CATLControlsModel* m_atlControlsModel = nullptr;
        CATLControl* m_parent = nullptr;
        bool m_isAutoLoad = true;

        // All the raw connection nodes. Used for reloading the data when switching middleware.
        TXmlNodeList m_connectionNodes;
    };

} // namespace AudioControls
