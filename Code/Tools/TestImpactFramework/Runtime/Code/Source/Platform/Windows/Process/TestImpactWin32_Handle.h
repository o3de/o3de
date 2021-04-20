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

#pragma once

#include <AzCore/PlatformIncl.h>

namespace TestImpact
{
    //! OS function to cleanup handle
    using CleanupFunc = BOOL (*)(HANDLE);

    //! RAII wrapper around OS handles.
    template<CleanupFunc CleanupFuncT>
    class Handle
    {
    public:
        Handle() = default;
        explicit Handle(HANDLE handle);
        ~Handle();

        operator HANDLE&();
        PHANDLE operator&();
        HANDLE& operator=(HANDLE handle);
        void Close();

    private:
        HANDLE m_handle = INVALID_HANDLE_VALUE;
    };

    template<CleanupFunc CleanupFuncT>
    Handle<CleanupFuncT>::Handle(HANDLE handle)
        : m_handle(handle)
    {
    }

    template<CleanupFunc CleanupFuncT>
    Handle<CleanupFuncT>::~Handle()
    {
        Close();
    }

    template<CleanupFunc CleanupFuncT>
    Handle<CleanupFuncT>::operator HANDLE&()
    {
        return m_handle;
    }

    template<CleanupFunc CleanupFuncT>
    PHANDLE Handle<CleanupFuncT>::operator&()
    {
        return &m_handle;
    }

    template<CleanupFunc CleanupFuncT>
    HANDLE& Handle<CleanupFuncT>::operator=(HANDLE handle)
    {
        // Setting the handle to INVALID_HANDLE_VALUE will close the handle
        if (handle == INVALID_HANDLE_VALUE && m_handle != INVALID_HANDLE_VALUE)
        {
            Close();
        }
        else
        {
            m_handle = handle;
        }

        return m_handle;
    }

    template<CleanupFunc CleanupFuncT>
    void Handle<CleanupFuncT>::Close()
    {
        if (m_handle != INVALID_HANDLE_VALUE)
        {
            CleanupFuncT(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
    }

    using ObjectHandle = Handle<CloseHandle>;
    using WaitHandle = Handle<UnregisterWait>;
} // namespace TestImpact
