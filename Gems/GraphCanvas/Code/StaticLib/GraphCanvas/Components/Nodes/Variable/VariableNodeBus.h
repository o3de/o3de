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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>

#include <GraphCanvas/Types/Endpoint.h>

class QGraphicsLayoutItem;
class QMimeData;

namespace GraphCanvas
{
    namespace Deprecated
    {
        //! SceneVariableRequestBus
        //! Requests to be made about variables in a particular scene.
        class SceneVariableRequests : public AZ::EBusTraits
        {
        public:
            // The BusId here is the SceneId of the scene that contains the variable.
            //
            // Should mainly be used with enumeration for collecting information and not as a way of directly interacting
            // with variables inside of a particular scene.
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = AZ::EntityId;

            virtual AZ::EntityId GetVariableId() const = 0;
        };

        using SceneVariableRequestBus = AZ::EBus<SceneVariableRequests>;

        //! VariableRequestBus
        //! Requests to be made about a particular variable.
        class VariableRequests : public AZ::EBusTraits
        {
        public:
            // The BusId here is the VariableId of the variable that information is required about.
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = AZ::EntityId;

            virtual AZStd::string GetVariableName() const = 0;
            virtual AZ::Uuid GetDataType() const = 0;
            virtual AZStd::string GetDataTypeDisplayName() const = 0;
        };

        using VariableRequestBus = AZ::EBus<VariableRequests>;
    }
}