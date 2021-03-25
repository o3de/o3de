/***************************************************************************
 * Copyright 1998-2018 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxCoreRender.                                   *
 *                                                                         *
 * Licensed under the Apache License, Version 2.0 (the "License");         *
 * you may not use this file except in compliance with the License.        *
 * You may obtain a copy of the License at                                 *
 *                                                                         *
 *     http://www.apache.org/licenses/LICENSE-2.0                          *
 *                                                                         *
 * Unless required by applicable law or agreed to in writing, software     *
 * distributed under the License is distributed on an "AS IS" BASIS,       *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
 * See the License for the specific language governing permissions and     *
 * limitations under the License.                                          *
 ***************************************************************************/

// NOTE: this file is included in LuxCore so any external dependency must be
// avoided here

#ifndef _LUXRAYS_OPENCLERROR_H
#define _LUXRAYS_OPENCLERROR_H

#include "luxrays/utils/exportdefs.h"
#include "luxrays/utils/ocl.h"

#if !defined(LUXRAYS_DISABLE_OPENCL)

namespace luxrays {

// Same utility function

CPP_EXPORT CPP_API std::string oclErrorString(cl_int error);

}

#endif

#endif  /* _LUXRAYS_OPENCLERROR_H */
