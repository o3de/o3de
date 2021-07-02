/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ACETypes.h>
#include <AzCore/std/string/string_view.h>

#include <IAudioConnection.h>
#include <IAudioSystemControl.h>

#include <ISystem.h>
#include <IXml.h>

namespace AudioControls
{
    class CATLControlsModel;

    //-------------------------------------------------------------------------------------------//
    struct SRawConnectionData
    {
        SRawConnectionData(XmlNodeRef node, bool isValid)
            : m_xmlNode(node)
            , m_isValid(isValid)
        {}

        XmlNodeRef m_xmlNode;

        // indicates if the connection is valid for the currently loaded middleware
        bool m_isValid;
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
