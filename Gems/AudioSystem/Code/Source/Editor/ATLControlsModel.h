/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AudioControl.h>
#include <AzCore/std/string/string_view.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    // available levels where the controls can be stored
    struct SControlScope
    {
        SControlScope() = default;
        SControlScope(const AZStd::string& _name, bool _bOnlyLocal)
            : name(_name)
            , bOnlyLocal(_bOnlyLocal)
        {}

        AZStd::string name;

        // if true, there is a level in the game audio
        // data that doesn't exist in the global list
        // of levels for your project
        bool bOnlyLocal;
    };

    //-------------------------------------------------------------------------------------------//
    struct IATLControlModelListener
    {
        virtual ~IATLControlModelListener() = default;
        virtual void OnControlAdded([[maybe_unused]] CATLControl* control) {}
        virtual void OnControlModified([[maybe_unused]] CATLControl* control) {}
        virtual void OnControlRemoved([[maybe_unused]] CATLControl* control) {}
        virtual void OnConnectionAdded([[maybe_unused]] CATLControl* control, [[maybe_unused]] IAudioSystemControl* middlewareControl) {}
        virtual void OnConnectionRemoved([[maybe_unused]] CATLControl* control, [[maybe_unused]] IAudioSystemControl* middlewareControl) {}
    };

    //-------------------------------------------------------------------------------------------//
    class CATLControlsModel
    {
        friend class IUndoControlOperation;
        friend class CUndoControlModified;
        friend class CATLControl;

    public:
        CATLControlsModel();
        ~CATLControlsModel();

        void Clear();
        CATLControl* CreateControl(const AZStd::string& controlName, EACEControlType type, CATLControl* parent = nullptr);
        void RemoveControl(CID id);

        CATLControl* GetControlByID(CID id) const;
        CATLControl* FindControl(const AZStd::string_view controlName, EACEControlType type, const AZStd::string_view scope, CATLControl* parent = nullptr) const;

        // Scope
        void AddScope(AZStd::string scopeName, bool localOnly = false);
        void ClearScopes();
        size_t GetScopeCount() const;
        SControlScope GetScopeAt(size_t index) const;
        bool ScopeExists(AZStd::string scopeName) const;

        // Helper functions
        bool IsNameValid(const AZStd::string_view name, EACEControlType type, const AZStd::string_view scope, const CATLControl* const parent = nullptr) const;
        AZStd::string GenerateUniqueName(const AZStd::string_view rootName, EACEControlType type, const AZStd::string_view scope, const CATLControl* const parent = nullptr) const;
        void ClearAllConnections();
        void ReloadAllConnections();

        void AddListener(IATLControlModelListener* modelListener);
        void RemoveListener(IATLControlModelListener* modelListener);
        void SetSuppressMessages(bool suppressMessages);
        bool IsTypeDirty(EACEControlType type);
        bool IsDirty();
        void ClearDirtyFlags();

    private:
        void OnControlAdded(CATLControl* control);
        void OnControlModified(CATLControl* control);
        void OnConnectionAdded(CATLControl* control, IAudioSystemControl* middlewareControl);
        void OnConnectionRemoved(CATLControl* control, IAudioSystemControl* middlewareControl);
        void OnControlRemoved(CATLControl* control);

        CID GenerateUniqueId() { return ++m_nextId; }

        AZStd::shared_ptr<CATLControl> TakeControl(CID nID);
        void InsertControl(AZStd::shared_ptr<CATLControl> control);

        static CID m_nextId;
        AZStd::vector<AZStd::shared_ptr<CATLControl>> m_controls;
        AZStd::vector<SControlScope> m_scopes;

        AZStd::vector<IATLControlModelListener*> m_listeners;
        bool m_suppressMessages;
        bool m_isControlTypeModified[eACET_NUM_TYPES];
    };

} // namespace AudioControls
