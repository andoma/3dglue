#include "scene.hpp"

#include "opengl.hpp"
#include "camera.hpp"
#include "object.hpp"
#include "image.hpp"

#include <sys/stat.h>

#include <glm/gtx/string_cast.hpp>

#include <optional>

namespace g3d {

struct Scene;

struct Grab {
    bool m_on{false};
    Hit m_hit;

    void press(Scene &s, Control ctrl, const glm::vec2 &xy);

    void release(Scene &s, Control ctrl, const glm::vec2 &xy);
};

struct GLFWImguiScene : public Scene {
    GLFWImguiScene(const char *title, int width, int height, float ground_size);

    ~GLFWImguiScene();

    bool prepare() override;

    void draw() override;

    void save_frame(void);

    void mark();

    glm::vec2 normalizedCursor();

    void drag(Control c, const glm::vec2 &delta, Grab &g, const glm::vec2 &xy);

    GLFWwindow *m_window;
    unsigned int m_width;
    unsigned int m_height;

    GLuint m_vao;

    std::shared_ptr<Object> m_crosshair;
    std::shared_ptr<Object> m_skybox;
    std::shared_ptr<Object> m_ground;

    glm::vec2 m_cursor_prev{0};

    Grab m_left_grab;

    Grab m_right_grab;

    bool m_shift_down{false};

    bool m_alt_down{false};

    bool m_ctrl_down{false};

    bool m_scene_editor_visible{true};

    float m_scene_editor_start{0.75};

    std::unique_ptr<Image2D> m_copy;
    int m_frame_cnt{0};

    bool m_record{false};

    bool m_measure_phase{false};

    std::optional<glm::vec3> m_p1;
    std::shared_ptr<Object> m_p1_crosshair;

    std::optional<glm::vec3> m_p2;
    std::shared_ptr<Object> m_p2_crosshair;
};

void
Grab::press(Scene &s, Control ctrl, const glm::vec2 &xy)
{
    if(s.m_hit.object &&
       s.m_hit.object->uiInput(s.m_hit, ctrl, {}, *s.m_camera, xy)) {
        m_hit = s.m_hit;
        printf("hit\n");
    } else {
        m_hit.object = nullptr;
    }
    m_on = true;
}

void
Grab::release(Scene &s, Control ctrl, const glm::vec2 &xy)
{
    if(m_hit.object)
        m_hit.object->uiInput(m_hit, ctrl, {}, *s.m_camera, xy);
    m_on = false;
}

void
GLFWImguiScene::save_frame(void)
{
    if(m_copy == NULL || m_width != m_copy->m_width ||
       m_height != m_copy->m_height) {
        m_copy = std::make_unique<Image2D>(m_width, m_height);
    }
    glReadPixels(0, 0, m_copy->m_width, m_copy->m_height, GL_RGB,
                 GL_UNSIGNED_BYTE, m_copy->m_data);

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "capture/frame-%04d.jpeg", m_frame_cnt);
    mkdir("capture", 0755);
    m_copy->save_jpeg(path, 95, true);
    m_frame_cnt++;
}

static void
glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    exit(1);
}

static void
resize_callback(GLFWwindow *window, int width, int height)
{
    GLFWImguiScene *s = (GLFWImguiScene *)glfwGetWindowUserPointer(window);
    s->m_width = width;
    s->m_height = height;
}

static void
MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    ImGuiIO &io = ImGui::GetIO();
    if(!io.WantCaptureMouse) {
        GLFWImguiScene *s = (GLFWImguiScene *)glfwGetWindowUserPointer(window);

        if(s->m_camera == NULL)
            return;

        auto xy = s->normalizedCursor();

        if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            if(s->m_alt_down) {
                s->mark();
            }
            s->m_left_grab.press(*s, Control::ROTATE_START, xy);
        }
        if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            s->m_left_grab.release(*s, Control::ROTATE_END, xy);
        }
        if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            s->m_right_grab.press(*s, Control::TRANSLATE_START, xy);
        }
        if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
            s->m_right_grab.release(*s, Control::TRANSLATE_END, xy);
        }
        return;
    }
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

static void
ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    ImGuiIO &io = ImGui::GetIO();
    if(!io.WantCaptureMouse) {
        GLFWImguiScene *s = (GLFWImguiScene *)glfwGetWindowUserPointer(window);

        if(s->m_camera == NULL)
            return;
        float scale = s->m_shift_down ? 0.1f : 1.0f;
        s->m_camera->uiInput(Control::ZOOM,
                             glm::vec2{-xoffset, yoffset} * scale);
        return;
    }
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

