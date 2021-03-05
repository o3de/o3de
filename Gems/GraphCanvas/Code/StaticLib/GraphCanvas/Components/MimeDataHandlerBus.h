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

#include <qglobal.h>
#include <qpoint.h>

#include <AzCore/EBus/EBus.h>

QT_FORWARD_DECLARE_CLASS(QMimeData);

namespace GraphCanvas
{
    //! SceneMimeDelegateRequests
    //! The API by which pluggable MIME handlers are added to the mime handler
    class SceneMimeDelegateRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Add a pluggable delegate handler to the main handler.
        //! # Parameters
        //! 1. The delegate to add.
        //! 2. Should the DefaultMimeHandler take ownership of the entity (serialize it, etc.)?
        virtual void AddDelegate(const AZ::EntityId& delegateId) = 0;
        //! Remove a pluggable delegate handler from the main handler.
        virtual void RemoveDelegate(const AZ::EntityId& delegateId) = 0;
    };

    using SceneMimeDelegateRequestBus = AZ::EBus<SceneMimeDelegateRequests>;

    //! MimeDelegateHandlerRequests
    //! This interface provides a means for pluggable MIME data handlers to provide capabilities to the default
    //! implementation (\ref DefaultMimeDataHandler).
    //!
    //! This allows custom handlers to be easily added to a scene for whatever use-cases a user has.
    class SceneMimeDelegateHandlerRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Is the delegate interested in the data?
        //! If no delegates are, then the drag will be refused.
        //! # Parameters
        //! 1. The scene that is receiving the event.
        //! 2. The QMimeData associated with the drag.
        virtual bool IsInterestedInMimeData(const AZ::EntityId& sceneId, const QMimeData* mimeData) = 0;

        //! When a dragged element is moved, all interested delegates will be notified.
        //! # Parameters
        //! 1. The scene that is receiving the event.
        //! 2. The QMimeData associated with the drag.
        virtual void HandleMove(const AZ::EntityId& sceneId, const QPointF& dropPoint, const QMimeData* mimeData) = 0;

        //! When a drop takes place, the first capable handler will receive the data to process.
        //! # Parameters        
        //! 1. The scene the view is displaying.
        //! 2. The QPoint where the drop occured.
        //! 3. The Mime data of the drop
        virtual void HandleDrop(const AZ::EntityId& sceneId, const QPointF& dropPoint, const QMimeData* mimeData) = 0;        

        //! When a leave occurs, all interested handlers will receive the data to process
        //! # Parameters
        //! 1. The scene the view is displaying.        
        //! 3. The Mime data of the drop
        virtual void HandleLeave(const AZ::EntityId& sceneId, const QMimeData* mimeData) = 0;
    };

    using SceneMimeDelegateHandlerRequestBus = AZ::EBus<SceneMimeDelegateHandlerRequests>;

}