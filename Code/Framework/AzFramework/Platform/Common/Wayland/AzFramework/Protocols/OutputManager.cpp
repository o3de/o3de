/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OutputManager.h"

#include <AzFramework/WaylandProtocolNames.h>

namespace AzFramework
{
    wl_output_listener OutputManager::s_outputListener = {
        .geometry = OutputGeometry,
        .mode = OutputMode,
        .done = OutputDone,
        .scale = OutputScale,
        .name = OutputName,
        .description = OutputDesc
    };

    OutputManager::OutputManager()
    {
        WaylandRegistryEventsBus::Handler::BusConnect();
    }

    OutputManager::~OutputManager()
    {
        WaylandRegistryEventsBus::Handler::BusDisconnect();
    }

    void OutputManager::OnRegister(wl_registry* registry, uint32_t id, AZ::Crc32 interface, uint32_t version)
    {
        if (interface != WaylandOutputName)
        {
            return;
        }

        wl_output* output = static_cast<wl_output*>(wl_registry_bind(registry, id, &wl_output_interface, version));

        auto info = new OutputInfo();
        info->m_output = output;
        info->m_id = id;

        wl_output_set_user_data(output, info);
        wl_output_add_listener(output, &s_outputListener, info);
        m_outputs.emplace(id, info);
    }

    void OutputManager::OnUnregister(wl_registry*, uint32_t id)
    {
        if (m_outputs.find(id) == m_outputs.end())
        {
            return;
        }

        auto info = m_outputs[id];
        m_outputs.erase(id);

        wl_output_destroy(info->m_output);

        delete info;
    }

    uint32_t OutputManager::GetRefreshRateMhz(wl_output* output)
    {
        OutputInfo* info = static_cast<OutputInfo*>(wl_output_get_user_data(output));
        if (info == nullptr)
        {
            return 0;
        }

        return info->m_refreshRateMhz;
    }

    AZStd::string OutputManager::GetOutputName(wl_output* output)
    {
        OutputInfo* info = static_cast<OutputInfo*>(wl_output_get_user_data(output));
        if (info == nullptr)
        {
            return {};
        }

        return info->m_name;
    }

    AZStd::string OutputManager::GetOutputDesc(wl_output* output)
    {
        OutputInfo* info = static_cast<OutputInfo*>(wl_output_get_user_data(output));
        if (info == nullptr)
        {
            return {};
        }

        return info->m_desc;
    }

    void OutputManager::OutputGeometry(void* data, wl_output*, int32_t x, int32_t y,
                                       int32_t physical_width, int32_t physical_height, int32_t subpixel,
                                       const char* make, const char* model,
                                       int32_t transform)
    {
        //data is guaranteed to be a valid OutputInfo pointer, OutputManager controls the lifetime of our wayland
        //output proxy objects and ensures the data pointer is only deleted when the proxy is destroyed.
        auto self = static_cast<OutputInfo*>(data);
        self->m_x = x;
        self->m_y = y;
        self->m_physicalWidth = physical_width;
        self->m_physicalHeight = physical_height;
        self->m_subpixel = static_cast<wl_output_subpixel>(subpixel);
        self->m_make = AZStd::string(make);
        self->m_model = AZStd::string(model);
        self->m_transform = static_cast<wl_output_transform>(transform);
    }

    void OutputManager::OutputMode(void* data, wl_output*, uint32_t flags, int32_t width,
                                   int32_t height, int32_t refresh)
    {
        auto self = static_cast<OutputInfo*>(data);
        if (flags & wl_output_mode::WL_OUTPUT_MODE_CURRENT)
        {
            // We only really care for the current mode.
            self->m_width = width;
            self->m_height = height;
            self->m_refreshRateMhz = refresh;
        }
    }

    void OutputManager::OutputDone(void*, wl_output*)
    {
        //Required no-op, libwayland doesn't null check listener callbacks.
    }

    void OutputManager::OutputScale(void* data, wl_output*, int32_t factor)
    {
        auto self = static_cast<OutputInfo*>(data);
        self->m_scaleFactor = factor;
    }

    void OutputManager::OutputName(void* data, wl_output*, const char* name)
    {
        auto self = static_cast<OutputInfo*>(data);
        self->m_name = AZStd::string(name);
    }

    void OutputManager::OutputDesc(void* data, wl_output*, const char* description)
    {
        auto self = static_cast<OutputInfo*>(data);
        self->m_desc = AZStd::string(description);
    }
} // namespace AzFramework
