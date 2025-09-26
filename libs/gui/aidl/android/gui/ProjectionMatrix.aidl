/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package android.gui;

/**
  represents Display Projection Matrix
  A square matrix that maps vectors into their projections onto a subspace.
*/
parcelable ProjectionMatrix
{
    float[4][4] prjMatrix;
}