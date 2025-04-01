/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Pass/PassData.h>
#include <Atom/RPI.Reflect/Pass/PassRequest.h>

namespace AZ
{
    namespace RPI
    {
        class PassTemplate;
        struct PassRequest;

        //! Single struct that serves as the input for pass constructors.
        //! A PassDescriptor must always have a valid name.
        //! It has optional pointers to a PassTemplate and a PassRequest
        //! If the PassRequest is valid then the PassTemplate must also be valid
        //! and point to the PassTemplate used by the PassRequest.
        struct ATOM_RPI_REFLECT_API PassDescriptor
        {
            AZ_TYPE_INFO(PassDescriptor, "{71E0E3D4-58FC-4254-BA7B-5A7ADFE15FE7}");

            PassDescriptor() = default;
            ~PassDescriptor() = default;

            explicit PassDescriptor(Name name, const AZStd::shared_ptr<const PassTemplate> passTemplate = nullptr, const PassRequest* passRequest = nullptr)
                : m_passName(name)
                , m_passTemplate(passTemplate)
            { 
                if (passRequest)
                {
                    // If the optional passRequest is passed in, create a copy of it and reference it with a smart pointer
                    // rather than keeping a reference to the passRequest
                    m_passRequest = AZStd::make_shared<PassRequest>(*passRequest);
                }
            }

            //! Required: Every PassDescriptor must have a valid name before being used as an input for Pass construction
            Name m_passName;

            //! Optional: The PassTemplate used to construct a Pass.
            AZStd::shared_ptr<const PassTemplate> m_passTemplate = nullptr;

            //! Optional: The PassRequest used to construct a Pass. If this is valid then m_passTemplate 
            //! cannot be null and m_passTemplate must point to the same template used by the PassRequest.
            AZStd::shared_ptr<PassRequest> m_passRequest;

            //! Optional: Custom data used for pass initialization. This data usually comes from the pass
            //! template or pass request. Only use this if you are initializing a pass without either of those.
            AZStd::shared_ptr<PassData> m_passData = nullptr;
        };
    }
}
