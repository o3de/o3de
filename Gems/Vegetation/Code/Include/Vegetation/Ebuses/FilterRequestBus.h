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

namespace Vegetation
{
    struct Descriptor;
    struct InstanceData;

    //stages determine the order of execution of filter requests
    enum class FilterStage : AZ::u8
    {
        Default = 0, //filter can be overridden by spawner etc
        PreProcess,
        PostProcess,
    };

    /**
    * the EBus is used to determine if vegetation operation should be performed
    */
    class FilterRequests : public AZ::ComponentBus
    {
    public:
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        virtual bool Evaluate(const InstanceData& instanceData) const = 0;

        virtual void SetFilterStage(FilterStage filterStage) = 0;

        virtual FilterStage GetFilterStage() const { return FilterStage::Default; };
    };

    typedef AZ::EBus<FilterRequests> FilterRequestBus;
}