static void
KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    GLFWImguiScene *s = (GLFWImguiScene *)glfwGetWindowUserPointer(window);

    if(key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
        s->m_shift_down = action == GLFW_PRESS;
    }

    if(key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT) {
        s->m_alt_down = action == GLFW_PRESS;
    }

    if(key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) {
        s->m_ctrl_down = action == GLFW_PRESS;
    }

    if(s->m_camera != NULL) {
        float theta = 0.5f;
        Control c = s->m_shift_down ? Control::ROTATE2 : Control::ROTATE1;
        if(key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
            s->m_camera->uiInput(c, {-theta, 0});
        } else if(key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
            s->m_camera->uiInput(c, {theta, 0});
        } else if(key == GLFW_KEY_UP && action == GLFW_PRESS) {
            s->m_camera->uiInput(c, {0, theta});
        } else if(key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
            s->m_camera->uiInput(c, {0, -theta});
        } else if(key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12) {
            int slot = key - GLFW_KEY_F1;
            if(action == GLFW_PRESS) {
                if(mods & GLFW_MOD_SHIFT) {
                    s->m_camera->transformStore(slot);
                } else {
                    s->m_camera->transformRecall(slot);
                }
                return;
            }
        }
    }

    ImGuiIO &io = ImGui::GetIO();
    if(!io.WantCaptureMouse) {
        if(s->m_hit.object && action == GLFW_PRESS && key == GLFW_KEY_SPACE &&
           s->m_camera) {
            s->m_camera->lookat(s->m_hit.world_pos);
        }

        if(action == GLFW_PRESS) {
            const char *name = glfwGetKeyName(key, scancode);
            if(name && s->m_keypress)
                s->m_keypress(name);
        }
        return;
    }
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

static void
CharCallback(GLFWwindow *window, unsigned int c)
{
    ImGuiIO &io = ImGui::GetIO();
    if(!io.WantCaptureMouse) {
        return;
    }
    ImGui_ImplGlfw_CharCallback(window, c);
}

GLFWImguiScene::GLFWImguiScene(const char *title, int width, int height,
                               float ground_size)
  : m_width(width), m_height(height)
{
    glfwSetErrorCallback(glfw_error_callback);
    if(!glfwInit()) {
        fprintf(stderr, "Unable to init GLFW\n");
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(width, height, title, NULL, NULL);
    if(m_window == NULL) {
        fprintf(stderr, "Unable to open window\n");
        exit(1);
    }
    glfwSetWindowUserPointer(m_window, this);
    glfwSetWindowPos(m_window, 50, 50);
    glfwSetWindowSizeCallback(m_window, resize_callback);
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    glewInit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, false);

    glfwSetWindowFocusCallback(m_window, ImGui_ImplGlfw_WindowFocusCallback);
    glfwSetCursorEnterCallback(m_window, ImGui_ImplGlfw_CursorEnterCallback);
    glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
    glfwSetScrollCallback(m_window, ScrollCallback);
    glfwSetKeyCallback(m_window, KeyCallback);
    glfwSetCharCallback(m_window, CharCallback);
    glfwSetMonitorCallback(ImGui_ImplGlfw_MonitorCallback);

    ImGui_ImplOpenGL3_Init();

    glGenVertexArrays(1, &m_vao);

    m_crosshair = makeCross();

    m_skybox = makeSkybox();

    if(ground_size)
        m_ground = makeGround(ground_size);
}

GLFWImguiScene::~GLFWImguiScene()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void
GLFWImguiScene::drag(Control c, const glm::vec2 &delta, Grab &g,
                     const glm::vec2 &xy)
{
    if(g.m_hit.object) {
        g.m_hit.object->uiInput(g.m_hit, c, delta, *m_camera, xy);
        return;
    }
    m_camera->uiInput(c, delta);
}

