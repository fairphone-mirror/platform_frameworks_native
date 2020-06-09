/*
 * Copyright (C) 2020, Fairphone B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string>
#include <vector>

#include <KHR/khrplatform.h>

// Do not include the version-specific GL headers here. Instead, forward-declare
// the required type declarations:
typedef unsigned int    EGLBoolean;
typedef void           *EGLConfig;
typedef void           *EGLDisplay;
typedef khronos_int32_t EGLint;
typedef unsigned int    GLenum;
typedef khronos_float_t GLfloat;
typedef khronos_uint8_t GLubyte;


/** Workaround broken and incomplete graphics driver behavior on the FP2.
 *
 * The OpenGL ES implementation of the driver for the Fairphone FP2 is
 * incomplete, unstable, and causes various tests to fail on Android 7 and 9.
 * Falling back to OpenGL ES 2.0 instead of 3.0 is more stable in many
 * situations. Therefore, OpenGL ES 3.0 is treated as experimental feature from
 * Android 7 onwards.
 *
 * This class gathers fixes and workarounds for bugs and incomplete
 * implementations in the outdated graphics driver. Based on the
 * ro.opengles.version system property, it operates in two modes:
 *
 * - OpenGL ES 2.0: Hide meta data that hints to OpenGL ES 3.0 support in
 *   strings and feature flags.
 * - Experimental OpenGL ES 3.0: Keep exposing GLES support as reported by the
 *   driver and only try applying fixes where possible.
 */
class FP2GLESWorkarounds {
public:
    /** Call this once before using any other functions of this class. */
    static void initialize();
    /** Check whether experimental OpenGL ES 3.0 support is enabled in the system.*/
    static bool isExperimentalGLES3Enabled();

    // Global GL constants

    /** Actual values for GL_ALIASED_POINT_SIZE_RANGE
     *
     * The driver reports range (1.0, 1023.0), but it actually renders points up
     * to size 4092. This causes conformance tests to fail, because the driver
     * is supposed to clamp requested point sizes to the range it reports. Fix
     * this by reporting values that match the actual behavior of the driver.
     * Test: dEQP-GLES2.functional.rasterization.limits#points
     */
    static constexpr GLfloat GL_ALIASED_POINT_SIZE_MIN = 1.f;
    static constexpr GLfloat GL_ALIASED_POINT_SIZE_MAX = 4092.f;

    /** Hide GLES3 support from context attributes if needed.
     *
     * Remove EGL_OPENGL_ES3_BIT_KHR from EGL_RENDERABLE_TYPE unless
     * experimental GLES3 support is enabled.
     */
    static EGLBoolean eglGetConfigAttrib(
        EGLDisplay display,
        EGLConfig config,
        EGLint attribute,
        EGLint * value,
        EGLBoolean driverResult);

    /** Adjust driver-reported strings if needed.
     *
     * Following changes are made, unless experimental GLES3 support is enabled:
     *  - Adjust version strings to report OpenGL ES 2.0 only.
     */
    static const GLubyte * glGetString(GLenum name, const GLubyte * driverString);

    /** Filter extension list in internal EGL data structures.
     *
     * OpenGL (ES) extensions need to be handled by thread and context. Use
     * EGL's existing logic for that and filter their internal data structures
     * directly.
     *
     * Note: Add a call to this function in egl_object.cpp directly, so that the
     * extensions are filtered direct where EGL's internal data structures are
     * initialized.
     */
    static void filterEGLContextExtensions(
        std::string * gl_extensions,
        std::vector<std::string> * tokenized_gl_extensions);
};
