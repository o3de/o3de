#!/bin/bash
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

###############################################################
# Clone and bootstrap O3DE
###############################################################
echo "Cloning o3de"
git clone --single-branch -b $O3DE_BRANCH $O3DE_REPO $O3DE_ROOT && \
    git -C $O3DE_ROOT lfs install && \
    git -C $O3DE_ROOT lfs pull && \
    git -C $O3DE_ROOT reset --hard $O3DE_COMMIT 
if [ $? -ne 0 ]
then
    echo "Error cloning o3de from $O3DE_REPO"
    exit 1
fi

$O3DE_ROOT/python/get_python.sh 
if [ $? -ne 0 ]
then
    echo "Error bootstrapping O3DE from $O3DE_REPO"
    exit 1
fi

export O3DE_BUILD=$WORKSPACE/build

export CONFIGURATION=profile
export OUTPUT_DIRECTORY=$O3DE_BUILD
export O3DE_PACKAGE_TYPE=DEB
export CMAKE_OPTIONS="-G 'Ninja Multi-Config' -DLY_PARALLEL_LINK_JOBS=4 -DLY_DISABLE_TEST_MODULES=TRUE -DO3DE_INSTALL_ENGINE_NAME=o3de-sdk -DLY_STRIP_DEBUG_SYMBOLS=TRUE"
export EXTRA_CMAKE_OPTIONS="-DLY_INSTALLER_AUTO_GEN_TAG=TRUE -DLY_INSTALLER_DOWNLOAD_URL=${INSTALLER_DOWNLOAD_URL} -DLY_INSTALLER_LICENSE_URL=${INSTALLER_DOWNLOAD_URL}/license -DO3DE_INCLUDE_INSTALL_IN_PACKAGE=TRUE"
export CPACK_OPTIONS="-D CPACK_UPLOAD_URL=${CPACK_UPLOAD_URL}"
export CMAKE_TARGET=all

cd $O3DE_ROOT && \
    $O3DE_ROOT/scripts/build/Platform/Linux/build_installer_linux.sh
if [ $? -ne 0 ]
then
    echo "Error building installer"
    exit 1
fi


su $O3DE_USER -c "sudo dpkg -i $O3DE_BUILD/CPackUploads/o3de_4.2.0.deb"
if [ $? -ne 0 ]
then
    echo "Error installing O3DE for the o3de user"
    exit 1
fi

rm -rf $O3DE_ROOT
rm -rf $O3DE_BUILD
rm -rf $HOME/.o3de
rm -rf $HOME/O3DE

rm -rf /home/o3de/.o3de
rm -rf /home/o3de/O3DE

exit 0
  
###############################################################
# Clone and register o3de-extras
###############################################################
echo "Cloning o3de-extras"
git clone --single-branch -b $O3DE_EXTRAS_BRANCH $O3DE_EXTRAS_REPO $O3DE_EXTRAS_ROOT && \
    git -C $O3DE_EXTRAS_ROOT lfs install && \
    git -C $O3DE_EXTRAS_ROOT lfs pull && \
    git -C $O3DE_EXTRAS_ROOT reset --hard $O3DE_EXTRAS_COMMIT
if [ $? -ne 0 ]
then
    echo "Error cloning o3de-extras from $O3DE_EXTRAS_REPO"
    exit 1
fi

echo "Registering o3de-extras"
$WORKSPACE/o3de/scripts/o3de.sh register -agp $O3DE_EXTRAS_ROOT/Gems/ && \
    $WORKSPACE/o3de/scripts/o3de.sh register -atp $O3DE_EXTRAS_ROOT/Templates/ && \
    $WORKSPACE/o3de/scripts/o3de.sh register -app $O3DE_EXTRAS_ROOT/Projects/
if [ $? -ne 0 ]
then
    echo "Error registering o3de-extras"
    exit 1
fi

###############################################################################
# Track the git commits from all the repos
###############################################################################
echo -e "\n\
Repository                        | Commit    | Branch\n\
----------------------------------+-----------------------------------------\n\
o3de                              | $(git -C $O3DE_ROOT rev-parse --short HEAD)   | $O3DE_BRANCH\n\
o3de-extras                       | $(git -C $O3DE_EXTRAS_ROOT rev-parse --short HEAD)   | $O3DE_EXTRAS_BRANCH\n\
\n\
" >> $WORKSPACE/git_commit.txt

###############################################################
# Build O3DE installer
###############################################################
echo Generating the o3de build project
cmake -B $O3DE_ROOT/build/install \
      -S $O3DE_ROOT \
      -G "Ninja Multi-Config" \
      -DLY_DISABLE_TEST_MODULES=ON \
      -DLY_STRIP_DEBUG_SYMBOLS=ON
