#include <opengl.hpp>

namespace OpenGL
{
/** Initialize OpenGL helper functions */
void init();
/** Destroy the default GL program and resources */
void fini();
/** Indicate we have started repainting the given output */
void bind_output(wf::output_t *output);
/** Indicate the output frame has been finished */
void unbind_output(wf::output_t *output);
}
