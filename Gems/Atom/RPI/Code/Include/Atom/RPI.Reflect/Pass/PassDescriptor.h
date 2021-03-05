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

#include <Atom/RPI.Reflect/Pass/PassData.h>

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
        struct PassDescriptor
        {
            AZ_TYPE_INFO(PassDescriptor, "{71E0E3D4-58FC-4254-BA7B-5A7ADFE15FE7}");

            PassDescriptor() = default;
            ~PassDescriptor() = default;

            explicit PassDescriptor(Name name, const AZStd::shared_ptr<PassTemplate> passTemplate = nullptr, const PassRequest* passRequest = nullptr)
                : m_passName(name)
                , m_passTemplate(passTemplate)
                , m_passRequest(passRequest)
            { }

            //! Required: Every PassDescriptor must have a valid name before being used as an input for Pass construction
            Name m_passName;

            //! Optional: The PassTemplate used to construct a Pass.
            AZStd::shared_ptr<PassTemplate> m_passTemplate = nullptr;

            //! Optional: The PassRequest used to construct a Pass. If this is valid then m_passTemplate 
            //! cannot be null and m_passTemplate must point to the same template used by the PassRequest.
            const PassRequest* m_passRequest = nullptr;

            //! Optional: Custom data used for pass initialization. This data usually comes from the pass
            //! template or pass request. Only use this if you are initializing a pass without either of those.
            AZStd::shared_ptr<PassData> m_passData = nullptr;
        };
    }
}
