#define LOG_TAG "GLES_FP2"

#include "gles_workarounds.h"

#include <cstring>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include <cutils/properties.h>
#include <log/log.h>

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


// The following two functions are taken from system/core/base/strings.cpp for
// convenience. See ./Android.bp for comments on not adding additional link
// dependencies.

std::vector<std::string> SplitString(const std::string& s,
                               const std::string& delimiters) {
  if (delimiters.size() == 0u) {
    return {};
  }

  std::vector<std::string> result;

  size_t base = 0;
  size_t found;
  while (true) {
    found = s.find_first_of(delimiters, base);
    result.push_back(s.substr(base, found - base));
    if (found == s.npos) break;
    base = found + 1;
  }

  return result;
}

template <typename ContainerT, typename SeparatorT>
std::string JoinStrings(const ContainerT& things, SeparatorT separator) {
  if (things.empty()) {
    return "";
  }

  std::ostringstream result;
  result << *things.begin();
  for (auto it = std::next(things.begin()); it != things.end(); ++it) {
    result << separator << *it;
  }
  return result.str();
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

namespace {

enum class FixResult {
    noMatch,
    applied,
    unsupportedCode,
};

/** Replace a function call on a vec3 by separate calls on each component.
 *
 * This helps fixing issues with dFdx/dFdy in GLSL on Adreno 330 that produces
 * wrong results only when called on a vec3.
 *
 * Parameters:
 *  pos: Starting position to search from; will be updated to the end of the
 *    current search.
 */
FixResult replace_func_vec3_call(std::string& source, const std::string& funcName, size_t& pos) {
    const size_t callStartPos = source.find(funcName, pos);
    if (callStartPos == std::string::npos) {
        return FixResult::noMatch;
    }
    // Find opening and closing brackets
    pos = callStartPos + funcName.length();
    for (; pos < source.length(); ++pos) {
        switch (source[pos]) {
            case ' ':
                continue;
            case '\t':
                continue;
            case '\n':
                continue;
            case '\r':
                continue;
            case '(':
                break;
            default:
                return FixResult::unsupportedCode;
        }
        break;
    }
    if (pos == source.length()) {
        pos = std::string::npos;
        return FixResult::unsupportedCode;
    }
    const size_t openingPos = pos;
    const size_t closingPos = source.find(")", openingPos);
    if (closingPos == std::string::npos) {
        return FixResult::unsupportedCode;
    }

    // Extract the parameter part of call like `func(var.xyz)`:
    const std::string paramStr = source.substr(
        openingPos + 1, closingPos - openingPos - 1);
    // Match `some_variable_42.swizzle`, where `swizzle` can use either of the
    // sets `xyzw`, `rgba` or `stpq` (according to GLSL specs). We are looking
    // for cases where the result is a vec3, thus typically `.xyz`.
    static const std::regex paramRegex(
        R"(\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\.\s*([xyzwrgbastpq]{3,3})\s*)");
    std::smatch match;
    if (!std::regex_match(paramStr, match, paramRegex) || (match.size() != 3)) {
        return FixResult::unsupportedCode;
    }
    const std::string& var = match[1];
    const std::string& swizzle = match[2];
    const std::string fixedCall = "vec3("
        + funcName + "(" + var + "." + swizzle[0] + "), "
        + funcName + "(" + var + "." + swizzle[1] + "), "
        + funcName + "(" + var + "." + swizzle[2] + "))";
    source.replace(callStartPos, closingPos - callStartPos + 1, fixedCall);
    pos = callStartPos + fixedCall.length();
    return FixResult::applied;
}

bool apply_GLSL_vec3_workaround(std::string& shaderString)
{
    // This is an extremely simplistic implementation. To do this properly, we
    // would need a complete syntax analysis.
    int appliedCount = 0;
    int unsupportedCount = 0;
    auto lines = SplitString(shaderString, "\n\r");
    for (auto & line : lines) {
        if (line.length() > 0 && line[0] == '#') {
            continue;
        }
        const auto commentStart = line.find("//");
        if (commentStart != std::string::npos) {
            line.resize(commentStart);
        }
        auto loopFix = [&appliedCount, &unsupportedCount] (
            std::string& source, const std::string& funcName)
            {
                size_t pos = 0;
                while (pos != source.length()) {
                    switch (replace_func_vec3_call(source, funcName, pos)) {
                        case FixResult::unsupportedCode:
                            ++unsupportedCount;
                            return;
                        case FixResult::noMatch:
                            return;
                        case FixResult::applied:
                            ++appliedCount;
                            break;
                    }
                }
            };
        loopFix(line, "dFdx");
        loopFix(line, "dFdy");
    }
    if (appliedCount) {
        shaderString = JoinStrings(lines, "\n");
    }
    if (appliedCount != 0 || unsupportedCount != 0) {
        ALOGI("glShaderSource: Found calls to dFdx/dFdy with vec3 parameter "
            "(broken on current Adreno 330 driver):");
        ALOGI("    Fixed cases: %i", appliedCount);
        ALOGI("    Unsupported code/syntax: %i", unsupportedCount);
    }
    return appliedCount != 0;
}

/** Workaround broken structs with highp in fragment shaders.
 *
 * Adreno 330 and other driver seem to have issues with highp struct members in
 * fragment shaders. For what we know, it affects vec3 members of structs only.
 */
bool apply_GLSL_frag_highp_workaround(std::string& shaderString)
{
    // First, drop simple comments.
    static const auto commentRegex = std::regex(R"(//.*)");
    // Again, actual syntax parsing would be required, but we'll keep it simpler
    // for now. Match only simple structs, ignore cases with nesting or macros.
    static const auto simpleStructRegex = std::regex(
        R"(struct\s+[a-zA-Z_][a-zA-Z0-9_]*\s*\{[\sa-zA-Z0-9;\[\]]+\}\s*;)");
    // What what we know, only highp vec3's are affected.
    static const auto highpVec3Regex = std::regex(R"(highp\s+vec3)");
    static const char* mediumpVec3 = "mediump vec3";

    int appliedCount = 0;

    // Construct a new shader; all current contents remaining to be processed.
    auto remainder = std::regex_replace(shaderString, commentRegex, "");
    std::string newShader;
    std::smatch structMatch;
    while (true) {
        if (!std::regex_search(remainder, structMatch, simpleStructRegex)) {
            // Nothing else matches, so just take over the remainder as-is.
            newShader.append(std::move(remainder));
            break;
        }

        // Take over the non-matching prefix.
        newShader.append(structMatch.prefix());

        // Check for and replace highp within the match
        auto adjustedStruct = std::regex_replace(
            structMatch.str(), highpVec3Regex, mediumpVec3);
        newShader.append(adjustedStruct);
        if (adjustedStruct != structMatch.str()) {
            ++appliedCount;
        }

        // Continue processing the rest.
        remainder = structMatch.suffix();
    }

    if (appliedCount == 0) {
        return false;
    }

    shaderString = newShader;
    ALOGI("glShaderSource: Applied workaround for highp vec3 in structs "
        "(broken on current Adreno 330 driver):");
    ALOGI("    Fixed cases: %i", appliedCount);
    return true;
}

}

bool FP2GLESWorkarounds::adjustShaderString(std::string& shaderString) {
    bool vec3_applied = apply_GLSL_vec3_workaround(shaderString);
    bool highp_applied = apply_GLSL_frag_highp_workaround(shaderString);
    return vec3_applied || highp_applied;
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
