/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

namespace MCore
{
    // default memory alignment in bytes
    enum
    {
        MCORE_DEFAULT_ALIGNMENT = 4,     /**< The default memory address alignment of objects, in bytes. */
        MCORE_SIMD_ALIGNMENT    = 16
    };


    // memory categories
    enum
    {
        MCORE_MEMCATEGORY_UNKNOWN           = 0,
        MCORE_MEMCATEGORY_ARRAY             = 1,
        MCORE_MEMCATEGORY_STRING            = 2,
        MCORE_MEMCATEGORY_DISKFILE          = 3,
        MCORE_MEMCATEGORY_MEMORYFILE        = 4,
        MCORE_MEMCATEGORY_MATRIX            = 5,
        MCORE_MEMCATEGORY_HASHTABLE         = 6,
        MCORE_MEMCATEGORY_TRILISTOPTIMIZER  = 7,
        MCORE_MEMCATEGORY_LOGMANAGER        = 9,
        MCORE_MEMCATEGORY_COMMANDLINE       = 10,
        MCORE_MEMCATEGORY_LOGFILECALLBACK   = 13,
        MCORE_MEMCATEGORY_HALTONSEQ         = 14,
        MCORE_MEMCATEGORY_SMALLARRAY        = 15,
        MCORE_MEMCATEGORY_COORDSYSTEM       = 16,
        MCORE_MEMCATEGORY_MCORESYSTEM       = 17,
        MCORE_MEMCATEGORY_COMMANDSYSTEM     = 18,
        MCORE_MEMCATEGORY_ATTRIBUTES        = 20,
        MCORE_MEMCATEGORY_IDGENERATOR       = 21,
        MCORE_MEMCATEGORY_WAVELETS          = 22,
        MCORE_MEMCATEGORY_HUFFMAN           = 23,
        MCORE_MEMCATEGORY_ABSTRACTDATA      = 26,
        MCORE_MEMCATEGORY_SYSTEM            = 27,
        MCORE_MEMCATEGORY_THREADING         = 29,
        MCORE_MEMCATEGORY_ATTRIBUTEPOOL     = 32,
        MCORE_MEMCATEGORY_ATTRIBUTEFACTORY  = 33,
        MCORE_MEMCATEGORY_RANDOM            = 34,
        MCORE_MEMCATEGORY_STRINGOPS         = 35,
        MCORE_MEMCATEGORY_FRUSTUM           = 36,
        MCORE_MEMCATEGORY_STREAM            = 37,
        MCORE_MEMCATEGORY_MULTITHREADMANAGER= 38,
        MCORE_MEMCATEGORY_TRIANGULATOR      = 39,
        // insert new categories here
        MCORE_MEMCATEGORY_MISC              = 99
    };
}   // namespace MCore
