/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package android.gui;

/**
  App layer composition type
*/
@Backing(type="int")
enum CompositionLayerType
{
    COMPOSITION_LAYER_NONE = 0,
    COMPOSITION_LAYER_QUAD = 1,
    COMPOSITION_LAYER_PROJECTION = 2
}