//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// TODO[Program]: Insert file names into error messages
// TODO[Program]: Process #include directives in GLSL

#include <azur/opengl.h>
#include <azur/log.h>
#include <azur/resources.h>
#include <azur/gl/gl.h>

namespace azur::gl {

//=== Loading and including source files =====================================//

/* Shader source code split in strings. This is used to add a prelude, split
   source files at #include directives to process them, and interleave #line
   directives in-between pieces to keep error reporting consistent. */
class ProgramSource
{
public:
    ProgramSource() = default;
    ~ProgramSource();

    /* Add a #line directive to place us at then given input number and line */
    void addLineDirective(int inputNumber, int line);
    /* Add raw text in the source code */
    void addText(char const *text, int size);

    /* Access to pieces */
    size_t pieceCount() { return m_pieceTexts.size(); }
    char const **pieceTexts() { return m_pieceTexts.data(); }
    int *pieceSizes() { return m_pieceSizes.data(); }

    /* Mark this string pointer as owned and free it when the ProgramSource is
       destroyed. */
    void takeOwnershipOf(char *ptr) { m_ownedStrings.push_back(ptr); }

    // TODO: ProgramSource: processText() method that handles #include

    void addInputName(string n) { m_inputNames.push_back(n); }
    size_t inputCount() { return m_inputNames.size(); }

    void dump();

private:
    /* List of pieces, as structure-of-arrays to simplify OpenGL calls */
    vector<char const *> m_pieceTexts;
    vector<int> m_pieceSizes;

    /* List of pointers to be freed when destroying the pieces */
    vector<char *> m_ownedStrings;
    /* Mapping of "source string numbers" used in #line to file names */
    vector<string> m_inputNames;
};

ProgramSource::~ProgramSource()
{
    for(char *str: m_ownedStrings)
        free(str);
}

void ProgramSource::addLineDirective(int inputNumber, int line)
{
    char *str = NULL;
    asprintf(&str, "#line %d %d\n", line, inputNumber);
    if(str) {
        takeOwnershipOf(str);
        addText(str, strlen(str));
    }
}

void ProgramSource::addText(char const *text, int size)
{
    m_pieceTexts.push_back(text);
    m_pieceSizes.push_back(size);
}

void ProgramSource::dump()
{
    assert(m_pieceTexts.size() == m_pieceSizes.size());

    fprintf(stderr, "ProgramSource: %zu pieces\n", m_pieceTexts.size());
    for(size_t i = 0; i < m_pieceTexts.size(); i++) {
        fprintf(stderr, "- \"\"\"%.*s\"\"\"\n",
            m_pieceSizes[i], m_pieceTexts[i]);
    }

    fprintf(stderr, "also has %zu input names\n", m_inputNames.size());
    for(auto const &in: m_inputNames)
        fprintf(stderr, "- %s\n", in.c_str());
}

//=== Program construction API ===============================================//

void Program::reset()
{
    m_programName = "<unnamed>";

    glDeleteProgram(m_prog);
    m_prog = 0;
    m_linked = false;

    m_sources.clear();
    m_shaders.clear();
    m_attributes.clear();
}

ProgramSource &Program::getProgramSource(GLuint type)
{
    if(!m_sources.count(type))
        m_sources.emplace(type, new ProgramSource());

    return *m_sources[type];
}

void Program::ProgramSourceDeleter::operator()(ProgramSource *ptr) const
{
    delete ptr;
}

void Program::setName(string programName)
{
    m_programName = std::move(programName);
}

bool Program::addPrelude(GLuint type)
{
    /* Shader prelude; this gives shader version, some macro definitions and
       some "selectors" which altogether provide some degree of compatibility
       between GLSL and GLSL ES. */
    char const *resourceName = NULL;

    #if AZUR_GRAPHICS_OPENGL_ES_3_0
    if(type == GL_VERTEX_SHADER)
        resourceName = "@azur:glsl/vs_prelude_gles3.glsl";
    else if(type == GL_FRAGMENT_SHADER)
        resourceName = "@azur:glsl/fs_prelude_gles3.glsl";

    #elif AZUR_GRAPHICS_OPENGL_3_3
    if(type == GL_VERTEX_SHADER)
        resourceName = "@azur:glsl/vs_prelude_gl3.glsl";
    else if(type == GL_FRAGMENT_SHADER)
        resourceName = "@azur:glsl/fs_prelude_gl3.glsl";
    #endif

    return resourceName ? addSourceFile(type, resourceName, false) : false;
}

bool Program::addSourceFile(GLuint type, std::string const &file, bool linedir)
{
    // TODO: Proper file access API
    extern char *load_file(char const *, size_t *);
    size_t size;
    char *src_rw = nullptr;
    char const *src_ro = nullptr;

    if(file.size() && file[0] == '@')
        src_ro = (char const *)azur::getResource(file.c_str(), &size);
    else
        src_rw = load_file(file.c_str(), &size);

    if(!src_ro && !src_rw) {
        azlog(ERROR, "Cannot read '%s': %s\n", file.c_str(), strerror(errno));
        return false;
    }

    ProgramSource &PS = getProgramSource(type);
    PS.addInputName(file);
    if(linedir)
        PS.addLineDirective(PS.inputCount() - 1, 1);
    PS.addText(src_ro ? src_ro : src_rw, size);
    if(src_rw)
        PS.takeOwnershipOf(src_rw);

    return true;
}

void Program::addSourceText(GLuint type, std::string_view code, string origin)
{
    ProgramSource &PS = getProgramSource(type);
    PS.addInputName(origin);
    PS.addLineDirective(PS.inputCount() - 1, 1);
    PS.addText(code.data(), code.size());
}

/* Common code for the shader compiling functions. */
static GLuint compileShader(GLenum type, ProgramSource &PS, string const &name)
{
    GLuint id = glCreateShader(type);
    if(id == 0) {
        azlog(ERROR, "glCreateShader failed\n");
        return 0;
    }

    GLint rc = GL_FALSE;
    GLsizei log_length = 0;

    azlog(INFO, "Compiling shader: %s\n", name.c_str());
    glShaderSource(id, PS.pieceCount(), PS.pieceTexts(), PS.pieceSizes());
    glCompileShader(id);

    glGetShaderiv(id, GL_COMPILE_STATUS, &rc);
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);

