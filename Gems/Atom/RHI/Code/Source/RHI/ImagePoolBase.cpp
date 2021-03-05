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
#include <Atom/RHI/ImagePoolBase.h>

namespace AZ
{
    namespace RHI
    {
        ResultCode ImagePoolBase::InitImage(Image* image, const ImageDescriptor& descriptor, PlatformMethod platformInitResourceMethod)
        {
            
            // The descriptor is assigned regardless of whether initialization succeeds. Descriptors are considered
            // undefined for uninitialized resources. This makes the image descriptor well defined at initialization
            // time rather than leftover data from the previous usage.             
            image->SetDescriptor(descriptor);
            
            return ResourcePool::InitResource(image, platformInitResourceMethod);
        }
    }
}
