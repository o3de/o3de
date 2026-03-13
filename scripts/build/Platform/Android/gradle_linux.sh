#!/usr/bin/env bash
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set -o errexit # exit on the first failure encountered

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
O3DE_PATH="$(cd "$SCRIPT_DIR/../../../.." && pwd)"

echo "Using O3DE Engine at '$O3DE_PATH'"

# Validate the Android SDK is set
if [[ -z "$ANDROID_SDK_ROOT" ]] && [[ -n "$ANDROID_HOME" ]]; then
    export ANDROID_SDK_ROOT="$ANDROID_HOME"
fi
if [[ -z "$ANDROID_SDK_ROOT" ]]; then
    echo "ANDROID_SDK_ROOT is not set"
    exit 1
fi
if [[ ! -d "$ANDROID_SDK_ROOT" ]]; then
    echo "ANDROID_SDK_ROOT '$ANDROID_SDK_ROOT' does not exist"
    exit 1
fi
echo "Using Android SDK at '$ANDROID_SDK_ROOT'"
"$O3DE_PATH/scripts/o3de.sh" android-configure --set-value sdk.root="$ANDROID_SDK_ROOT" --global

# Configure gradle
if [[ -n "$USE_GRADLE_FROM_PATH" ]]; then
    GRADLE_LOCATION=$(which gradle 2>/dev/null || true)
    if [[ -z "$GRADLE_LOCATION" ]]; then
        echo "USE_GRADLE_FROM_PATH is set but gradle was not found on PATH"
        exit 1
    fi
    GRADLE_HOME="$(dirname "$(dirname "$GRADLE_LOCATION")")"
    GRADLE_BUILD_HOME="$GRADLE_HOME"
else
    if [[ -z "$GRADLE_BUILD_HOME" ]]; then
        echo "GRADLE_BUILD_HOME is not set"
        exit 1
    fi
    if [[ ! -d "$GRADLE_BUILD_HOME" ]]; then
        echo "GRADLE_BUILD_HOME '$GRADLE_BUILD_HOME' does not exist"
        exit 1
    fi
fi

echo "Using Gradle Build at '$GRADLE_BUILD_HOME'"
"$O3DE_PATH/scripts/o3de.sh" android-configure --set-value gradle.home="$GRADLE_BUILD_HOME" --global

# Optionally override the gradle plugin version, otherwise default to 8.1.4
ANDROID_GRADLE_PLUGIN="${ANDROID_GRADLE_PLUGIN:-8.1.4}"
echo "Using Android gradle plugin version $ANDROID_GRADLE_PLUGIN"
"$O3DE_PATH/scripts/o3de.sh" android-configure --set-value android.gradle.plugin="$ANDROID_GRADLE_PLUGIN" --global

# Optionally override the version of the android NDK to use, otherwise default to 25
ANDROID_NDK_VERSION="${ANDROID_NDK_VERSION:-25}"
echo "Using Android NDK Version $ANDROID_NDK_VERSION"
"$O3DE_PATH/scripts/o3de.sh" android-configure --set-value ndk.version="$ANDROID_NDK_VERSION.*"

# Create/clean output directory
if [[ -d "$OUTPUT_DIRECTORY" ]]; then
    echo "Clearing and resetting existing build folder"
    rm -rf "$OUTPUT_DIRECTORY"
fi
mkdir -p "$OUTPUT_DIRECTORY"

# Set temp directory
if [[ -z "$TMP" ]]; then
    TMP="${WORKSPACE:-/tmp}/o3de_tmp"
    export TMP
    export TEMP="$TMP"
    mkdir -p "$TMP"
fi

# Create a minimal project for the native build process
GRADLE_PROJECT_DIR="$TMP/o3de_gradle_ar"
rm -rf "$GRADLE_PROJECT_DIR"
echo "Creating a minimal project for the native build process"
echo "$O3DE_PATH/scripts/o3de.sh create-project -pp $GRADLE_PROJECT_DIR -pn GradleTest -tn MinimalProject"
"$O3DE_PATH/scripts/o3de.sh" create-project -pp "$GRADLE_PROJECT_DIR" -pn GradleTest -tn MinimalProject

