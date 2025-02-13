#include "skydome.hpp"
#include <debug.hpp>
#include <core.hpp>
#include <img.hpp>

#include <output.hpp>
#include <workspace-manager.hpp>


#include <glm/gtc/matrix_transform.hpp>

#define SKYDOME_GRID_WIDTH 128
#define SKYDOME_GRID_HEIGHT 128

wf_cube_background_skydome::wf_cube_background_skydome(wf::output_t *output)
{
    this->output = output;
    load_program();

    auto section = wf::get_core().config->get_section("cube");
    background_image = section->get_option("skydome_texture", "");
    mirror_opt = section->get_option("skydome_mirror", "1");
    reload_texture();
}

wf_cube_background_skydome::~wf_cube_background_skydome()
{
    OpenGL::render_begin();
    GL_CALL(glDeleteProgram(program));
    OpenGL::render_end();
}

void wf_cube_background_skydome::load_program()
{
    OpenGL::render_begin();

    std::string shader_path = INSTALL_PREFIX "/share/wayfire/cube/shaders_2.0";

    program = OpenGL::create_program(
        shader_path + "/vertex.glsl", shader_path + "/frag.glsl");

    vpID    = GL_CALL(glGetUniformLocation(program, "VP"));
    modelID = GL_CALL(glGetUniformLocation(program, "model"));
    uvID  = GL_CALL(glGetAttribLocation(program, "uvPosition"));
    posID = GL_CALL(glGetAttribLocation(program, "position"));

    OpenGL::render_end();
}

void wf_cube_background_skydome::reload_texture()
{
    if (last_background_image == background_image->as_string())
        return;

    last_background_image = background_image->as_string();
    OpenGL::render_begin();

    if (tex == (uint32_t)-1)
    {
        GL_CALL(glGenTextures(1, &tex));
    }

    GL_CALL(glBindTexture(GL_TEXTURE_2D, tex));

    if (image_io::load_from_file(last_background_image, GL_TEXTURE_2D))
    {
        GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    }
    else
    {
        log_error("Failed to load skydome image from \"%s\".",
            last_background_image.c_str());
        GL_CALL(glDeleteTextures(1, &tex));
        tex = -1;
    }

    GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));

    OpenGL::render_end();
}

void wf_cube_background_skydome::fill_vertices()
{
    if (mirror_opt->as_int() == last_mirror)
        return;

    last_mirror = mirror_opt->as_int();

    float scale = 75.0;
    int gw = SKYDOME_GRID_WIDTH + 1;
    int gh = SKYDOME_GRID_HEIGHT;

    vertices.clear();
    indices.clear();
    coords.clear();

    for (int i = 1; i < gh; i++)
    {
        for (int j = 0; j < gw; j++)
        {
            float theta = ((2 * M_PI) / (gw - 1)) * j;
            float phi = (M_PI / gh) * i;

            vertices.push_back(cos(theta) * sin(phi) * scale);
            vertices.push_back(cos(phi) * scale);
            vertices.push_back(sin(theta) * sin(phi) * scale);

            if (last_mirror == 0)
            {
                coords.push_back((float) j / (gw - 1));
                coords.push_back((float) (i - 1) / (gh - 2));
            }
            else
            {
                float u = ((float) j / (gw - 1)) * 2.0;
                coords.push_back(u - ((u > 1.0) ? (2.0 * (u - 1.0)) : 0));
                coords.push_back((float) (i - 1) / (gh - 2));
            }
        }
    }

    for (int i = 1; i < gh - 1; i++)
    {
        for(int j = 0; j < gw - 1; j++)
        {
            indices.push_back((i - 1) * gw + j);
            indices.push_back((i - 1) * gw + j + gw);
            indices.push_back((i - 1) * gw + j + 1);
            indices.push_back((i - 1) * gw + j + 1);
            indices.push_back((i - 1) * gw + j + gw);
            indices.push_back((i - 1) * gw + j + gw + 1);
        }
    }
}

void wf_cube_background_skydome::render_frame(const wf_framebuffer& fb,
        wf_cube_animation_attribs& attribs)
{
    fill_vertices();
    reload_texture();

    if (tex == (uint32_t)-1)
    {
        GL_CALL(glClearColor(TEX_ERROR_FLAG_COLOR));
        GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
        return;
    }

    OpenGL::render_begin(fb);

    GL_CALL(glUseProgram(program));

    GL_CALL(glEnableVertexAttribArray(posID));
    GL_CALL(glEnableVertexAttribArray(uvID));

    auto rotation = glm::rotate(glm::mat4(1.0),
        (float) (attribs.duration.progress(attribs.offset_y) * 0.5),
        glm::vec3(1., 0., 0.));

    auto view = glm::lookAt(glm::vec3(0., 0., 0.),
        glm::vec3(0., 0., -attribs.duration.progress(attribs.offset_z)),
        glm::vec3(0., 1., 0.));

    auto vp = fb.transform * attribs.projection * view * rotation;

    GL_CALL(glUniformMatrix4fv(vpID, 1, GL_FALSE, &vp[0][0]));

    GL_CALL(glVertexAttribPointer(posID, 3, GL_FLOAT, GL_FALSE, 0, vertices.data()));
    GL_CALL(glVertexAttribPointer(uvID, 2, GL_FLOAT, GL_FALSE, 0, coords.data()));

    GetTuple(vx, vy, output->workspace->get_current_workspace());
    (void)vy;

    auto model = glm::rotate(glm::mat4(1.0),
        float(attribs.duration.progress(attribs.rotation)) - vx * attribs.side_angle,
        glm::vec3(0, 1, 0));

    GL_CALL(glUniformMatrix4fv(modelID, 1, GL_FALSE, &model[0][0]));

    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, tex));

    GL_CALL(glDrawElements(GL_TRIANGLES,
            6 * SKYDOME_GRID_WIDTH * (SKYDOME_GRID_HEIGHT - 2),
            GL_UNSIGNED_INT, indices.data()));

    GL_CALL(glDisableVertexAttribArray(posID));
    GL_CALL(glDisableVertexAttribArray(uvID));
    OpenGL::render_end();
}
