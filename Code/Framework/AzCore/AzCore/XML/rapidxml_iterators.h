/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZCORE_RAPIDXML_ITERATORS_H_INCLUDED
#define AZCORE_RAPIDXML_ITERATORS_H_INCLUDED

#define RAPIDXML_SKIP_AZCORE_ERROR

// the intention is that you only include the customized version of rapidXML through this header, so that
// you can override behavior here.
#include <rapidxml/rapidxml_iterators.h>

#undef INCLUDING_RAPIDXML_VIA_AZCORE

#endif // AZCORE_RAPIDXML_ITERATORS_H_INCLUDED
