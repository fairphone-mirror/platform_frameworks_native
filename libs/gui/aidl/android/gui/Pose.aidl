/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package android.gui;
import android.gui.Position;
import android.gui.Orientation;

/**
  represents Pose.

  pos represents position within a space.
  orienation represents orientation within a space.
*/
parcelable Pose
{
    Position pos;
    Orientation orientation;
}
