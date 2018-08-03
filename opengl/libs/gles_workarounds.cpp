#define LOG_TAG "GLES_FP2"

#include "gles_workarounds.h"

#include <cstring>
#include <regex>
#include <string>
#include <vector>

#include <cutils/properties.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES3/gl3.h>

#include "egl_impl.h"

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

void early_gles_workarounds_init()
{
    fp2_experimental_gles3 = check_fp2_experimental_gles3();
}

pthread_once_t once_control = PTHREAD_ONCE_INIT;
const int sEarlyInitState = pthread_once(&once_control, &early_gles_workarounds_init);


const std::vector<std::string> & unstableGLExtension()
{
    static const std::vector<std::string> exts = {
        "GL_EXT_color_buffer_float",
        "GL_EXT_color_buffer_half_float",
        "GL_EXT_sRGB",
        "GL_EXT_sRGB_write_control",
        "GL_EXT_texture_sRGB_decode",
        "GL_OES_texture_half_float",
        "GL_OES_texture_half_float_linear",
    };
    return exts;
}

const std::regex & unstableGLExtRegex()
{
    static const std::regex r = []() {
        // Construct regex: match all unstable extension names.
        std::string regexStr;
        for (size_t i = 0; i < unstableGLExtension().size(); ++i) {
            regexStr.append(unstableGLExtension()[i]);
            if (i != unstableGLExtension().size() - 1) {
                regexStr.append("|");
            }
        }
        // Match whole words only and remove redundant space.
        regexStr = "\\b(" + regexStr + ")\\b[ ]?";
        return std::regex(regexStr);
    }();
    return r;
}

std::string filterStableExtensions(const std::string & extensions)
{
    return std::regex_replace(
        extensions,
        unstableGLExtRegex(),
        "");
}

const std::vector<std::string> & unstableEGLExtension()
{
    static const std::vector<std::string> exts = {
        "EGL_KHR_gl_colorspace",
    };
    return exts;
}

const std::regex & unstableEGLExtRegex()
{
    static const std::regex r = []() {
        // Construct regex: match all unstable extension names.
        std::string regexStr;
        for (size_t i = 0; i < unstableEGLExtension().size(); ++i) {
            regexStr.append(unstableEGLExtension()[i]);
            if (i != unstableEGLExtension().size() - 1) {
                regexStr.append("|");
            }
        }
        // Match whole words only and remove redundant space.
        regexStr = "\\b(" + regexStr + ")\\b[ ]?";
        return std::regex(regexStr);
    }();
    return r;
}

std::string filterStableEGLExtensions(const char * extensions)
{
    return std::regex_replace(
        extensions,
        unstableEGLExtRegex(),
        "");
}

// Check for incompletely supported formats for textures.
bool isValidTextureFormat(GLenum format, GLenum internalformat) {
    if (fp2_experimental_gles3) {
        // Experimental mode: Expose everything the driver implements.
        return true;
    }

    // The "GL_SRGB*_EXT" enums below are introduced by the extension
    // GL_EXT_sRGB. They are available as identical, non-*_EXT enum in GLES3 as
    // well. GL_SRGB8 is available through GLES3 only.

    if (format == GL_SRGB_EXT || format == GL_SRGB_ALPHA_EXT) {
        return false;
    }

    if (internalformat == GL_SRGB8 || internalformat == GL_SRGB8_ALPHA8_EXT
        || internalformat == GL_SRGB_EXT || internalformat == GL_SRGB_ALPHA_EXT) {
        return false;
    }

    // Following formats are only available in OpenGL ES 3.0 and must fail in
    // earlier contexts.
    if (internalformat == GL_R16F || internalformat == GL_RG16F
        || internalformat == GL_RGBA16F || internalformat == GL_R11F_G11F_B10F) {
        return false;
    }

    return true;
}

// Check for incompletely supported formats for renderbuffers.
bool isValidRenderbufferFormat(GLenum internalformat) {
    if (fp2_experimental_gles3) {
        return true;
    }

    // GL_EXT_sRGB adds GL_SRGB8_ALPHA8_EXT for renderbuffers. All other
    // GL_SRGB* enums get rejected by the driver, if used on renderbuffers.
    if (internalformat == GL_SRGB8_ALPHA8_EXT) {
        return false;
    }

    if (internalformat == GL_RGB10_A2 || internalformat == GL_R11F_G11F_B10F) {
        return false;
    }

    // Hide all float renderbuffer support, because it doesn't work reliably
    if (internalformat == GL_R16F || internalformat == GL_RG16F
        || internalformat == GL_RGBA16F) {
        return false;
    }

    return true;
}

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
    if (!value) {
        return driverResult;
    }

    if ((attribute == EGL_RENDERABLE_TYPE) && !fp2_experimental_gles3) {
        *value = *value & ~EGL_OPENGL_ES3_BIT_KHR;
    }

    return driverResult;
}


