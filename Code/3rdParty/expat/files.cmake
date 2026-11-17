#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set(FILES
    "${expat_SOURCE_DIR}/expat/lib/xcs.c"
    "${expat_SOURCE_DIR}/expat/lib/xmlparse.c"
    "${expat_SOURCE_DIR}/expat/lib/xmlrole.c"
    "${expat_SOURCE_DIR}/expat/lib/xmltok.c"
)

# xmltok_impl.c and xmltok_ns.c are included by xmltok.c, not compiled separately.
if(HAVE_ARC4RANDOM)
    list(APPEND FILES "${expat_SOURCE_DIR}/expat/lib/random_arc4random.c")
endif()

if(HAVE_ARC4RANDOM_BUF)
    list(APPEND FILES "${expat_SOURCE_DIR}/expat/lib/random_arc4random_buf.c")
endif()

if(WIN32)
    list(APPEND FILES "${expat_SOURCE_DIR}/expat/lib/random_rand_s.c")
else()
    list(APPEND FILES "${expat_SOURCE_DIR}/expat/lib/random_dev_urandom.c")
endif()

if(HAVE_GETENTROPY)
    list(APPEND FILES "${expat_SOURCE_DIR}/expat/lib/random_getentropy.c")
endif()

if(HAVE_GETRANDOM OR HAVE_SYSCALL_GETRANDOM)
    list(APPEND FILES "${expat_SOURCE_DIR}/expat/lib/random_getrandom.c")
endif()
