/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioControl.h>

#include <ACETypes.h>
#include <ATLControlsModel.h>
#include <AudioControlsEditorPlugin.h>
#include <AudioControlsEditorUndo.h>
#include <IAudioSystemControl.h>
#include <ImplementationManager.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    CATLControl::CATLControl(const AZStd::string& controlName, CID id, EACEControlType type, CATLControlsModel* atlControlsModel)
        : m_name(controlName)
        , m_id(id)
        , m_type(type)
        , m_atlControlsModel(atlControlsModel)
    {
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl::~CATLControl()
    {
        // Same as ClearConnections but without signalling that 'this' control is being modified...
        if (CAudioControlsEditorPlugin::GetImplementationManager())
        {
            if (IAudioSystemEditor* audioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManager()->GetImplementation())
            {
                for (auto& connectionPtr : m_connectedControls)
                {
                    if (IAudioSystemControl* middlewareControl = audioSystemImpl->GetControl(connectionPtr->GetID()))
                    {
                        audioSystemImpl->ConnectionRemoved(middlewareControl);
                        SignalConnectionRemoved(middlewareControl);
                    }
                }
            }
        }

        m_connectedControls.clear();
    }

    //-------------------------------------------------------------------------------------------//
    CID CATLControl::GetId() const
    {
        return m_id;
    }

    //-------------------------------------------------------------------------------------------//
    EACEControlType CATLControl::GetType() const
    {
        return m_type;
    }

    //-------------------------------------------------------------------------------------------//
    AZStd::string CATLControl::GetName() const
    {
        return m_name;
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* CATLControl::GetParent() const
    {
        return m_parent;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SetId(CID id)
    {
        if (id != m_id)
        {
            SignalControlAboutToBeModified();
            m_id = id;
            SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SetType(EACEControlType type)
    {
        if (type != m_type)
        {
            SignalControlAboutToBeModified();
            m_type = type;
            SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SetName(const AZStd::string_view name)
    {
        if (name != m_name)
        {
            SignalControlAboutToBeModified();
            m_name = name;
            SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    AZStd::string CATLControl::GetScope() const
    {
        return m_scope;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SetScope(const AZStd::string_view scope)
    {
        if (m_scope != scope)
        {
            SignalControlAboutToBeModified();
            m_scope = scope;
            SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    bool CATLControl::HasScope() const
    {
        return !m_scope.empty();
    }

    //-------------------------------------------------------------------------------------------//
    bool CATLControl::IsAutoLoad() const
    {
        return m_isAutoLoad;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SetAutoLoad(bool isAutoLoad)
    {
        if (isAutoLoad != m_isAutoLoad)
        {
            SignalControlAboutToBeModified();
            m_isAutoLoad = isAutoLoad;
            SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    size_t CATLControl::ConnectionCount() const
    {
        return m_connectedControls.size();
    }

    //-------------------------------------------------------------------------------------------//
    TConnectionPtr CATLControl::GetConnectionAt(size_t index) const
    {
        return (index < m_connectedControls.size() ? m_connectedControls[index] : nullptr);
    }

    //-------------------------------------------------------------------------------------------//
    TConnectionPtr CATLControl::GetConnection(CID id) const
    {
        if (id != ACE_INVALID_CID)
        {
            for (const TConnectionPtr& connection : m_connectedControls)
            {
                if (connection && connection->GetID() == id)
                {
                    return connection;
                }
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    TConnectionPtr CATLControl::GetConnection(IAudioSystemControl* middlewareControl) const
    {
        return GetConnection(middlewareControl->GetId());
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::AddConnection(TConnectionPtr connection)
    {
        if (connection)
        {
            SignalControlAboutToBeModified();
            m_connectedControls.push_back(connection);

            if (IAudioSystemEditor* audioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManager()->GetImplementation())
            {
                if (IAudioSystemControl* middlewareControl = audioSystemImpl->GetControl(connection->GetID()))
                {
                    SignalConnectionAdded(middlewareControl);
                }
            }
            SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::RemoveConnection(TConnectionPtr connection)
    {
        if (connection)
        {
            auto it = AZStd::find(m_connectedControls.begin(), m_connectedControls.end(), connection);
            if (it != m_connectedControls.end())
            {
                SignalControlAboutToBeModified();
                m_connectedControls.erase(it);

                if (IAudioSystemEditor* audioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManager()->GetImplementation())
                {
                    if (IAudioSystemControl* middlewareControl = audioSystemImpl->GetControl(connection->GetID()))
                    {
                        SignalConnectionRemoved(middlewareControl);
                    }
                }
                SignalControlModified();
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::ClearConnections()
    {
        SignalControlAboutToBeModified();
        if (IAudioSystemEditor* audioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManager()->GetImplementation())
        {
            for (auto& connectionPtr : m_connectedControls)
            {
                if (IAudioSystemControl* middlewareControl = audioSystemImpl->GetControl(connectionPtr->GetID()))
                {
                    audioSystemImpl->ConnectionRemoved(middlewareControl);
                    SignalConnectionRemoved(middlewareControl);
                }
            }
        }
        m_connectedControls.clear();
        SignalControlModified();
    }

    //-------------------------------------------------------------------------------------------//
    bool CATLControl::IsFullyConnected() const
    {
        bool isConnected = ConnectionCount() > 0;
        //  Switches have no connections. Their child Switch_States do.
        if (m_type == eACET_SWITCH)
        {
            isConnected = true;
            for (auto& childPtr : m_children)
            {
                if (!childPtr->IsFullyConnected())
                {
                    isConnected = false;
                    break;
                }
            }
        }
        else
        {
            if (IAudioSystemEditor* audioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManager()->GetImplementation())
            {
                for (auto& connectionPtr : m_connectedControls)
                {
                    if (IAudioSystemControl* middlewareControl = audioSystemImpl->GetControl(connectionPtr->GetID()))
                    {
                        if (!middlewareControl->IsConnected() || middlewareControl->IsPlaceholder())
                        {
                            isConnected = false;
                            break;
                        }
                    }
                }
            }
        }

        return isConnected;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::RemoveConnection(IAudioSystemControl* middlewareControl)
    {
        if (middlewareControl)
        {
            const CID id = middlewareControl->GetId();
            auto it = m_connectedControls.begin();
            auto end = m_connectedControls.end();
            for (; it != end; ++it)
            {
                if ((*it)->GetID() == id)
                {
                    SignalControlAboutToBeModified();
                    m_connectedControls.erase(it);
                    SignalConnectionRemoved(middlewareControl);
                    SignalControlModified();
                    return;
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SignalControlModified()
    {
        if (m_atlControlsModel)
        {
            m_atlControlsModel->OnControlModified(this);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SignalControlAboutToBeModified()
    {
        if (!CUndo::IsSuspended())
        {
            CUndo undo("ATL Control Modified");
            CUndo::Record(new CUndoControlModified(GetId()));
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SignalConnectionAdded(IAudioSystemControl* middlewareControl)
    {
        if (m_atlControlsModel)
        {
            m_atlControlsModel->OnConnectionAdded(this, middlewareControl);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SignalConnectionRemoved(IAudioSystemControl* middlewareControl)
    {
        if (m_atlControlsModel)
        {
            m_atlControlsModel->OnConnectionRemoved(this, middlewareControl);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::ReloadConnections()
    {
        if (IAudioSystemEditor* audioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManager()->GetImplementation())
        {
            for (auto& connectionNode : m_connectionNodes)
            {
                if (TConnectionPtr connection = audioSystemImpl->CreateConnectionFromXMLNode(connectionNode.m_xmlNode, m_type))
                {
                    AddConnection(connection);
                    connectionNode.m_isValid = true;
                }
                else
                {
                    connectionNode.m_isValid = false;
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SetParent(CATLControl* parent)
    {
        m_parent = parent;
        if (m_parent)
        {
            SetScope(m_parent->GetScope());
        }
    }

    bool CATLControl::SwitchStateConnectionCheck(IAudioSystemControl* middlewareControl)
    {
        if (IAudioSystemEditor* audioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManager()->GetImplementation())
        {
            CID parentID = middlewareControl->GetParent()->GetId();
            EACEControlType compatibleType = audioSystemImpl->ImplTypeToATLType(middlewareControl->GetType());
            if (compatibleType == EACEControlType::eACET_SWITCH_STATE && m_type == EACEControlType::eACET_SWITCH)
            {
                for (auto& child : m_children)
                {
                    for (int j = 0; child && j < child->ConnectionCount(); ++j)
                    {
                        TConnectionPtr tmpConnection = child->GetConnectionAt(j);
                        if (tmpConnection)
                        {
                            IAudioSystemControl* tmpMiddlewareControl = audioSystemImpl->GetControl(tmpConnection->GetID());
                            EACEControlType controlType = audioSystemImpl->ImplTypeToATLType(tmpMiddlewareControl->GetType());
                            if (tmpMiddlewareControl && controlType == EACEControlType::eACET_SWITCH_STATE)
                            {
                                if (parentID != ACE_INVALID_CID && tmpMiddlewareControl->GetParent()->GetId() != parentID)
                                {
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
        }
        return true;
    }

} // namespace AudioControls
