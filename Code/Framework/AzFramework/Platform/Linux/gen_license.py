#!/bin/python3

#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import os
import subprocess
import xml.etree.ElementTree as ET

LICENSE_FILE_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "LICENSE_WL_PROTOCOLS.txt")
WAYLAND_PROTOCOLS = [
    "stable/xdg-shell/xdg-shell",
    "unstable/xdg-decoration/xdg-decoration-unstable-v1",
    "staging/cursor-shape/cursor-shape-v1",
    "stable/tablet/tablet-v2",
    "unstable/pointer-constraints/pointer-constraints-unstable-v1",
    "unstable/relative-pointer/relative-pointer-unstable-v1"
]

def does_pkgconfig_exist():
    """Check if pkg-config is installed."""
    try:
        subprocess.run(["pkg-config", "--version"], check=True, capture_output=True)
        return True
    except Exception:
        return False

def get_wl_protocols_dir():
    """Get the directory where wayland protocols are installed."""
    result = subprocess.run(["pkg-config", "--variable=pkgdatadir", "wayland-protocols"], capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError("Failed to get wayland-protocols pkgdatadir")

    # The output usually has a double slash at the start
    if result.stdout.startswith("//"):
        result.stdout = result.stdout[1:]
    return result.stdout.strip()

def add_protocol(protocol_location, output_file):
    """Extracts the needed info from the protocol XML file and writes it to the output file."""
    with open(protocol_path, "r") as file:
        xml_content = file.read()
        root = ET.fromstring(xml_content)
        protocol_name = root.get("name")
        copyright_node = root.find("copyright")
        copyright = copyright_node.text
        # Write the copyright to the output file
        output_file.write(f"Protocol {protocol_name}:\n")
        output_file.write(f"{copyright}\n")
        output_file.write("\n")


if __name__ == "__main__":
    if not does_pkgconfig_exist():
        print("pkg-config is not installed.")
        exit(1)

    wl_protocols_dir = get_wl_protocols_dir()
    print(f"Wayland protocols directory: {wl_protocols_dir}")
    print(f"Output license file:  {LICENSE_FILE_PATH}")

    output_file = open(LICENSE_FILE_PATH, "w")

    for protocol in WAYLAND_PROTOCOLS:
        protocol_path = os.path.join(wl_protocols_dir, protocol) + ".xml"

        if os.path.exists(protocol_path):
            add_protocol(protocol_path, output_file)
        else:
            raise RuntimeError(f"Protocol file does not exist: {protocol_path}")
