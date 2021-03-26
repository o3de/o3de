#!/bin/sh

#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# Original file Copyright Crytek GMBH or its affiliates, used under license.
#

(cd ../../CryEngine; find CryCommon -type d | sort -r) >tmp.dirs
(cd ../../CryEngine; find CryAction -type d | sort -r) >>tmp.dirs

awk '{  group = $1;
    gsub(/[\.\/]/,"_",group);
    print $1 ": " group
}' tmp.dirs >group-filter.txt

awk '{  group = $1;
    gsub(/[\.\/]/,"_",group);
    M = split($1,parts,"/");
    parent = parts[1]
    for (i = 2; i < M; i++) {
        parent = parent "_" parts[i]
    }
    print "/*! @defgroup " group " " parts[M]
    print " *  @ingroup " parent
    print " */"
}' tmp.dirs >doc-inputs/auto-groups.dox
rm tmp.dirs