bool
GLFWImguiScene::prepare()
{
    if(glfwWindowShouldClose(m_window))
        return false;

    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(m_width * 0.75f, 0));
    ImGui::SetNextWindowSize(ImVec2(m_width * 0.25f, m_height));

    m_scene_editor_visible = ImGui::Begin(
        "Scene", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::End();

    m_scene_editor_start = m_scene_editor_visible ? 0.75f : 1.0f;

    auto cursor = normalizedCursor();
    auto cursor_delta = cursor - m_cursor_prev;

    if(m_camera != NULL) {
        m_camera->update(m_width * m_scene_editor_start, m_height);

        if(!m_left_grab.m_on && !m_right_grab.m_on) {
            m_hit.distance = INFINITY;
            m_hit.object = nullptr;

            auto origin = m_camera->origin();
            auto direction = m_camera->direction(cursor);

            for(auto &o : m_objects) {
                if(o->m_visible) {
                    o->hit(origin, direction, glm::mat4{1}, m_hit);
                }
            }
        }

        if(m_left_grab.m_on) {
            if(m_shift_down) {
                drag(Control::ROTATE2, cursor_delta, m_left_grab, cursor);
            } else {
                drag(Control::ROTATE1, cursor_delta, m_left_grab, cursor);
            }
        } else if(m_right_grab.m_on) {
            if(m_shift_down) {
                drag(Control::TRANSLATE2, cursor_delta, m_right_grab, cursor);
            } else {
                drag(Control::TRANSLATE1, cursor_delta, m_right_grab, cursor);
            }
        }

        if(ImGui::Begin("Scene")) {
            ImGui::Checkbox("Record", &m_record);

            if(ImGui::CollapsingHeader("Camera")) {
                m_camera->ui();
            }

            if(ImGui::CollapsingHeader("Environment")) {
                if(m_skybox)
                    m_skybox->ui(*this);

                if(m_ground)
                    m_ground->ui(*this);

                ImGui::Checkbox("Crosshair", &m_crosshair->m_visible);
            }

            if(ImGui::CollapsingHeader("Measure")) {
                if(m_hit.object) {
                    ImGui::Text("    X:% -9.2f Y:% -9.2f Z:% -9.2f",
                                m_hit.world_pos.x, m_hit.world_pos.y,
                                m_hit.world_pos.z);
                } else {
                    ImGui::Text("No hit");
                }

                if(m_p1) {
                    ImGui::Text("P1  X:% -9.2f Y:% -9.2f Z:% -9.2f", m_p1->x,
                                m_p1->y, m_p1->z);
                }

                if(m_p2) {
                    ImGui::Text("P2  X:% -9.2f Y:% -9.2f Z:% -9.2f", m_p2->x,
                                m_p2->y, m_p2->z);
                }

                if(m_p1 && m_p2) {
                    ImGui::Text("Distance: %f", glm::distance(*m_p1, *m_p2));
                }
            }
        }
        ImGui::End();

        float ch_size = m_camera->distance() * 0.025f;
        m_crosshair->setModelMatrix(
            glm::scale(glm::translate(glm::mat4{1}, m_camera->lookAt()),
                       glm::vec3{ch_size}));
    }

    if(ImGui::Begin("Scene")) {
        for(auto &o : m_objects) {
            if(!o->m_name)
                continue;

            ImGui::PushID((void *)o.get());
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            if(ImGui::CollapsingHeader((*o->m_name).c_str())) {
                o->ui(*this);
            }
            ImGui::PopID();
        }
    }
    ImGui::End();

    m_cursor_prev = cursor;

    return true;
}

glm::vec2
GLFWImguiScene::normalizedCursor()
{
    double mouse_x, mouse_y;
    int display_w, display_h;

    glfwGetCursorPos(m_window, &mouse_x, &mouse_y);
    glfwGetWindowSize(m_window, &display_w, &display_h);

    auto cursor = (glm::vec2(mouse_x, mouse_y) /
                   glm::vec2(display_w * m_scene_editor_start, display_h)) *
                      2.0f -
                  1.0f;
    return cursor;
}

void
GLFWImguiScene::draw()
{
    ImGui::Render();

    glBindVertexArray(m_vao);

    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w * m_scene_editor_start, display_h);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    if(m_skybox && m_skybox->m_visible)
        m_skybox->draw(*this, *m_camera, glm::mat4{1});

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for(auto &o : m_objects) {
        if(o->m_visible) {
            o->draw(*this, *m_camera, glm::mat4{1});
        }
    }

    if(m_ground && m_ground->m_visible)
        m_ground->draw(*this, *m_camera, glm::mat4{1});

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    if(m_crosshair->m_visible)
        m_crosshair->draw(*this, *m_camera, glm::mat4{1});

    if(m_p1_crosshair)
        m_p1_crosshair->draw(*this, *m_camera, glm::mat4{1});
    if(m_p2_crosshair)
        m_p2_crosshair->draw(*this, *m_camera, glm::mat4{1});

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwMakeContextCurrent(m_window);
    glfwSwapBuffers(m_window);

    if(m_record)
        save_frame();
}

void
GLFWImguiScene::mark()
{
    if(m_measure_phase) {
        m_p2 = m_hit.world_pos;

        if(!m_p2_crosshair)
            m_p2_crosshair = makeCross();

        m_p2_crosshair->setModelMatrix(
            glm::scale(glm::translate(glm::mat4{1}, *m_p2), glm::vec3{10}));

    } else {
        m_p1 = m_hit.world_pos;

        if(!m_p1_crosshair)
            m_p1_crosshair = makeCross();

        m_p1_crosshair->setModelMatrix(
            glm::scale(glm::translate(glm::mat4{1}, *m_p1), glm::vec3{10}));
    }

    m_measure_phase = !m_measure_phase;
}

std::shared_ptr<Scene>
makeGLFWImguiScene(const char *title, int width, int height, float ground_size)
{
    return std::make_shared<GLFWImguiScene>(title, width, height, ground_size);
}

}  // namespace g3d