# Optionally sign the APK if we are generating an APK
GENERATE_SIGNED_APK=false
if [[ "$SIGN_APK" == "true" ]] && [[ "$GRADLE_BUILD_CMD" == "assemble" ]]; then
    GENERATE_SIGNED_APK=true
fi

if [[ "$GENERATE_SIGNED_APK" == "true" ]]; then
    # Setup signing with JDK keytool
    JAVA_BIN="$JAVA_HOME/bin"
    KEYTOOL_PATH="$JAVA_BIN/keytool"

    if [[ ! -x "$KEYTOOL_PATH" ]]; then
        echo "keytool not found at '$KEYTOOL_PATH'. Ensure JAVA_HOME is set to a valid JDK installation."
        exit 1
    fi

    CI_ANDROID_KEYSTORE_FILE="ly-android-dev.keystore"
    CI_ANDROID_KEYSTORE_ALIAS="ly-android"
    CI_ANDROID_KEYSTORE_PASSWORD="lumberyard"
    CI_KEYSTORE_VALIDITY_DAYS=10000
    CI_KEYSTORE_CERT_DN="cn=LY Developer, ou=Lumberyard, o=Amazon, c=US"
    CI_ANDROID_KEYSTORE_FILE_ABS="$(pwd)/$OUTPUT_DIRECTORY/$CI_ANDROID_KEYSTORE_FILE"

    # Generate the keystore file if needed
    if [[ ! -f "$CI_ANDROID_KEYSTORE_FILE_ABS" ]]; then
        echo "[ci_build] Generating keystore file $CI_ANDROID_KEYSTORE_FILE"
        "$KEYTOOL_PATH" -genkeypair -v \
            -keystore "$CI_ANDROID_KEYSTORE_FILE_ABS" \
            -storepass "$CI_ANDROID_KEYSTORE_PASSWORD" \
            -alias "$CI_ANDROID_KEYSTORE_ALIAS" \
            -keypass "$CI_ANDROID_KEYSTORE_PASSWORD" \
            -keyalg RSA -keysize 2048 \
            -validity "$CI_KEYSTORE_VALIDITY_DAYS" \
            -dname "$CI_KEYSTORE_CERT_DN"
    else
        echo "Using keystore file at $CI_ANDROID_KEYSTORE_FILE_ABS"
    fi

    "$O3DE_PATH/scripts/o3de.sh" android-configure --set-value signconfig.store.file="$CI_ANDROID_KEYSTORE_FILE_ABS"
    "$O3DE_PATH/scripts/o3de.sh" android-configure --set-value signconfig.key.alias="$CI_ANDROID_KEYSTORE_ALIAS"
    "$O3DE_PATH/scripts/o3de.sh" android-configure --set-value signconfig.key.password="$CI_ANDROID_KEYSTORE_PASSWORD"
    "$O3DE_PATH/scripts/o3de.sh" android-configure --set-value signconfig.store.password="$CI_ANDROID_KEYSTORE_PASSWORD"
fi

"$O3DE_PATH/scripts/o3de.sh" android-configure --set-value asset.mode=NONE

cd "$GRADLE_PROJECT_DIR"

echo "$O3DE_PATH/scripts/o3de.sh android-generate -p $GRADLE_PROJECT_DIR -B $OUTPUT_DIRECTORY $ADDITIONAL_GENERATE_ARGS"
"$O3DE_PATH/scripts/o3de.sh" android-generate -p "$GRADLE_PROJECT_DIR" -B "$OUTPUT_DIRECTORY" $ADDITIONAL_GENERATE_ARGS

export CMAKE_BUILD_PARALLEL_LEVEL=$(nproc)

# Run the gradle build from the output directory
pushd "$OUTPUT_DIRECTORY"

# Stop any running or orphaned gradle daemon
echo "[ci_build] gradlew --stop"
./gradlew --stop || true

echo "[ci_build] gradlew --info --no-daemon ${GRADLE_BUILD_CMD}${CONFIGURATION}"
./gradlew --info --no-daemon "${GRADLE_BUILD_CMD}${CONFIGURATION}"

echo "[ci_build] gradlew --stop"
./gradlew --stop

popd

exit 0
