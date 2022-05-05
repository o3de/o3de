```yaml
Copyright (c) Contributors to the Open 3D Engine Project.For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
```

# DccScriptingInterface ( aka DCCsi )

The *DccScriptingInterface* is a Gem for O3DE to extend and interface with dcc tools in the python ecosystem. Each dcc tool may have it's own specific version of python, most are some version of py3+. O3DE provides an install of py3+ and manages package dependancies with requirements.txt files and the cmake build system.

## What is the DCCsi?

- A shared Development Environment for technical art.
- Leverage the existing Python Ecosystem for technical art.
- Work with Python across a number of DCC tools as well as O3DE.
- Integrate a DCC app like Substance (or Substance SAT api) from the Python driven VFX and Games ecosystem.
- Extend O3DE and unlock its potential for content creators, and the Technical Artists that service them.

### Tenets:

1. Interoperability: Design DCC-agnostic modules and DCC-bespoke modules  to work together efficiently and intuitively.

2. Encapsulation: Define a module in terms of its essential features and interface to other components, to facilitate logical layered design and easy maintenance.

3. Extensibility: Design the tool set to be easily extensible with new functionality, along with new utilities and functionality.  Individual pieces should have a generic communication mechanism to allow newly written tools to slot cleanly and transparently into the tool chain.

### What is provided (High Level):

DCC-Agnostic Python Framework (as a modular Gem) related to multiple integrations for:

- O3DE Editor extensibility (python scripting, utils and PySide2 tools)

- DCC applications, O3DE related extensibility using their Python APIs/SDKs

- Custom standalone tools and utils (python based) to improve O3DE workflows

### DCCsi Status:

The DCCsi is currently still concidered experimental and an early prototype, mainly because the functionality is still barebones, and the scaffolding is still being routinely refactored to get core patterns in place.

## Getting Started

To Do: document
