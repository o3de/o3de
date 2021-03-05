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

////////////////////////////////////////////////////////////////////////////////////////////////////
class LyShineExamplesCppExampleInterface
    : public AZ::EBusTraits
{
public: // member functions

    //! Builds the example canvas
    virtual void CreateCanvas() = 0;

    //! Destroys the example canvas
    virtual void DestroyCanvas() = 0;
};

typedef AZ::EBus<LyShineExamplesCppExampleInterface> LyShineExamplesCppExampleBus;
