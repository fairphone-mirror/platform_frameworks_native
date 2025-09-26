/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package android.gui;

/**
  represents Frustum.

  angleLeft is the angle of the left side of the field of view.
  angleRight is the angle of the right side of the field of view.
  angleUp is the angle of the top part of the field of view.
  angleDown is the angle of the bottom part of the field of view.

  angles must be specified in radians
*/
parcelable Frustum
{
    float angleLeft;
    float angleRight;
    float angleUp;
    float angleDown;
}