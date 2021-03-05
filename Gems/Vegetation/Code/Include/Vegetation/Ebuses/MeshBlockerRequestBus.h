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
#include <Vegetation/Ebuses/AreaConfigRequestBus.h>

namespace Vegetation
{
    class MeshBlockerRequests
        : public AreaConfigRequests
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual bool GetInheritBehavior() const = 0;
        virtual void SetInheritBehavior(bool value) = 0;

        virtual float GetMeshHeightPercentMin() const = 0;
        virtual void SetMeshHeightPercentMin(float meshHeightPercentMin) = 0;

        virtual float GetMeshHeightPercentMax() const = 0;
        virtual void SetMeshHeightPercentMax(float meshHeightPercentMax) = 0;

        virtual bool GetBlockWhenInvisible() const = 0;
        virtual void SetBlockWhenInvisible(bool value) = 0;
    };

    using MeshBlockerRequestBus = AZ::EBus<MeshBlockerRequests>;
}