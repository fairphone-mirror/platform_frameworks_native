/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package android.gui;

/**
 * App layer lock type
 */
@Backing(type="int")
enum RenderLayerReferenceSpaceType {
    RENDER_LAYER_REFERENCE_SPACE_NONE = 0,
    RENDER_LAYER_REFERENCE_SPACE_WORLD = 1,
    RENDER_LAYER_REFERENCE_SPACE_HEAD = 2,
    RENDER_LAYER_REFERENCE_SPACE_SPHERE = 3
}