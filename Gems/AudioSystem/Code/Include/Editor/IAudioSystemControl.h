/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/std/string/string_view.h>

#include <ACETypes.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    class IAudioSystemControl
    {
    public:
        IAudioSystemControl() = default;

        IAudioSystemControl(const AZStd::string& name, CID id, TImplControlType type)
            : m_name(name)
            , m_id(id)
            , m_type(type)
        {}

        virtual ~IAudioSystemControl() = default;

        // unique id for this control
        CID GetId() const
        {
            return m_id;
        }

        void SetId(CID id)
        {
            m_id = id;
        }

        TImplControlType GetType() const
        {
            return m_type;
        }

        void SetType(TImplControlType type)
        {
            m_type = type;
        }

        const AZStd::string& GetName() const
        {
            return m_name;
        }

        void SetName(const AZStd::string_view name)
        {
            if (m_name != name)
            {
                m_name = name;
            }
        }

        bool IsPlaceholder() const
        {
            return m_isPlaceholder;
        }

        void SetPlaceholder(bool isPlaceholder)
        {
            m_isPlaceholder = isPlaceholder;
        }

        bool IsLocalized() const
        {
            return m_isLocalized;
        }

        void SetLocalized(bool isLocalized)
        {
            m_isLocalized = isLocalized;
        }

        bool IsConnected() const
        {
            return m_isConnected;
        }

        void SetConnected(bool isConnected)
        {
            m_isConnected = isConnected;
        }

        size_t ChildCount() const
        {
            return m_children.size();
        }

        void AddChild(IAudioSystemControl* pChild)
        {
            m_children.push_back(pChild);
            pChild->SetParent(this);
        }

        IAudioSystemControl* GetChildAt(size_t index) const
        {
            return (index < m_children.size() ? m_children[index] : nullptr);
        }

        void SetParent(IAudioSystemControl* pParent)
        {
            m_parent = pParent;
        }

        IAudioSystemControl* GetParent() const
        {
            return m_parent;
        }

    private:
        AZStd::vector<IAudioSystemControl*> m_children;

        AZStd::string m_name;
        IAudioSystemControl* m_parent = nullptr;
        CID m_id = ACE_INVALID_CID;
        TImplControlType m_type = AUDIO_IMPL_INVALID_TYPE;
        bool m_isPlaceholder = false;
        bool m_isLocalized = false;
        bool m_isConnected = false;
    };

} // namespace AudioControls
