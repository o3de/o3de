/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Utilities/RandomNumberGenerator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AzQtComponents
{
    namespace
    {
        thread_local AZStd::unique_ptr<QRandomGenerator> generator;
    } // anonymous namespace

    QRandomGenerator* GetRandomGenerator()
    {
        if (!generator)
        {
            generator.reset(new QRandomGenerator(QRandomGenerator::system()->generate()));
        }
        return generator.get();
    }
} // namespace AzQtComponents
