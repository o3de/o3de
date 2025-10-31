/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <AzCore/Name/Name.h>

namespace AZ
{
    namespace RPI
    {
        // Helper functions for building paths
        // For example if a pass has a path 'Root.Window1' and it's child pass is 'Forward'
        // we can use these concatenation functions to get 'Root.Window1.Forward'

        ATOM_RPI_REFLECT_API inline AZStd::string ConcatPassString(const AZStd::string_view& first, const AZStd::string_view& second)
        {
            return AZStd::string::format("%.*s.%.*s", static_cast<int>(first.length()), first.data(), static_cast<int>(second.length()), second.data());
        }

        ATOM_RPI_REFLECT_API inline AZStd::string ConcatPassString(const Name& first, const Name& second)
        {
            return ConcatPassString(first.GetStringView(), second.GetStringView());
        }

        ATOM_RPI_REFLECT_API inline Name ConcatPassName(const Name& first, const Name& second)
        {
            return Name(ConcatPassString(first, second));
        }
    }

}

