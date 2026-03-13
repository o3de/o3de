/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzFramework/WaylandInterface.h>

#include <wayland-client.h>

namespace AzFramework
{
    class OutputManagerRequests
    {
    public:
        AZ_RTTI(OutputManagerRequests, "{8EEBB5C3-91BA-4AE8-8529-6F0FE69E7E9B}");
        virtual ~OutputManagerRequests() = default;

        virtual uint32_t GetRefreshRateMhz(wl_output* output) = 0;
        virtual AZStd::string GetOutputName(wl_output* output) = 0;
        virtual AZStd::string GetOutputDesc(wl_output* output) = 0;
    };

    using OutputManagerInterface = AZ::Interface<OutputManagerRequests>;

    class OutputManager
        : protected WaylandRegistryEventsBus::Handler
        , protected OutputManagerInterface::Registrar
    {
    public:
        OutputManager();
        ~OutputManager() override;

        //WaylandRegistryEventsBus
        void OnRegister(wl_registry* registry, uint32_t id, AZ::Crc32 interface, uint32_t version) override;
        void OnUnregister(wl_registry*, uint32_t id) override;

        //OutputManagerInterface
        uint32_t GetRefreshRateMhz(wl_output* output) override;
        AZStd::string GetOutputName(wl_output* output) override;
        AZStd::string GetOutputDesc(wl_output* output) override;

    private:
        static void OutputGeometry(
            void* data,
            struct wl_output* wl_output,
            int32_t x,
            int32_t y,
            int32_t physical_width,
            int32_t physical_height,
            int32_t subpixel,
            const char* make,
            const char* model,
            int32_t transform);
        static void OutputMode(void* data, struct wl_output* wl_output, uint32_t flags, int32_t width, int32_t height, int32_t refresh);
        static void OutputDone(void* data, struct wl_output* wl_output);
        static void OutputScale(void* data, struct wl_output* wl_output, int32_t factor);
        static void OutputName(void* data, struct wl_output* wl_output, const char* name);
        static void OutputDesc(void* data, struct wl_output* wl_output, const char* description);

        static wl_output_listener s_outputListener;

        struct OutputInfo
        {
            wl_output* m_output = nullptr;
            uint32_t m_id = 0;
            int32_t m_x = 0;
            int32_t m_y = 0;
            int32_t m_width = 0;
            int32_t m_height = 0;
            int32_t m_refreshRateMhz = 0;
            int32_t m_physicalWidth = 0;
            int32_t m_physicalHeight = 0;
            wl_output_subpixel m_subpixel = {};
            AZStd::string m_make = "";
            AZStd::string m_model = "";
            wl_output_transform m_transform = {};

            AZStd::string m_name = "";
            AZStd::string m_desc = "";

            int32_t m_scaleFactor = 0;
        };

        AZStd::unordered_map<uint32_t, OutputInfo*> m_outputs = {};
    };
} // namespace AzFramework