    bool needLogs = (rc == GL_FALSE) || (log_length > 1);
    if(needLogs) {
        azlog_begin();
        azlog(ERROR, "Shader compilation %s!\n",
            rc == GL_FALSE ? "failed" : "succeeded, but there are logs");
    }

    if(log_length > 1) {
        GLchar *log = new GLchar[log_length + 1];
        glGetShaderInfoLog(id, log_length, &log_length, log);
        if(log_length > 0) {
            azlog(ERROR, "%s", log);
            if(log[log_length - 1] != '\n')
                azlog(ERROR, "\n");
        }
        delete[] log;
    }

    if(needLogs)
        azlog_end();

    return id;
}

bool Program::compile()
{
    m_prog = glCreateProgram();
    if(m_prog == 0) {
        azlog(ERROR, "glCreateProgram failed\n");
        return false;
    }

    /* Compile all shaders even if one fails, so user gets all the logs. */
    bool success = true;
    for(auto const &[type, source]: m_sources) {
        GLuint id = compileShader(type, *source, m_programName);
        if(id != 0)
            m_shaders.push_back(id);
        else
            success = false;
    }

    if(!success) {
        for(auto id: m_shaders)
            glDeleteShader(id);
        m_shaders.clear();
    }
    return success;
}

bool Program::link()
{
    if(!m_prog) {
        azlog(ERROR, "cannot link program that wasn't created\n");
        return false;
    }
    if(!m_shaders.size()) {
        azlog(ERROR, "linking program with no source!\n");
        return false;
    }
    m_linked = false;

    /* Attach all shaders */
    for(auto id: m_shaders)
        glAttachShader(m_prog, id);

    GLint rc = GL_FALSE;
    GLsizei log_length = 0;

    azlog(INFO, "Linking program\n");
    glLinkProgram(m_prog);

    glGetProgramiv(m_prog, GL_LINK_STATUS, &rc);
    glGetProgramiv(m_prog, GL_INFO_LOG_LENGTH, &log_length);

    bool needLogs = (rc == GL_FALSE) || (log_length > 1);
    if(needLogs) {
        azlog_begin();
        azlog(ERROR, "Shader link %s!\n",
            rc == GL_FALSE ? "failed" : "succeeded, but there are logs");
    }

    if(log_length > 1) {
        GLchar *log = new GLchar[log_length + 1];
        glGetProgramInfoLog(m_prog, log_length, &log_length, log);
        if(log_length > 0) {
            azlog(ERROR, "%s", log);
            if(log[log_length - 1] != '\n')
                azlog(ERROR, "\n");
        }
        delete[] log;
    }

    if(needLogs)
        azlog_end();

    /* Detach individual shaders */
    for(auto id: m_shaders)
        glDetachShader(m_prog, id);

    if(rc == GL_FALSE) {
        glDeleteProgram(m_prog);
        m_prog = 0;
        return m_linked;
    }

    /* On success, delete individual shaders */
    for(auto id: m_shaders)
        glDeleteShader(id);

    m_shaders.clear();
    /* Clear cached vertex attributes */
    m_attributes.clear();

    m_linked = true;
    return m_linked;
}

GLuint Program::getAttributeLocation(char const *name)
{
    std::string name_s {name};

    if(!m_attributes.count(name_s))
        m_attributes[name_s] = glGetAttribLocation(m_prog, name);

    return m_attributes[name_s];
}

void Program::useProgram()
{
    if(isLinked())
        glUseProgram(m_prog);
    else
        azlog(ERROR, "program is not linked!\n");
}

void Program::setUniform(GLuint location, float f)
{
    glUniform1f(location, f);
}
void Program::setUniform(GLuint location, float f1, float f2)
{
    glUniform2f(location, f1, f2);
}
void Program::setUniform(GLuint location, float f1, float f2, float f3)
{
    glUniform3f(location, f1, f2, f3);
}
void Program::setUniform(GLuint location, glm::vec2 const &v2)
{
    glUniform2fv(location, 1, &v2[0]);
}
void Program::setUniform(GLuint location, glm::vec3 const &v3)
{
    glUniform3fv(location, 1, &v3[0]);
}
void Program::setUniform(GLuint location, glm::vec4 const &v4)
{
    glUniform4fv(location, 1, &v4[0]);
}
void Program::setUniform(GLuint location, glm::mat2 const &m2)
{
    glUniformMatrix2fv(location, 1, GL_FALSE, &m2[0][0]);
}
void Program::setUniform(GLuint location, glm::mat3 const &m3)
{
    glUniformMatrix3fv(location, 1, GL_FALSE, &m3[0][0]);
}
void Program::setUniform(GLuint location, glm::mat4 const &m4)
{
    glUniformMatrix4fv(location, 1, GL_FALSE, &m4[0][0]);
}
void Program::setUniform(GLuint location, int i)
{
    glUniform1i(location, i);
}

}
