/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ATLControlsModel.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AudioControlsEditorUndo.h>
#include <IEditor.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    CID CATLControlsModel::m_nextId = ACE_INVALID_CID;

    //-------------------------------------------------------------------------------------------//
    CATLControlsModel::CATLControlsModel()
        : m_suppressMessages(false)
    {
        ClearDirtyFlags();
    }

    //-------------------------------------------------------------------------------------------//
    CATLControlsModel::~CATLControlsModel()
    {
        Clear();
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* CATLControlsModel::CreateControl(const AZStd::string& controlName, EACEControlType type, CATLControl* parent)
    {
        AZStd::shared_ptr<CATLControl> control = AZStd::make_shared<CATLControl>(controlName, GenerateUniqueId(), type, this);
        if (control)
        {
            if (parent)
            {
                control->SetParent(parent);
            }

            InsertControl(control);

            if (!CUndo::IsSuspended())
            {
                CUndo undo("Audio Control Created");
                CUndo::Record(new CUndoControlAdd(control->GetId()));
            }
        }
        return control.get();
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::RemoveControl(CID id)
    {
        if (id != ACE_INVALID_CID)
        {
            for (auto it = m_controls.begin(); it != m_controls.end(); ++it)
            {
                AZStd::shared_ptr<CATLControl>& control = *it;
                if (control && control->GetId() == id)
                {
                    control->ClearConnections();
                    OnControlRemoved(control.get());

                    // Remove control from parent
                    CATLControl* parent = control->GetParent();
                    if (parent)
                    {
                        parent->RemoveChild(control.get());
                    }

                    if (!CUndo::IsSuspended())
                    {
                        CUndo::Record(new CUndoControlRemove(control));
                    }

                    m_controls.erase(it, it + 1);
                    break;
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* CATLControlsModel::GetControlByID(CID id) const
    {
        if (id != ACE_INVALID_CID)
        {
            size_t size = m_controls.size();
            for (size_t i = 0; i < size; ++i)
            {
                if (m_controls[i]->GetId() == id)
                {
                    return m_controls[i].get();
                }
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    bool CATLControlsModel::IsNameValid(const AZStd::string_view name, EACEControlType type, const AZStd::string_view scope, const CATLControl* const parent) const
    {
        const size_t size = m_controls.size();
        for (size_t i = 0; i < size; ++i)
        {
            if (m_controls[i]
                && m_controls[i]->GetType() == type
                && (AZ::StringFunc::Equal(m_controls[i]->GetName().c_str(), name.data()))
                && (m_controls[i]->GetScope().empty() || m_controls[i]->GetScope() == scope)
                && (m_controls[i]->GetParent() == parent))
            {
                return false;
            }
        }
        return true;
    }

    //-------------------------------------------------------------------------------------------//
    AZStd::string CATLControlsModel::GenerateUniqueName(const AZStd::string_view rootName, EACEControlType type, const AZStd::string_view scope, const CATLControl* const parent) const
    {
        AZStd::string uniqueName = rootName;
        AZ::u32 number = 1;
        while (!IsNameValid(uniqueName, type, scope, parent))
        {
            uniqueName = AZStd::string::format("%s_%u", rootName.data(), number++);
        }

        return uniqueName;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::AddScope(AZStd::string scopeName, bool localOnly)
    {
        AZStd::to_lower(scopeName.begin(), scopeName.end());
        const size_t size = m_scopes.size();
        for (int i = 0; i < size; ++i)
        {
            if (m_scopes[i].name == scopeName)
            {
                return;
            }
        }
        m_scopes.push_back(SControlScope(scopeName, localOnly));
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::ClearScopes()
    {
        m_scopes.clear();
    }

    //-------------------------------------------------------------------------------------------//
    bool CATLControlsModel::ScopeExists(AZStd::string scopeName) const
    {
        AZStd::to_lower(scopeName.begin(), scopeName.end());
        const size_t size = m_scopes.size();
        for (size_t i = 0; i < size; ++i)
        {
            if (m_scopes[i].name == scopeName)
            {
                return true;
            }
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------//
    size_t CATLControlsModel::GetScopeCount() const
    {
        return m_scopes.size();
    }

    //-------------------------------------------------------------------------------------------//
    SControlScope CATLControlsModel::GetScopeAt(size_t index) const
    {
        return (index < m_scopes.size() ? m_scopes[index] : SControlScope());
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::Clear()
    {
        m_controls.clear();
        m_scopes.clear();
        ClearDirtyFlags();
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::AddListener(IATLControlModelListener* modelListener)
    {
        if (AZStd::find(m_listeners.begin(), m_listeners.end(), modelListener) == m_listeners.end())
        {
            m_listeners.push_back(modelListener);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::RemoveListener(IATLControlModelListener* modelListener)
    {
        auto it = AZStd::find(m_listeners.begin(), m_listeners.end(), modelListener);
        if (it != m_listeners.end())
        {
            m_listeners.erase(it);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::OnControlAdded(CATLControl* control)
    {
        if (!m_suppressMessages)
        {
            for (auto listener : m_listeners)
            {
                listener->OnControlAdded(control);
            }
            m_isControlTypeModified[control->GetType()] = true;
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::OnControlRemoved(CATLControl* control)
    {
        if (!m_suppressMessages)
        {
            for (auto listener : m_listeners)
            {
                listener->OnControlRemoved(control);
            }
            m_isControlTypeModified[control->GetType()] = true;
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::OnConnectionAdded(CATLControl* control, IAudioSystemControl* middlewareControl)
    {
        if (!m_suppressMessages)
        {
            for (auto listener : m_listeners)
            {
                listener->OnConnectionAdded(control, middlewareControl);
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::OnConnectionRemoved(CATLControl* control, IAudioSystemControl* middlewareControl)
    {
        if (!m_suppressMessages)
        {
            for (auto listener : m_listeners)
            {
                listener->OnConnectionRemoved(control, middlewareControl);
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::OnControlModified(CATLControl* control)
    {
        if (!m_suppressMessages)
        {
            for (auto listener : m_listeners)
            {
                listener->OnControlModified(control);
            }
            m_isControlTypeModified[control->GetType()] = true;
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::SetSuppressMessages(bool suppressMessages)
    {
        m_suppressMessages = suppressMessages;
    }

    //-------------------------------------------------------------------------------------------//
    bool CATLControlsModel::IsDirty()
    {
        for (int i = 0; i < eACET_NUM_TYPES; ++i)
        {
            if (m_isControlTypeModified[i])
            {
                return true;
            }
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------//
    bool CATLControlsModel::IsTypeDirty(EACEControlType type)
    {
        if (type != eACET_NUM_TYPES)
        {
            return m_isControlTypeModified[type];
        }
        return true;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::ClearDirtyFlags()
    {
        for (int i = 0; i < eACET_NUM_TYPES; ++i)
        {
            m_isControlTypeModified[i] = false;
        }
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* CATLControlsModel::FindControl(const AZStd::string_view controlName, EACEControlType type, const AZStd::string_view scope, CATLControl* parent) const
    {
        if (parent)
        {
            const size_t size = parent->ChildCount();
            for (size_t i = 0; i < size; ++i)
            {
                CATLControl* control = parent->GetChild(i);
                if (control
                    && control->GetName() == controlName
                    && control->GetType() == type
                    && control->GetScope() == scope)
                {
                    return control;
                }
            }
        }
        else
        {
            const size_t size = m_controls.size();
            for (size_t i = 0; i < size; ++i)
            {
                CATLControl* control = m_controls[i].get();
                if (control
                    && control->GetName() == controlName
                    && control->GetType() == type
                    && control->GetScope() == scope)
                {
                    return control;
                }
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    AZStd::shared_ptr<CATLControl> CATLControlsModel::TakeControl(CID id)
    {
        const size_t size = m_controls.size();
        for (size_t i = 0; i < size; ++i)
        {
            if (m_controls[i]->GetId() == id)
            {
                AZStd::shared_ptr<CATLControl> control = m_controls[i];
                RemoveControl(id);
                return control;
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::InsertControl(AZStd::shared_ptr<CATLControl> control)
    {
        if (control)
        {
            m_controls.push_back(control);

            CATLControl* parent = control->GetParent();
            if (parent)
            {
                parent->AddChild(control.get());
            }

            OnControlAdded(control.get());
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::ClearAllConnections()
    {
        const size_t size = m_controls.size();
        for (size_t i = 0; i < size; ++i)
        {
            CATLControl* control = m_controls[i].get();
            if (control)
            {
                control->ClearConnections();
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::ReloadAllConnections()
    {
        const size_t size = m_controls.size();
        for (size_t i = 0; i < size; ++i)
        {
            CATLControl* control = m_controls[i].get();
            if (control)
            {
                control->ReloadConnections();
            }
        }
    }

} // namespace AudioControls
