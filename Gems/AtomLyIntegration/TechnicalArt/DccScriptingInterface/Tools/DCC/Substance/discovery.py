
# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! @brief
Module Documentation:
    < DCCsi >:: Tools//DCC//Substance//discovery.py

This module handles discovery of the Substance3D installation.

(Not Implemented): This module is stubbed but not implemented yet.

# To Do: the approach Substance suggests is discovery via the registry
# Substance docs: https://substance3d.adobe.com/documentation/sddoc/retrieving-the-installation-path-172823228.html

Tenets:
1. Attempt to find all possible installations
2. Be cross-platform (eventually, dccsi is still prototype)
3. Present User will all found installations
4. Attempt to identify the latest/newest version, present to user as suggestion
   (unless it is a version not yet supported, in which case inform them)
5. Allow the search path and or exe to be explicit

Why?:
1. Game teams develop pipelines and for business reasons may lock DCC app versions for years.
2. Major DCC version jumps may alter APIs, we as developers and game teams need migration time.
3. Some studios may occasionally have custom/bespoke application builds
   (I have for example used a bespoke build of Maya at several dev studios working directly with Autodesk.)
4. Be flexible and help users decide instead of deciding for them

How?:
1. Search the latest known default installation path(s)
2. Check the registry, may be non-standard install path
3. Grep for multiple versions, compare versions, etc
4. Let User tell you where it is ...
"""
# -------------------------------------------------------------------------

# This module is not yet implemented, this is a placeholder.
