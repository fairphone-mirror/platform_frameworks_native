/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package android.gui;

/**
  Layer Visibility Type
*/
@Backing(type="int")
enum LayerVisibilityType
{
    LAYER_VISIBILITY_NONE = 0,
    LAYER_VISIBILITY_LEFT_EYE = 1,
    LAYER_VISIBILITY_RIGHT_EYE = 2,
    LAYER_VISIBILITY_BOTH_EYES = 3
}