if [ $? -ne 0 ]
then
    echo "Error generating the o3de build project"
    exit 1
fi

echo Building the o3de installer
cmake --build $O3DE_ROOT/build/install \
      --config profile \
      --target install
if [ $? -ne 0 ]
then
    echo "Error building the O3DE installer"
    exit 1
fi

exit 0


###############################################################
# Build the assets for ROSCon2023Demo
###############################################################
pushd $ROSCON_DEMO_PROJECT/build/tools/bin/profile

# Initial run to process the assets
./AssetProcessorBatch

# Secondary run to re-process ones that missed dependencies
./AssetProcessorBatch

popd

###############################################################\
# Bundle the assets for ROSCon2023Demo
###############################################################\
mkdir -p $ROSCON_DEMO_PROJECT/build/bundles
mkdir -p $ROSCON_SIMULATION_HOME/Cache/linux

# Generate the asset lists file for the game
pushd $ROSCON_DEMO_PROJECT/build/tools/bin/profile

echo "Creating the game assetList ..."
./AssetBundlerBatch assetLists \
         --assetListFile $ROSCON_DEMO_PROJECT/build/bundles/game_linux.assetList \
         --platform linux \
         --project-path $ROSCON_DEMO_PROJECT \
         --addSeed levels/$DEMO_LEVEL/$DEMO_LEVEL.spawnable \
         --allowOverwrites
if [ $? -ne 0 ]
then
    echo "Error generating asset list from levels/$DEMO_LEVEL/$DEMO_LEVEL.spawnable"
    exit 1
fi

echo "Creating the engine assetList ..."
./AssetBundlerBatch assetLists \
         --assetListFile $ROSCON_DEMO_PROJECT/build/bundles/engine_linux.assetList \
         --platform linux \
         --project-path $ROSCON_DEMO_PROJECT \
         --addDefaultSeedListFiles \
         --allowOverwrites
if [ $? -ne 0 ]
then
    echo "Error generating default engine asset list"
    exit 1
fi

echo "Creating the game asset bundle (pak) ..."
./AssetBundlerBatch bundles \
        --maxSize 2048 \
        --platform linux \
        --project-path $ROSCON_DEMO_PROJECT \
        --allowOverwrites \
        --assetListFile $ROSCON_DEMO_PROJECT/build/bundles/game_linux.assetList \
        --outputBundlePath $ROSCON_SIMULATION_HOME/Cache/linux/game_linux.pak
if [ $? -ne 0 ]
then
    echo "Error bundling generating game pak"
    exit 1
fi
             
echo "Creating the engine asset bundle (pak) ..."
./AssetBundlerBatch bundles \
        --maxSize 2048 \
        --platform linux \
        --project-path $ROSCON_DEMO_PROJECT \
        --allowOverwrites \
        --assetListFile $ROSCON_DEMO_PROJECT/build/bundles/engine_linux.assetList \
        --outputBundlePath $ROSCON_SIMULATION_HOME/Cache/linux/engine_linux.pak
if [ $? -ne 0 ]
then
    echo "Error bundling generating game pak"
    exit 1
fi

# Build the game launcher monolithically
echo "Building launcher"
cmake -B $ROSCON_DEMO_PROJECT/build/game \
      -S $ROSCON_DEMO_PROJECT \
      -G "Ninja Multi-Config" \
      -DLY_DISABLE_TEST_MODULES=ON \
      -DLY_STRIP_DEBUG_SYMBOLS=ON \
      -DLY_MONOLITHIC_GAME=ON \
      -DALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES=0 \
    && cmake --build $ROSCON_DEMO_PROJECT/build/game \
            --config profile \
            --target ./ROSCon2023Demo.GameLauncher
if [ $? -ne 0 ]
then
    echo "Error bundling simulation launcher"
    exit 1
fi

# Package the binaries to the simulation folder
cp $ROSCON_DEMO_PROJECT/build/game/bin/profile/ROSCon2023Demo.GameLauncher $ROSCON_SIMULATION_HOME/ 
cp $ROSCON_DEMO_PROJECT/build/game/bin/profile/*.json $ROSCON_SIMULATION_HOME/ 
cp $ROSCON_DEMO_PROJECT/build/game/bin/profile/*.so $ROSCON_SIMULATION_HOME

# Cleanup
rm -rf $O3DE_ROOT
rm -rf $O3DE_EXTRAS_ROOT
rm -rf $ROSCON_DEMO_HUMAN_WORKER_ROOT
rm -rf $ROSCON_DEMO_UR_ROBOTS_ROOT
rm -rf $ROSCON_DEMO_OTTO_ROBOTS_ROOT
rm -rf $ROSCON_DEMO_PROJECT
rm -rf /root/.o3de

exit 0
