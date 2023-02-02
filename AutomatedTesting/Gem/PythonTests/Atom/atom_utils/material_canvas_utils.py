"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.atomtools
import azlmbr.math as math
import azlmbr.bus as bus


def get_graph_name(document_id: math.Uuid) -> str:
    """
    Gets the graph name of the given document_id and returns it as a string.
    :param document_id: The UUID of a given document file.
    :return: str representing the graph name of document_id
    """
    return azlmbr.atomtools.GraphDocumentRequestBus(bus.Event, "GetGraphName", document_id)
