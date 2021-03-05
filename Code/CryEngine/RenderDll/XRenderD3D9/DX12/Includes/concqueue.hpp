/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

// Original file Copyright Crytek GMBH or its affiliates, used under license.
#ifndef __CONC_QUEUE_INCLUDED__
#define __CONC_QUEUE_INCLUDED__

#include "concqueue-mpmc-bounded.hpp"
#include "concqueue-mpsc.hpp"
#include "concqueue-spsc.hpp"
#include "concqueue-spsc-bounded.hpp"

#define BoundMPMC   concqueue::mpmc_bounded_queue_t
#define BoundSPSC   concqueue::spsc_bounded_queue_t
#define UnboundMPSC concqueue::mpsc_queue_t
#define UnboundSPSC concqueue::spsc_queue_t

template<template <typename> class queue, typename subject>
class ConcQueue
    : public queue<subject>
{
};

#endif
