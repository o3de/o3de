/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_MAESTRO_TYPES_SEQUENCETYPE_H
#define CRYINCLUDE_CRYCOMMON_MAESTRO_TYPES_SEQUENCETYPE_H
#pragma once

enum class SequenceType
{
    Legacy                = 0,        // legacy CryEntity Sequence Object
    SequenceComponent     = 1         // Sequence Component on an AZ::Entity
};

#endif // CRYINCLUDE_CRYCOMMON_MAESTRO_TYPES_SEQUENCETYPE_H
