/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/ViewGroup.h>
#include <AzCore/Debug/Trace.h>

namespace AZ::RPI
{
    void ViewGroup::Init(const Descriptor& desc)
    {
        m_desc = desc;

        if (auto rpiSystemInterface = RPISystemInterface::Get())
        {
            m_xrSystem = rpiSystemInterface->GetXRSystem();
            if (m_xrSystem)
            {
                m_numSterescopicViews = m_xrSystem->GetNumViews();
                AZ_Assert(m_numSterescopicViews <= XRMaxNumViews, "Atom only supports two XR views");
            }
        }

        for (int i = 0; i < MaxViewTypes; i++)
        {
            m_cameraViews[i].m_onProjectionMatrixChangedHandler = MatrixChangedEvent::Handler(
                [this,i](const AZ::Matrix4x4& matrix)
                {
                    if (m_desc.m_projectionEventFunction)
                    {
                        m_desc.m_projectionEventFunction(m_cameraViews[i].m_view);
                    }
                    if (m_cameraViews[i].m_projectionMatrixChangedEvent.HasHandlerConnected())
                    {
                        m_cameraViews[i].m_projectionMatrixChangedEvent.Signal(matrix);
                    }   
                });

            m_cameraViews[i].m_onViewMatrixChangedHandler = MatrixChangedEvent::Handler(
                [this,i](const AZ::Matrix4x4& matrix)
                {
                    if(m_desc.m_viewEventFunction)
                    {
                        m_desc.m_viewEventFunction(m_cameraViews[i].m_view);
                    }
                    if (m_cameraViews[i].m_viewMatrixChangedEvent.HasHandlerConnected())
                    {
                        m_cameraViews[i].m_viewMatrixChangedEvent.Signal(matrix);
                    }
                });
        }
    }

    void ViewGroup::CreateMainView(AZ::Name name)
    { 
        if (m_cameraViews[DefaultViewType].m_view == nullptr)
        {
            m_cameraViews[DefaultViewType].m_view = AZ::RPI::View::CreateView(name, AZ::RPI::View::UsageFlags::UsageCamera);
        }
    }

    void ViewGroup::CreateStereoscopicViews(AZ::Name name)
    {
        if (m_xrSystem)
        {
            for (AZ::u32 i = 0; i < m_numSterescopicViews; i++)
            {
                uint32_t xrViewIndex =
                    i == 0 ? static_cast<uint32_t>(AZ::RPI::ViewType::XrLeft) : static_cast<uint32_t>(AZ::RPI::ViewType::XrRight);
                if (m_cameraViews[xrViewIndex].m_view == nullptr)
                {
                    AZ::Name xrViewName = AZ::Name(AZStd::string::format("%s XR %i", name.GetCStr(), i));
                    m_cameraViews[xrViewIndex].m_view =
                        AZ::RPI::View::CreateView(xrViewName, AZ::RPI::View::UsageCamera | AZ::RPI::View::UsageXR);
                }
            }
        }
    }

    void ViewGroup::Activate()
    {
        for (uint32_t i = 0; i < AZ::RPI::MaxViewTypes; i++)
        {
            if (m_cameraViews[i].m_view)
            {
                m_cameraViews[i].m_view->ConnectWorldToViewMatrixChangedHandler(m_cameraViews[i].m_onViewMatrixChangedHandler);
            }
        }
    }

    void ViewGroup::Deactivate()
    {
        for (int i = 0; i < AZ::RPI::MaxViewTypes; i++)
        {
            if (m_cameraViews[i].m_onViewMatrixChangedHandler.IsConnected())
            {
                m_cameraViews[i].m_onViewMatrixChangedHandler.Disconnect();
            }

            if (m_cameraViews[i].m_onProjectionMatrixChangedHandler.IsConnected())
            {
                m_cameraViews[i].m_onProjectionMatrixChangedHandler.Disconnect();
            }
        }
    }

    AZ::RPI::ViewPtr ViewGroup::GetView(ViewType viewType) const
    {
        uint32_t viewIndex = static_cast<uint32_t>(viewType);
        AZ_Assert(viewIndex < m_cameraViews.size(), "View Index %i out of range. Array size is %i ", viewIndex, m_cameraViews.size());
        return m_cameraViews[viewIndex].m_view;
    }

    void ViewGroup::SetView(const AZ::RPI::ViewPtr view, ViewType viewType)
    {
        uint32_t viewIndex = static_cast<uint32_t>(viewType);
        AZ_Assert(viewIndex < m_cameraViews.size(), "View Index %i out of range. Array size is %i ", viewIndex, m_cameraViews.size());
        m_cameraViews[viewIndex].m_view = view;
    }

    bool ViewGroup::IsAnyViewEnabled() const
    {
        for (const auto& view : m_cameraViews)
        {
            if (view.m_view)
            {
                return true;
            }
        }
        return false;
    }

