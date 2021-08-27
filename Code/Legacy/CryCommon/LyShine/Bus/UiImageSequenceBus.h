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
class UiImageSequenceInterface
    : public AZ::ComponentBus
{
public: // types

    enum class ImageType : int32_t
    {
        Stretched,      //!< the texture is stretched to fit the rect without maintaining aspect ratio
        Fixed,          //!< the texture is not stretched at all
        StretchedToFit, //!< the texture is scaled to fit the rect while maintaining aspect ratio
        StretchedToFill //!< the texture is scaled to fill the rect while maintaining aspect ratio
    };

public: // member functions

    virtual ~UiImageSequenceInterface() {}

    //! Gets the type of the image
    virtual ImageType GetImageType() = 0;

    //! Sets the type of the image
    virtual void SetImageType(ImageType imageType) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiImageSequenceInterface> UiImageSequenceBus;
