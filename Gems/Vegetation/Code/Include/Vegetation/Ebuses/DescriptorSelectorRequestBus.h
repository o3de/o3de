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
#include <AzCore/std/containers/vector.h>
#include <Vegetation/Descriptor.h>

namespace Vegetation
{
    struct DescriptorSelectorParams
    {
        AZ::Vector3 m_position = AZ::Vector3(0.0f, 0.0f, 0.0f);
    };

    /**
    * the EBus is used to select/reduce vegetation descriptors from set
    */
    class DescriptorSelectorRequests : public AZ::ComponentBus
    {
    public:
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        //the descriptor pointer list will be mutated and reduced by this operation
        //descriptor pointers should be valid during selection and not stored by implementor
        virtual void SelectDescriptors(const DescriptorSelectorParams& params, DescriptorPtrVec& descriptors) const = 0;
    };

    typedef AZ::EBus<DescriptorSelectorRequests> DescriptorSelectorRequestBus;
}