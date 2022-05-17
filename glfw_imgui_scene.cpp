#include "scene.hpp"

#include "opengl.hpp"
#include "camera.hpp"
#include "object.hpp"

#include <glm/gtx/string_cast.hpp>

namespace g3d {

struct GLFWImguiScene : public Scene {
    GLFWImguiScene(const char *title, int width, int height);

    glm::vec3 cursorDirection() override;

    ~GLFWImguiScene();

    bool prepare() override;

    void draw() override;

    glm::vec2 normalizedCursor();

    GLFWwindow *m_window;
    int m_width;
    int m_height;

    GLuint m_vao;

    float m_fov{45};

    std::shared_ptr<Object> m_crosshair;
    std::shared_ptr<Object> m_skybox;
    std::shared_ptr<Object> m_ground;

    glm::vec2 m_cursor_prev{0};

    bool m_left_down{false};

    bool m_right_down{false};

    bool m_shift_down{false};

    bool m_alt_down{false};

    bool m_scene_editor_visible{true};

    float m_scene_editor_start{0.75};
};

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

        if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            s->m_left_down = true;
        }
        if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            s->m_left_down = false;
        }
        if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            s->m_right_down = true;
        }
        if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
            s->m_right_down = false;
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
        s->m_camera->uiInput(Camera::Control::SCROLL,
                             glm::vec2{-xoffset, yoffset} * scale);
        return;
    }
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

static void
KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    GLFWImguiScene *s = (GLFWImguiScene *)glfwGetWindowUserPointer(window);

    if(key == GLFW_KEY_LEFT_SHIFT) {
        s->m_shift_down = action == GLFW_PRESS;
    }

    if(key == GLFW_KEY_LEFT_ALT) {
        s->m_alt_down = action == GLFW_PRESS;
    }

    if(s->m_camera != NULL) {
        if(key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12) {
            int slot = key - GLFW_KEY_F1;
            if(action == GLFW_PRESS) {
                if(mods & GLFW_MOD_SHIFT) {
                    s->m_camera->positionStore(slot);
                } else {
                    s->m_camera->positionRecall(slot);
                }
                return;
            }
        }
    }

    ImGuiIO &io = ImGui::GetIO();
    if(!io.WantCaptureMouse) {
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

GLFWImguiScene::GLFWImguiScene(const char *title, int width, int height)
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

    m_ground = makeGround(1000);
}

GLFWImguiScene::~GLFWImguiScene()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(m_window);
    glfwTerminate();
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
        if(m_left_down) {
            if(m_alt_down) {
                m_camera->uiInput(Camera::Control::DRAG3, cursor_delta);
            } else if(m_shift_down) {
                m_camera->uiInput(Camera::Control::DRAG2, cursor_delta);
            } else {
                m_camera->uiInput(Camera::Control::DRAG1, cursor_delta);
            }
        } else if(m_right_down) {
            if(m_shift_down) {
                m_camera->uiInput(Camera::Control::DRAG4, cursor_delta);
            } else {
                m_camera->uiInput(Camera::Control::DRAG3, cursor_delta);
            }
        }

        if(ImGui::Begin("Scene")) {
            if(ImGui::CollapsingHeader("Camera")) {
                ImGui::SliderFloat("FOV", &m_fov, 1, 180);

                m_camera->ui();
            }

            if(ImGui::CollapsingHeader("Environment")) {
                if(m_skybox)
                    m_skybox->ui();

                if(m_ground)
                    m_ground->ui();

                ImGui::Checkbox("Crosshair", &m_crosshair->m_visible);
            }
        }
        ImGui::End();

        auto proj = glm::perspective(
            glm::radians(m_fov),
            (float)m_width * m_scene_editor_start / m_height, 10.0f, -10.0f);

        m_P = proj;
        m_V = m_camera->compute();
    }

    m_crosshair->setModelMatrix(glm::scale(
        glm::translate(glm::mat4{1}, m_camera->lookAt()), {100, 100, 100}));

    if(ImGui::Begin("Scene")) {
        for(auto &o : m_objects) {
            ImGui::PushID((void *)o.get());
            if(ImGui::CollapsingHeader(o->m_name.c_str())) {
                o->ui();
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

glm::vec3
GLFWImguiScene::cursorDirection()
{
    auto cursor = normalizedCursor();

    auto ray =
        glm::inverse(m_P * m_V) * glm::vec4(cursor.x, -cursor.y, 0, 1.0f);
    return glm::normalize(glm::vec3(ray));
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
        m_skybox->draw(m_P, m_V);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for(auto &o : m_objects) {
        if(o->m_visible)
            o->draw(m_P, m_V);
    }

    if(m_ground && m_ground->m_visible)
        m_ground->draw(m_P, m_V);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    if(m_crosshair->m_visible)
        m_crosshair->draw(m_P, m_V);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwMakeContextCurrent(m_window);
    glfwSwapBuffers(m_window);
}

std::shared_ptr<Scene>
makeGLFWImguiScene(const char *title, int width, int height)
{
    return std::make_shared<GLFWImguiScene>(title, width, height);
}

}  // namespace g3d
