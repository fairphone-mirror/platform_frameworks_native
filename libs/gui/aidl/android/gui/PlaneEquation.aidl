/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package android.gui;

/**
  plane equation
  ax+by+cz+d = 0 where a, b, c and d are components of the normal vector.
*/
parcelable PlaneEquation {
    float a;
    float b;
    float c;
    float d;
}