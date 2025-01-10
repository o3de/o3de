/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewProviderBus.h>
#include <Atom/RPI.Public/XR/XRRenderingInterface.h>
#include <AzCore/Name/Name.h>

namespace AZ
{
    namespace RPI
    {
        //! This class is responsible for managing stereoscopic and non-stereoscopic views.
        //! It also allows calling code to provide handlers or functions to be called in case the projection
        //! or view matrix changes. 
        class ATOM_RPI_PUBLIC_API ViewGroup final
        {
        public:
            AZ_TYPE_INFO(ViewGroup, "{C1AD45ED-03E2-41E0-9DC7-779B4B1842FF}");
            AZ_CLASS_ALLOCATOR(ViewGroup, AZ::SystemAllocator);

            struct Descriptor
            {
                //! Function to be called in case view matrix changed
                AZStd::function<void(AZ::RPI::ViewPtr view)> m_viewEventFunction;

                //! Function to be called in case projection matrix changed
                AZStd::function<void(AZ::RPI::ViewPtr view)> m_projectionEventFunction;
            };

            struct ViewData
            {
                //! Pointer to the view
                AZ::RPI::ViewPtr m_view = nullptr;

                //If a View (i.e m_view) related transforms are updated these handlers are updated
                MatrixChangedEvent::Handler m_onViewMatrixChangedHandler;
                MatrixChangedEvent::Handler m_onProjectionMatrixChangedHandler;

                //The event handlers above will also signal the following event in case
                //any handlers from outside are connected to these events
                MatrixChangedEvent m_projectionMatrixChangedEvent;
                MatrixChangedEvent m_viewMatrixChangedEvent;
            };

            //! Initialization which involves caching the descriptor and XR system related properties if applicable.
            void Init(const Descriptor& desc);

            //! Create the main non-stereoscopic view.
            void CreateMainView(AZ::Name name);

            //! Create all stereoscopic views.
            void CreateStereoscopicViews(AZ::Name name);

            //! Activate this view group which involves connecting the handlers to the views.
            void Activate();

            //! Deactivate this view group which involves dis-connecting the handlers from the views.
            void Deactivate();

            //! Set the View pointer for the provided view type.
            void SetView(const AZ::RPI::ViewPtr view, ViewType viewType = ViewType::Default);

            //! Set the view transform for the view associated with a view type.
            void SetCameraTransform(const AZ::Matrix3x4& cameraTransform, ViewType viewType = ViewType::Default);

            //! Set the projection transform for the view associated with a view type.
            void SetViewToClipMatrix(const AZ::Matrix4x4& viewToClipMatrix, ViewType viewType = ViewType::Default);

            //! Set the stereoscopic projection transform for the view associated with a view type.
            void SetStereoscopicViewToClipMatrix(const AZ::Matrix4x4& viewToClipMatrix, bool reverseDepth = true, ViewType viewType= ViewType::Default);

            //! Allow calling code to connect a handler to an event that is signaled when the view transform is updated.
            void ConnectViewMatrixChangedEvent(MatrixChangedEvent::Handler& handler, ViewType viewType = ViewType::Default);

            //! Allow calling code to connect a handler to an event that is signaled when the projection transform is updated.
            void ConnectProjectionMatrixChangedEvent(MatrixChangedEvent::Handler& handler, ViewType viewType = ViewType::Default);

            //! Allow calling code to signal the event associated with view transform of the view associated with viewType.
            void SignalViewMatrixChangedEvent(const AZ::Matrix4x4& matrix, ViewType viewType = ViewType::Default);

            //! Allow calling code to signal the event associated with projection transform of the view associated with viewType.
            void SignalProjectionMatrixChangedEvent(const AZ::Matrix4x4& matrix, ViewType viewType = ViewType::Default);

            //! Allow calling code to connect the internal handler to the view transform event of the view associated with viewType.
            void ConnectViewMatrixChangedHandler(ViewType viewType = ViewType::Default);

            //! Allow calling code to connect the internal handler to the projection transform event of the view associated with viewType.
            void ConnectProjectionMatrixChangedHandler(ViewType viewType = ViewType::Default);

            //! Allow calling code to disconnect the handler related to the view transform of the view associated with viewType.
            void DisconnectViewMatrixHandler(ViewType viewType = ViewType::Default);

            //! Allow calling code to disconnect the handler related to the projection transform of the view associated with viewType.
            void DisconnectProjectionMatrixHandler(ViewType viewType = ViewType::Default);

            //! Returns true if this group contains an active view.
            bool IsAnyViewEnabled() const;

             //! Returns true if a view exists within this view group.
            bool IsViewInGroup(const AZ::RPI::ViewPtr view) const;

            //! Returns true if all the views within viewGroup matches the one in this group
            bool IsViewGroupViewsSame(const AZ::RPI::ViewGroupPtr viewGroup) const;

            //! Return the number of stereoscopic views.
            uint32_t GetNumViews() const;

            //! Return the view pointer associated with the provided view type.
            AZ::RPI::ViewPtr GetView(ViewType viewType = ViewType::Default) const;

        private:
            uint32_t GetViewIndex(ViewType viewType);

            //! Vector to all the view data
            AZStd::array<ViewData, MaxViewTypes> m_cameraViews;

            //! XRRendering interface
            AZ::RPI::XRRenderingInterface* m_xrSystem = nullptr;

            //! Number of stereoscopic views
            AZ::u32 m_numSterescopicViews = 0;

            Descriptor m_desc;
        };
    } // namespace RPI
} // namespace AZ
