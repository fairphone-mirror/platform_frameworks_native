/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package android.gui;

/**
  represents orientation.
  Rotation is represented by a unit orientation.

  x is the x coordinate of the orientation.
  y is the y coordinate of the orientation.
  z is the z coordinate of the orientation.
  w is the w coordinate of the orientation.
*/
parcelable Orientation {
    float x;
    float y;
    float z;
    float w;
}