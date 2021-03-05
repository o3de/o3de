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

#include <AzQtComponents/Components/StyleManager.h>

namespace AzQtComponents
{
    // Here for backwards compatibility with the old name of this class
    class LumberyardStylesheet : public StyleManager
    {
    public:
        LumberyardStylesheet(QObject* parent) : StyleManager(parent) {}
    };

} // namespace AzQtComponents

