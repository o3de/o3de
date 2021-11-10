/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomBackend.h>

namespace AZ::DOM
{
    //! Central interface for registering and looking up backend types by file extension.
    class BackendRegistryInterface
    {
    public:
        AZ_RTTI(BackendRegistryInterface, "{B72A88AC-DF15-4F92-9489-ABCDEA3D94E6}")
        using BackendFactory = AZStd::function<AZStd::unique_ptr<Backend>()>;

        virtual ~BackendRegistryInterface() = default;

        //! Registers a factory for a given backend.
        //! \param name A unique name to identify this backend.
        //! \param extensions A set of file extensions this backend supports.
        //! Extensions should by the entire file suffix including any dots (.), for example:
        //! ".json" or ".tar.gz"
        template<class TBackend>
        void RegisterBackend(AZStd::string name, AZStd::vector<AZStd::string> extensions);

        //! Looks up a DOM backend based on its name.
        virtual AZStd::unique_ptr<Backend> GetBackendByName(AZStd::string_view name) = 0;

        //! Looks up a DOM backend based on a file extension.
        virtual AZStd::unique_ptr<Backend> GetBackendForExtension(AZStd::string_view extension) = 0;

    protected:
        //! Registers a factory for a given backend, called by RegisterBackend.
        //! \param factory The factory function to create new backend instances.
        //! \param name The unique name to identify this backend.
        //! \param extensions A set of file extensions this backend supports.
        //! \see RegisterBackend
        virtual void RegisterBackendInternal(BackendFactory factory, AZStd::string name, AZStd::vector<AZStd::string> extensions) = 0;
    };

    template<class TBackend>
    void BackendRegistryInterface::RegisterBackend(AZStd::string name, AZStd::vector<AZStd::string> extensions)
    {
        RegisterBackendInternal([](){
            return AZStd::make_unique<TBackend>();
        }, AZStd::move(name), AZStd::move(extensions));
    }
} // namespace AZ::DOM
