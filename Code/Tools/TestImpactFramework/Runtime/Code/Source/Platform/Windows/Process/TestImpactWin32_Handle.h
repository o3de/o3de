/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
