#include <node.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "uv.h"
#include "window.h"

using namespace v8;
using namespace node;

void init(Handle<Object> exports) {
  assert(glfwInit());

  Window::Init(exports);
}

NODE_MODULE(glfw3_window, init)
