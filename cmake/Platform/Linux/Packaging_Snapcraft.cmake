#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Copy snapcraft file

set(snap_file_name "o3de_${CPACK_PACKAGE_VERSION}_amd64")

if(CPACK_SNAP_DISTRO)
  configure_file("${LY_ROOT_FOLDER}/cmake/Platform/Linux/Packaging/snapcraft_${CPACK_SNAP_DISTRO}.yaml.in"
    "${CPACK_TEMPORARY_DIRECTORY}/snapcraft.yaml"
  )
  set(snap_file_name "o3de_${CPACK_PACKAGE_VERSION}_${CPACK_SNAP_DISTRO}_amd64")
else()
  configure_file("${LY_ROOT_FOLDER}/cmake/Platform/Linux/Packaging/snapcraft.yaml.in"
    "${CPACK_TEMPORARY_DIRECTORY}/snapcraft.yaml"
  )
endif()

execute_process (COMMAND lsb_release -a)

# Download python before packaging. The python runtimes themselves will not be part of the packaging, but
# the following script will also run through the scripts and create the necessary egg-link folders
# which will be installed into the package
execute_process (COMMAND bash ${CPACK_TEMPORARY_DIRECTORY}/O3DE/${CPACK_PACKAGE_VERSION}/python/get_python.sh)

# Since snap is immutable, we will pre-compile all the python scripts that is packaged into python byte-codes
set(O3DE_PY_SUBDIRS "Code;Gems;scripts;Tools")
foreach(engine_subfolder ${O3DE_PY_SUBDIRS})
    file(GLOB_RECURSE pyfiles "${CPACK_TEMPORARY_DIRECTORY}/O3DE/${CPACK_PACKAGE_VERSION}/${engine_subfolder}/*.py")
    foreach(pyfile ${pyfiles})
        execute_process (COMMAND bash ${CPACK_TEMPORARY_DIRECTORY}/O3DE/${CPACK_PACKAGE_VERSION}/python/python.sh -m py_compile ${pyfile})
    endforeach()
endforeach()


# Generate the files needed for the desktop icon
file(MAKE_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}/snap/gui/")
string(JOIN "\n" O3DE_DESKTOP_CONTENT
  "[Desktop Entry]"
  "Version=${CPACK_PACKAGE_VERSION}"
  "Name=O3DE"
  "Comment=O3DE Project Manager"
  "Type=Application"
  "Exec=o3de"
  "Icon=\${SNAP}/meta/gui/o3de.svg"
  "Terminal=false"
  "StartupWMClass=O3DE-Snap"
  "StartupNotify=true"
  "X-GNOME-Autostart-enabled=true"
)
file(WRITE "${CPACK_TEMPORARY_DIRECTORY}/snap/gui/o3de.desktop" ${O3DE_DESKTOP_CONTENT})
file(COPY_FILE
     "${LY_ROOT_FOLDER}/Code/Tools/ProjectManager/Resources/o3de.svg"
     "${CPACK_TEMPORARY_DIRECTORY}/snap/gui/o3de.svg"
)


# make sure that all files have the correct permissions
execute_process (COMMAND chmod -R 755 O3DE
                 WORKING_DIRECTORY ${CPACK_TEMPORARY_DIRECTORY}
)

# clean then build snap
execute_process (COMMAND snapcraft clean
                 WORKING_DIRECTORY ${CPACK_TEMPORARY_DIRECTORY}
)

execute_process (COMMAND snapcraft --verbose
                 WORKING_DIRECTORY ${CPACK_TEMPORARY_DIRECTORY}
)

set(snap_file "${CPACK_TEMPORARY_DIRECTORY}/o3de_${CPACK_PACKAGE_VERSION}_amd64.snap")

# Manually copy the files, the CPACK_EXTERNAL_BUILT_PACKAGES process runs after our packaging post build script
# which is too late to be uploaded.
file(COPY_FILE
     ${snap_file}
     "${CPACK_TOPLEVEL_DIRECTORY}/${snap_file_name}.snap"
)
