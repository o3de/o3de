----

Copyright (c) Contributors to the Open 3D Engine Project.  For complete copyright and license terms please see the LICENSE at the root of this distribution.
SPDX-License-Identifier: Apache-2.0 OR MIT

----

# O3DE DCCsi and Wing Pro IDE

###### Status: Prototype

###### Version: 0.0.1

###### Support: <u>Wing Pro 8+</u>, currently Windows only (other not tested but may work)

- Not yet tested in Installer builds (I am a developer building from source), more robust support for end users is being worked on for next release (2210)

- Supports O3DE Python as Launch Configuration

- Supports Blender Python as a Launch Configuration

- Others (like Maya) WIP

## TL/DR

To get started, you can launch Wing from a Win CMD prompt, first make sure you are

```shell
# First, ensure that your engine build has O3DE Python setup!
> cd C:\path\to\o3de\
> get_python.bat

# change to the dccsi root
cd C:\path\to\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface
# python.cmd in this folder, wraps o3de python
> python.cmd Tools\IDE\Wing\start,py
```

### Coming Soon

- Start from O3DE Editor menu

## Overview

This document contains instructions and other information related to using Wing IDE with Open 3D Engine (O3DE). [ [WingIDE Link](https://wingware.com/?gclid=Cj0KCQjwxIOXBhCrARIsAL1QFCb2FuL_fHPRqkrN-4voBg0q7VMuNaFPWxAbnTxjv4diNm5kMBPwVhQaAv4yEALw_wcB) ] [ [Wing Docs Link](https://wingware.com/doc/TOC) ]

O3DE has a Gem called the DccScriptingInterface (DCCsi) that helps user manage the usage of many popular Digital Content Creation tools (DCC) such as 3D modeling tools like Blender (and others.)

Most modern DCC tools come with a Python interpreter and APIs to customize and extend the editor experience.  Likewise, O3DE has a Python Interpreter and APIs for automation.  Many professional game studios use this to create custom tools, workflows, pipelines and automation for content creation.  Technical Artists (TAs) often are the discipline doing this type of development work, and utilize Python to do so, but may not be as fluent in developer tools as a C++ engineer; thus there is an importance on having good development environments.  

The DCCsi integrations are workflow integrations with the intent to provide an improved ease-of-use experience when these tools are used to author source content for O3DE.  The same can be said for IDEs and custom tooling. These novice to intermediate TAs that want to just jump in a start creating, but don't know how to get started, are the target audience for this integration... the goal is to get you up and running with minimal set up.

The DCCsi helps with aspects such as, configuration and settings, launching DCC tools, and IDEs, and bootstrapping them with O3DE extensions (generally python scripts using the DCC tools APIs to automate tasks)

# Getting Started

Each IDE is different.  Some TAs (like me) like Wing IDE because it's relatively easy to set up and works great with DCC apps like Maya.  It's also pretty powerful and you can do a lot of data-driven configuration with it (which we will be covering some of.)  We aren't picking an IDE for you, that's a personal choice, we are just showing you how one like Wing can be set up to provide a better out-of-box experience (so that you don't have to do the setup and configuration yourself.)  In fact, we intend to do the same for other IDS like PyCharm and VScode (maybe others in the future.)

To use Wing, there are a few things that you need to do locally to get set up (and we may automate some of these steps in the future.)  

## What to know ...

The default location for the DCCsi Gem folder is something like:

`C:/path/to/o3de/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface`

But you are reading this, so you have already found it :)

### General

There are many ways to work with O3DE, especially as a developer. Here are some things to be aware of...

- What type of user are you?  Are you a C++ engine developer, a Technical Artist (TA) working in source, or a TA on a game team using a pre-built engine; or a end user that is a novice with little python experience?

- O3DE has it's own python interpreter, the python version and/or location could change as upgrades happen. We may want to use this O3DE python as the IDE interpreter when running our code (likewise we may want to use a compatible  DCC tools interpreter); and this is one reason why the env is data-driven and configurable.)

- O3DE builds it's own Qt dlls (and other binaries), provides it's own access to PySide2, installs python package dependencies during cmake configuration; we need to ensure we are bootstrapping access to these things, so we are developing and testing code with the same interpreter, using the same packages and code access, and operating in an environment similar to the editor! (and there are other ways to operate, like creating a standalone PySide2 gui application. A topic for another time perhaps.)

- We also want to interface with a number of DCC applications, each with their own python interpreters and configuration concerns.  So we might want to use those python.exe's as the IDE interpreter, access their APIs, and manage compatible package dependencies; we might have common shared code running across these tools ... things can get out of hand and rather confusing quickly (this IDE integration attempts to solve some of this nightmare for you.)

- Are you using the O3DE installer? The release, or the nightly? What platform are you on?  All of these decisions have an impact on development as well, and we want to be able to configure our development environments in a way that deals with the differences.
  
  - These will have different install locations (and these locations have changed over time as the engine matures.)
    
    - Nightly:  `C:\O3DE\0.0.0.0`
    
    - Release: `C:\O3DE\22.05.0`
    
    - You can customize the install location...
  
  - The installer is pre-built, so it has a known folder structure like this:
    
    - `C:\O3DE\0.0.0.0\bin\Windows\profile\Default\editor.exe`

- But you might be building from source.  You might have multiple engine repos cloned locally, they may be on different branches.  You can build in an engine-centric way, or a project-centric way, and developers choose their cmake configuration and build folder.  Configuration needs to account for these many options.  This guide is geared towards developers, and developers need to be aware of these things.

- Be aware of your development set up. Here is an example of the location I work with the most locally on a day-to-day bases.  Generally I only work on windows, I build in an engine-centric way, because I branch hop a lot ... and I test branches with multiple projects I am also working on. I keep it simple and easy so I can do these things most efficiently.
  
  - `C:\depot\o3de-dev\build\bin\profile\editor.exe`

- This is not a guide for actually developing python code and tools for the O3DE editor (that does exist however); this guide is focused on how Wing IDE is setup.

- This guide likewise isn't focus very much on DCC tools or their configuration, however it does touch on these topics as they are concerned with Wing, like picking Blender, or MayaPy.exe as your IDE interpreter (aka Launch Configuration)

- This is not an O3DE build guide, it assumes you either have built from source (and know where that is!), or that you have a pre-built install (and likewise know where that is located.)

### Specifics

Location of the DCCsi gem folder:

    `C:\path\to\o3de\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface`

In the root of the DCCsi, one of the files will start O3DE python, you can start here for running and testing code outside of an IDE or the Editor:

   ` DccScriptingInterface\python.cmd`

The actual interpreter that runs is somewhere like

    `o3de\python\runtime\python-3.7.12-rev2-windows\python\python.exe`

The folder for Wing developers is here:

    `DccScriptingInterface\Tools\IDE\Wing`

It's this last location we will be most concerned with.

There is also a windows .bat launcher for Wing in this location:

    `DccScriptingInterface\Tools\IDE\Wing\win_launch_wingide.bat`

This is an alternative to launching from the editor menu, or using the scripted approach of starting wing from the dccsi cmd:

    `dccsi > python.cmd Tools\IDE\Wing\start.py`

Development is a catch-22, sometimes the script framework is buggy and broken (or simply a change is wip), and you need a reliable fallback for your dev environment.

# Setup

Here are some set up instructions that hopefully get you up and running.

## Configure O3DE .bat Env:

This section is for developers who desire to configure and start via the .bat file

    `DccScriptingInterface\Tools\IDE\Wing\win_launch_wingide.bat`

To make sure we can alter the Wing environment, to work with your set up, you will want to locate this file:

    `DccScriptingInterface\Tools\IDE\WingIDE\Env_Dev.bat.example`

Then you will want to copy it, and rename the copy:

    `DccScriptingInterface\Tools\IDE\WingIDE\Env_Dev.bat`

What this file provides, is the ability and opportunity to make envar settings and overrides prior to starting WingIDE from the .bat file launcher.  If you have already found that the launcher didn't work for you, this is a section you will want to pay attention to.

You may find the following ENVARs useful to set

to do: document the envars

## Configure O3DE dynamic Env

There is a python driven approach to configuration, settings, bootstrapping and launching apps like Wing.  The files we will be most concerned with are:

The primary settings file, which is distributed.  This has default settings defined.

    `DccScriptingInterface\Tools\IDE\Wing\settings.json`

This is a secondary settings file that can be manually made, or generated, for developers.  This file allows a developer to make local overrides to envars and settings.  These overrides take precedence.

   ` DccScriptingInterface\Tools\IDE\Wing\settings.local.json`

to do: ... describe how to generate the settings.local.json

to do: ... describe 

to do: ... use the start.py script to launch Wing

### wingstub.py (for debugging)

This is necessary for attaching Wing as the debugger in scripts that are running

1. Locate the DCCsi gem folder (see above)

2. Locate your Wing IDE install, the default location is somewhere like:
   
   1. C:\Program Files (x86)\Wing Pro 8
   
   2. Locate: "C:\Program Files (x86)\Wing Pro 8\wingdbstub.py"

3. Copy the file wingdbstub.py

4. It needs to be copied somewhere on the PYTHONPATH, for instance you could copy it here:  "DccScriptingInterface\Tools\IDE\WingIDE\wingdbstub.py"

5. Open the wingdbstub.py file and modify line 96 to
   
   1. **kEmbedded = 1**

Notes: 

- If you install a new version of Wing, you should check the new version to see if the wingdbstub.py file has changed (use a diff tool?); if it has do the steps above again.

- For debugging to work, on windows you likely will need to add the wing executable to **Windows Defender Firewall**. When you start wing for the first time it may prompt you to do this, otherwise may need to do it manually (see **HELP** below)

## HELP

Trouble attaching debugger?  Open your firewall and add an exception for wing.

- In Windows 10:  
  - Win + S and search for **Windows Defender Firewall**
  - Select **Allow an app or feature through Windows Defender Firewall**
- In the allowed apps window:
  - Verify there isn't already an entry for Wing by scrolling through
  - Click on **Change Settings**
  - Then click on **Allow another app...**
- In the Add an app window:
  - click on **Browse...** and point it to your wing executable
    - C:\Program Files (x86)\Wing Pro 8\bin\wing.exe
  - click on **Add**

# Revision Info:

- This is the second working Proof of Concept. The first was a windows only .bat file based env and launchers. This new integration replaces that with data-driven configuration and settings, with python scripting to bootstrap and launch. (this provides a path to starting from within the main o3de Editor via a PySide2 menu)

- This iteration is developer focused, such as a Technical Artist. It does not have some of the end-user functionality or creature comforts that you might want or expect, yet.

- This version is only the integration patterns for configuration, settings, launch and bootstrapping. No additional tooling within Wing IDE is implemented yet, however Wing has it's own API and extensibility, so this could be an area for future work.

- Currently only the latest version of Wing Pro 8 has been tested: 8.3.2.0 (rev 9d633cb1c4a7), Release (June 17, 2022).  We expect any version of Wing Pro8 to work fine, inform us and help get it fixed if it doesn't.

- Wing 7.x was previously supported, but it was python2.7 based and we are deprecating support for py2.7, and this includes deprecation of support for apps that are pre-py3 bound.

- Previous versions of Wing may still work, however you will need to configure and/or modify the env yourself. This would also include version such as Wing Community Edition.
