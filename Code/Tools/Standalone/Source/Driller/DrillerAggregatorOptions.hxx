/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_DRILLERAGGREGATOR_OPTIONS_H
#define DRILLER_DRILLERAGGREGATOR_OPTIONS_H

#include "QtGui/qcolor.h"

namespace Driller
{
    class Aggregator;

    class AggregatorOptions
    {
    public:

        AggregatorOptions( Aggregator *owner )
            : m_Owner(owner)
        {
        }
        virtual ~AggregatorOptions()
        {
        }

        Aggregator *m_Owner;
    };
}

#endif //DRILLER_DRILLERAGGREGATOR_OPTIONS_H
