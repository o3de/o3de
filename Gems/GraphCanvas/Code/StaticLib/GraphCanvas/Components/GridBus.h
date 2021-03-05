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

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>

namespace GraphCanvas
{

    //! GridRequests
    //! Requests which are serviced by the \ref GraphCanvas::Connection component.
    class GridRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Set the major grid line pitch.
        //! The pitch is specified in scene coordinates and X and Y can be separately specified.
        virtual void SetMajorPitch(const AZ::Vector2&) = 0;
        //! Get the major grid line pitch.
        //! Contains the distinct X and Y pitches.
        virtual AZ::Vector2 GetMajorPitch() const = 0;

        //! Set the minor grid line pitch.
        //! The pitch is specified in scene coordinates and X and Y can be separately specified.
        //! NOTE: This pitch should be a factor of the major pitch.
        virtual void SetMinorPitch(const AZ::Vector2&) = 0;
        //! Get the minor grid line pitch.
        //! Contains the distinct X and Y pitches.
        virtual AZ::Vector2 GetMinorPitch() const = 0;

        //! When the view is zoomed, the grid lines (minor and/or major) could become too close together to be useful or
        //! visually appealing.
        //! If the on-screen (i.e. scaled) distance in pixels between grid lines falls below this threshold, they will
        //! no longer be drawn. This will cause the minor and then the major grid lines to disappear as we zoom out
        //! further and further.
        virtual void SetMinimumVisualPitch(int) = 0;
        //! Get the smallest on-screen distance between grid lines that is allowable for them to still be rendered.
        virtual int GetMinimumVisualPitch() const = 0;
    };

    using GridRequestBus = AZ::EBus<GridRequests>;

    //! ConnectionNotifications
    //! Notifications about changes to a connection's state.
    class GridNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;


        //! The grid has had its major grid line pitch changed.
        virtual void OnMajorPitchChanged(const AZ::Vector2&) {}
        //! The grid has had its minor grid line pitch changed.
        virtual void OnMinorPitchChanged(const AZ::Vector2&) {}
        //! The grid has had its minimum allowable visual grid-line spacing changed.
        virtual void OnMinimumVisualPitchChanged(int) {}
    };

    using GridNotificationBus = AZ::EBus<GridNotifications>;

}