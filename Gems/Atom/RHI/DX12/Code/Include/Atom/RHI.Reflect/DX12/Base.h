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

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/DX12/Base_Platform.h>

namespace AZ
{
    namespace DX12
    {
        //! For details go to AZ::RHI::Factory::GetAPIUniqueIndex 
        static constexpr uint32_t APIUniqueIndex = static_cast<uint32_t>(RHI::APIIndex::DX12);
    }
}