bool FP2GLESWorkarounds::validateGlGetParameter(GLenum name)
{
    if (fp2_experimental_gles3) {
        return true;
    }

    switch (name) {
        case GL_MAJOR_VERSION:
            [[fallthrough]];
        case GL_MINOR_VERSION:
            // These parameter does not exist prior to Open GL ES 3.0.
            android::egl_set_framework_error_for_currrent_context(GL_INVALID_ENUM);
            return false;
        default:
            return true;
    }
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

const char* FP2GLESWorkarounds::eglQueryString(GLenum name, const char * driverString)
{
    if (name != EGL_EXTENSIONS) {
        return driverString;
    }

    if (!driverString) {
        return driverString;
    }

    static const char * previousDriverString = nullptr;
    static std::string stableExtensions;
    if (previousDriverString != driverString) {
        stableExtensions = filterStableEGLExtensions(driverString);
        previousDriverString = driverString;
    }
    return stableExtensions.c_str();
}


bool FP2GLESWorkarounds::checkValidGlTexImage2D(GLenum /*target*/,
    GLint /*level*/,
    GLint internalformat,
    GLsizei /*width*/,
    GLsizei /*height*/,
    GLint /*border*/,
    GLenum format,
    GLenum /*type*/,
    const void * /*data*/)
{
    if (!isValidTextureFormat(format, internalformat)) {
        // glTexImage2D and related report GL_INVALID_OPERATION if format,
        // internal format and type don't match the expected combinations.
        android::egl_set_framework_error_for_currrent_context(GL_INVALID_OPERATION);
        return false;
    }

    return true;
}

bool FP2GLESWorkarounds::checkValidGlTexSubImage2D(GLenum /*target*/,
    GLint /*level*/,
    GLint /*xoffset*/,
    GLint /*yoffset*/,
    GLsizei /*width*/,
    GLsizei /*height*/,
    GLenum format,
    GLenum /*type*/,
    const void * /*data*/)
{
    if (!isValidTextureFormat(format, 0)) {
        android::egl_set_framework_error_for_currrent_context(GL_INVALID_OPERATION);
        return false;
    }

    return true;
}

bool FP2GLESWorkarounds::checkValidGlTexImage3D(GLenum /*target*/, GLint /*level*/,
    GLint internalformat, GLsizei /*width*/, GLsizei /*height*/, GLsizei /*depth*/,
    GLint /*border*/, GLenum format, GLenum /*type*/, const void */*pixels*/) {
    if (!isValidTextureFormat(format, internalformat)) {
        android::egl_set_framework_error_for_currrent_context(GL_INVALID_OPERATION);
        return false;
    }
    return true;
}

bool FP2GLESWorkarounds::checkValidGlTexSubImage3D(GLenum /*target*/, GLint /*level*/,
    GLint /*xoffset*/, GLint /*yoffset*/, GLint /*zoffset*/, GLsizei /*width*/, GLsizei /*height*/,
    GLsizei /*depth*/, GLenum format, GLenum /*type*/,
    const void */*pixels*/) {
    if (!isValidTextureFormat(format, 0)) {
        android::egl_set_framework_error_for_currrent_context(GL_INVALID_OPERATION);
        return false;
    }
    return true;
}

bool FP2GLESWorkarounds::checkValidGlRenderbufferStorage(GLenum /*target*/,
    GLenum internalformat,
    GLsizei /*width*/,
    GLsizei /*height*/)
{
    if (!isValidRenderbufferFormat(internalformat)) {
        // glRenderbufferStorage reports GL_INVALID_ENUM in case of wrong
        // internal format.
        android::egl_set_framework_error_for_currrent_context(GL_INVALID_ENUM);
        return false;
    }

    return true;
}

void FP2GLESWorkarounds::filterEGLContextExtensions(
    std::string * gl_extensions,
    std::vector<std::string> * tokenized_gl_extensions)
{
    if (fp2_experimental_gles3) {
        return;
    }

    // EGL stores extensions both in a string (separated by space) and a vector.
    // Filter unstable extensions from both lists.
    for (auto && ext : unstableGLExtension())
    {
        const auto it = std::find(
            tokenized_gl_extensions->begin(),
            tokenized_gl_extensions->end(),
            ext
        );
        if (it != tokenized_gl_extensions->end()) {
            tokenized_gl_extensions->erase(it);
        }
    }
    *gl_extensions = filterStableExtensions(*gl_extensions);
}
