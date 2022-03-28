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

    GLFWwindow *m_window;
    int m_width;
    int m_height;

    GLuint m_vao;

    float m_fov{45};

    std::shared_ptr<Object> m_crosshair;
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
scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    GLFWImguiScene *s = (GLFWImguiScene *)glfwGetWindowUserPointer(window);

    if(s->m_camera != NULL)
        s->m_camera->scrollInput(xoffset, yoffset);
}

static void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    GLFWImguiScene *s = (GLFWImguiScene *)glfwGetWindowUserPointer(window);

    if(s->m_camera != NULL) {
        if(key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12) {
            int slot = key - GLFW_KEY_F1;
            if(action == GLFW_PRESS) {
                if(mods & GLFW_MOD_SHIFT) {
                    s->m_camera->positionStore(slot);
                } else {
                    s->m_camera->positionRecall(slot);
                }
            }
        }
    }
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
    glfwSetScrollCallback(m_window, scroll_callback);
    glfwSetKeyCallback(m_window, key_callback);

    glfwSetWindowSizeCallback(m_window, resize_callback);
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    glewInit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init();

    glGenVertexArrays(1, &m_vao);

    m_crosshair = makeCross();
    m_objects.push_back(m_crosshair);
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

    if(m_camera != NULL) {
        if(ImGui::Begin("Camera")) {
            ImGui::SliderFloat("FOV", &m_fov, 1, 180);

            ImGui::Checkbox("Crosshair", &m_crosshair->m_visible);

            auto proj = glm::perspective(
                glm::radians(m_fov), (float)m_width / m_height, 10.0f, -10.0f);

            m_P = proj;
            m_V = m_camera->compute();
        }
        ImGui::End();
    }

    if(m_skybox != NULL) {
        ImGui::PushID((void *)m_skybox.get());
        m_skybox->prepare();
        ImGui::PopID();
    }

    m_crosshair->setModelMatrix(glm::scale(
        glm::translate(glm::mat4{1}, m_camera->lookAt()), {100, 100, 100}));

    for(auto &o : m_objects) {
        ImGui::PushID((void *)o.get());
        o->prepare();
        ImGui::PopID();
    }

    return true;
}

glm::vec3
GLFWImguiScene::cursorDirection()
{
    double mouse_x, mouse_y;
    int display_w, display_h;

    glfwGetCursorPos(m_window, &mouse_x, &mouse_y);
    glfwGetWindowSize(m_window, &display_w, &display_h);

    auto cursor =
        (glm::vec2(mouse_x, mouse_y) / glm::vec2(display_w, display_h)) * 2.0f -
        1.0f;
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
    glViewport(0, 0, display_w, display_h);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(m_skybox && m_skybox->m_visible)
        m_skybox->draw(m_P, m_V);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for(auto &o : m_objects) {
        if(o->m_visible)
            o->draw(m_P, m_V);
    }

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
