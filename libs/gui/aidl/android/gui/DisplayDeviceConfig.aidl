/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package android.gui;
import android.gui.ProjectionMatrix;
import android.gui.Orientation;

/**
  Display Device configuration

  projectionMatrix for left and right eye
  rotation for left and right eye
*/

parcelable DisplayDeviceConfig
{
    ProjectionMatrix[2] projectionMatrix;
    float[256] gamma;
    Orientation[2] rotation;
    char[512] calibrationFileStr;
}