    void ViewGroup::SetViewToClipMatrix(const AZ::Matrix4x4& viewToClipMatrix, ViewType viewType)
    {
        uint32_t viewIndex = GetViewIndex(viewType);
        if (m_cameraViews[viewIndex].m_view)
        {
            m_cameraViews[viewIndex].m_view->SetViewToClipMatrix(viewToClipMatrix);
        }
    }

    void ViewGroup::SetStereoscopicViewToClipMatrix(const AZ::Matrix4x4& viewToClipMatrix, bool reverseDepth, ViewType viewType)
    {
        uint32_t viewIndex = GetViewIndex(viewType);
        if (m_cameraViews[viewIndex].m_view)
        {
            m_cameraViews[viewIndex].m_view->SetStereoscopicViewToClipMatrix(viewToClipMatrix, reverseDepth);
        }
    }

    void ViewGroup::SetCameraTransform(const AZ::Matrix3x4& cameraTransform, ViewType viewType)
    {
        uint32_t viewIndex = GetViewIndex(viewType);
        if (m_cameraViews[viewIndex].m_view)
        {
            m_cameraViews[viewIndex].m_view->SetCameraTransform(cameraTransform);
        }
    }

    uint32_t ViewGroup::GetNumViews() const
    {
        return aznumeric_cast<uint32_t>(m_cameraViews.size());
    }

    void ViewGroup::ConnectViewMatrixChangedEvent(MatrixChangedEvent::Handler& handler, ViewType viewType)
    {
        uint32_t viewIndex = GetViewIndex(viewType);
        handler.Connect(m_cameraViews[viewIndex].m_viewMatrixChangedEvent);
    }

    void ViewGroup::ConnectProjectionMatrixChangedEvent(MatrixChangedEvent::Handler& handler, ViewType viewType)
    {
        uint32_t viewIndex = GetViewIndex(viewType);
        handler.Connect(m_cameraViews[viewIndex].m_projectionMatrixChangedEvent);
    }

    void ViewGroup::SignalViewMatrixChangedEvent(const AZ::Matrix4x4& matrix, ViewType viewType)
    {
        uint32_t viewIndex = GetViewIndex(viewType);
        m_cameraViews[viewIndex].m_viewMatrixChangedEvent.Signal(matrix);
    }

    void ViewGroup::SignalProjectionMatrixChangedEvent(const AZ::Matrix4x4& matrix, ViewType viewType)
    {
        uint32_t viewIndex = GetViewIndex(viewType);
        m_cameraViews[viewIndex].m_projectionMatrixChangedEvent.Signal(matrix);
    }

    uint32_t ViewGroup::GetViewIndex(ViewType viewType)
    {
        uint32_t viewIndex = static_cast<uint32_t>(viewType);
        AZ_Assert(viewIndex < m_cameraViews.size(), "View Index %i out of range. Array size is %i ", viewIndex, m_cameraViews.size());
        return viewIndex;
    }

    void ViewGroup::ConnectViewMatrixChangedHandler(ViewType viewType)
    {
        uint32_t viewIndex = GetViewIndex(viewType);
        m_cameraViews[viewIndex].m_view->ConnectWorldToViewMatrixChangedHandler(m_cameraViews[viewIndex].m_onViewMatrixChangedHandler);
    }

    void ViewGroup::ConnectProjectionMatrixChangedHandler(ViewType viewType)
    {
        uint32_t viewIndex = GetViewIndex(viewType);
        m_cameraViews[viewIndex].m_view->ConnectWorldToClipMatrixChangedHandler(m_cameraViews[viewIndex].m_onProjectionMatrixChangedHandler);
    }

    void ViewGroup::DisconnectViewMatrixHandler(ViewType viewType)
    {
        uint32_t viewIndex = GetViewIndex(viewType);
        m_cameraViews[viewIndex].m_onViewMatrixChangedHandler.Disconnect();
    }

    void ViewGroup::DisconnectProjectionMatrixHandler(ViewType viewType)
    {
        uint32_t viewIndex = GetViewIndex(viewType);
        m_cameraViews[viewIndex].m_onProjectionMatrixChangedHandler.Disconnect();
    }

    bool ViewGroup::IsViewInGroup(const AZ::RPI::ViewPtr view) const
    {
        for (const auto& groupView : m_cameraViews)
        {
            if (groupView.m_view == view)
            {
                return true;
            }
        }
        return false;
    }

    bool ViewGroup::IsViewGroupViewsSame(const AZ::RPI::ViewGroupPtr viewGroup) const
    {
        for (uint32_t i = 0; i < MaxViewTypes; i++)
        {
            if (m_cameraViews[i].m_view != viewGroup->GetView(static_cast<ViewType>(i)))
            {
                return false;
            }
        }
        return true;
    }
} // namespace AZ::RPI
