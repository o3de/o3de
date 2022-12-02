"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import sys
import os
import azlmbr.bus
import azlmbr.paths
import collections
import random

def main():
    paths = azlmbr.atomtools.util.GetPathsInSourceFoldersMatchingWildcard('*.materialgraph')
    for path in paths.copy():
        if 'cache' in path.lower():
            paths.remove(path)

    for i in range(0, 100):
        for path in paths:
            if azlmbr.atomtools.util.IsDocumentPathEditable(path):
                azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(azlmbr.bus.Broadcast, 'OpenDocument', path)
        azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(azlmbr.bus.Broadcast, 'CloseAllDocuments')
        random.shuffle(paths)

if __name__ == "__main__":
    main()

