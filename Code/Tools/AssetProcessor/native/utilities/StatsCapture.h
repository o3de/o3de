/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// This is an AssetProcessor-only stats capture system.  Its kept out-of-band
// from the rest of the Asset Processor systems so that it can avoid interfering
// with the rest of the processing decision making and other parts of AssetProcessor.
// This is not meant to be used anywhere except in AssetProcessor.

#pragma once

#include <AzCore/std/string/string_view.h>

namespace AssetProcessor
{
    namespace StatsCapture
    {
        //! call this one time before capturing stats.
        void Initialize();

        //! Call this one time as part of shutting down.
        void Shutdown();

        //! Start the clock running for a particular stat name.
        void BeginCaptureStat(AZStd::string_view statName);

        //! Stop the clock running for a particular stat name.
        void EndCaptureStat(AZStd::string_view statName);

        //! Do additional processing and then write the cumulative stats to log.
        //! Note that since this is an AP-specific system, the analysis done in the dump function
        //! is going to make a lot of assumptions about the way the data is encoded.
        void Dump();
    }
    
}
