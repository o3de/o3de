/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
