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

def main():

    documentId = azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(azlmbr.bus.Broadcast, 'CreateDocumentFromTypeName', 'Material')

    if documentId.IsNull():
        print("The material document could not be opened")
        return

    for path in azlmbr.atomtools.util.GetPathsInSourceFoldersMatchingWildcard('*.material'):
        if azlmbr.atomtools.util.IsDocumentPathEditable(path):
            if azlmbr.atomtools.AtomToolsDocumentRequestBus(azlmbr.bus.Event, 'Open', documentId, path):
                azlmbr.atomtools.AtomToolsDocumentRequestBus(azlmbr.bus.Event, 'Save', documentId)

    azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(azlmbr.bus.Broadcast, 'DestroyDocument', documentId)

if __name__ == "__main__":
    main()

