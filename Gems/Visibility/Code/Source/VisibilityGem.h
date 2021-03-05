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

#include <IGem.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>


class VisibilityGem
    : public AZ::Module
{
public:
    AZ_RTTI(VisibilityGem, "{5138F2B6-EDFB-490E-AB3E-B82E43263A20}");
    
    VisibilityGem();
};