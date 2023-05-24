/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/Components/StyleBus.h>

DECLARE_EBUS_INSTANTIATION(GraphCanvas::StyleManagerNotifications);

namespace GraphCanvas
{
    AZStd::string StyledEntityRequests::GetFullStyleElement() const
    {
        AZStd::string element = GetElement();
        AZStd::string subStyle = GetClass();

        if (subStyle.empty())
        {
            return element;
        }
        else
        {
            return AZStd::string::format("%s.%s", element.c_str(), subStyle.c_str());
        }
    }

   
    void PaletteIconConfiguration::ClearPalettes()
    {
        m_colorPalettes.clear();
        m_paletteCrc = AZ::Crc32();
    }

    void PaletteIconConfiguration::ReservePalettes(size_t size)
    {
        m_colorPalettes.reserve(size);
    }

    void PaletteIconConfiguration::SetColorPalette(AZStd::string_view paletteString)
    {
        ClearPalettes();
        AddColorPalette(paletteString);
    }

    void PaletteIconConfiguration::AddColorPalette(AZStd::string_view paletteString)
    {
        m_colorPalettes.push_back(paletteString);
        m_paletteCrc.Add(paletteString.data());
    }

    const AZStd::vector< AZStd::string >& PaletteIconConfiguration::GetColorPalettes() const
    {
        return m_colorPalettes;
    }

    AZ::Crc32 PaletteIconConfiguration::GetPaletteCrc() const
    {
        return m_paletteCrc;
    }
} // namespace GraphCanvas
