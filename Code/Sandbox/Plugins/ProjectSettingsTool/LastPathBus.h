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

#include <AzCore/EBus/EBus.h>


namespace ProjectSettingsTool
{
    class LastPathTraits
        : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<LastPathTraits>;

        // Bus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual QString GetLastImagePath() = 0;
        virtual void SetLastImagePath(const QString& path) = 0;
    };

    typedef AZ::EBus<LastPathTraits> LastPathBus;
} // namespace ProjectSettingsTool
