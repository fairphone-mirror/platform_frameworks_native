#include "gles_workarounds.h"

#include <cstring>

#include <cutils/properties.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

namespace
{

bool fp2_experimental_gles3 = false;

bool check_fp2_experimental_gles3()
{
    static const char s_OpenGLESv30[] = "196608";
    char prop[PROPERTY_VALUE_MAX];
    property_get("ro.opengles.version", prop, "");
    return strncmp(prop, s_OpenGLESv30, PROPERTY_VALUE_MAX) == 0;
}

}


void FP2GLESWorkarounds::initialize()
{
    fp2_experimental_gles3 = check_fp2_experimental_gles3();
}

bool FP2GLESWorkarounds::isExperimentalGLES3Enabled()
{
    return fp2_experimental_gles3;
}

EGLBoolean FP2GLESWorkarounds::eglGetConfigAttrib(
    const EGLDisplay /* display */,
    const EGLConfig /* config */,
    const EGLint attribute,
    EGLint * value,
    const EGLBoolean driverResult)
{
    if ((attribute == EGL_RENDERABLE_TYPE) && !fp2_experimental_gles3) {
        *value = *value & ~EGL_OPENGL_ES3_BIT_KHR;
    }

    return driverResult;
}

const GLubyte * FP2GLESWorkarounds::glGetString(const GLenum name, const GLubyte * driverString)
{
    if (fp2_experimental_gles3) {
        return driverString;
    }

    if (name == GL_VERSION) {
        struct VersionStringReplacement {
            const char * broken_version_prefix;
            const size_t check_length;
            const char * fake_version_string;
        };
        const VersionStringReplacement versionStringReplacements[] = {
            { "OpenGL ES 3.", 12, "OpenGL ES 2.0" },
            { "OpenGL ES-CM 3.", 15, "OpenGL ES-CM 2.0" },
            { NULL, 0, NULL }
        };
        const VersionStringReplacement * r = versionStringReplacements;
        for (; r->broken_version_prefix != NULL; ++r) {
            if (strncmp((const char *)driverString,
                r->broken_version_prefix, r->check_length) == 0) {
                return (const GLubyte *)r->fake_version_string;
            }
        }
        return driverString;
    }

    return driverString;
}
