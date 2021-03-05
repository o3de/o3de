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
//! Defines an interface for working with indexable image types, such as sprite-sheets or image sequences.
class UiIndexableImageInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiIndexableImageInterface() {}
    
    //! Sets the index of the image to display
    virtual void SetImageIndex(AZ::u32 index) = 0;

    //! Gets the index of the image to display
    virtual const AZ::u32 GetImageIndex() = 0;

    //! Gets the number of indices for this image.
    virtual const AZ::u32 GetImageIndexCount() = 0;    

    //! Given an index, return its alias (if defined)
    virtual AZStd::string GetImageIndexAlias(AZ::u32 index) = 0;

    //! Given an index, set an alias for it
    virtual void SetImageIndexAlias(AZ::u32 index, const AZStd::string& alias) = 0;

    //! Given an alias, return the index that corresponds to it
    virtual AZ::u32 GetImageIndexFromAlias(const AZStd::string& alias) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiIndexableImageInterface> UiIndexableImageBus;
