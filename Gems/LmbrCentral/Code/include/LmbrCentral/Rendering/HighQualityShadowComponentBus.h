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

#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    /*!
     * Messages serviced by the unique shadow component.
     */
    class HighQualityShadowComponentRequests
        : public AZ::ComponentBus
    {
    public:

        virtual void SetEnabled(bool enabled) = 0;
        virtual bool GetEnabled() = 0;
    };

    using HighQualityShadowComponentRequestBus = AZ::EBus<HighQualityShadowComponentRequests>;
    
    /*!
     * Messages serviced by the unique shadow editor component.
     */
    class EditorHighQualityShadowComponentRequests
        : public AZ::ComponentBus
    {
    public:

        virtual void RefreshProperties() = 0;
    };

    using EditorHighQualityShadowComponentRequestBus = AZ::EBus<EditorHighQualityShadowComponentRequests>;
} // namespace LmbrCentral
