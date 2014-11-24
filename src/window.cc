#include <node.h>
#include <nan.h>

#include "window.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

using namespace v8;
using namespace node;
//
// Event processors
//

#define EMIT_EVENT\
  Local<Value> argv[] = { event }; \
  win->eventCallback->Call(1, argv); \

void uv_gui_idler(uv_timer_t* timer, int status) {
  glfwPollEvents();
}

void APIENTRY refreshCallback(GLFWwindow* window) {
  Window *win = (Window *)glfwGetWindowUserPointer(window);

  if (!win->handle) {
    return;
  }

  win->swapBuffers();
}

void APIENTRY resizedCallback(GLFWwindow* window,int width,int height) {
  NanScope();

  Window *win = (Window *)glfwGetWindowUserPointer(window);

  if (!win->handle || !win->hasEventHandler) {
    return;
  }

  Local<Object> event = NanNew<Object>();
  event->Set(NanNew<String>("type"), NanNew<String>("resize"));
  event->Set(NanNew<String>("objectType"), NanNew<String>("Event"));
  event->Set(NanNew<String>("width"), NanNew<Number>(width));
  event->Set(NanNew<String>("height"), NanNew<Number>(height));
  event->Set(NanNew<String>("_defaultPrevented"), NanNew<Boolean>(false));

  EMIT_EVENT

  if (!event->Get(NanNew<String>("_defaultPrevented"))->BooleanValue()) {
    win->width = width;
    win->height = height;
    win->setupSize();
  } else {
    glfwSetWindowSize(win->handle, win->width, win->height);
    glfwSetWindowPos(win->handle, win->x, win->y);
  }
}

void APIENTRY movedCallback(GLFWwindow* window,int x,int y) {
  NanScope();

  Window *win = (Window *)glfwGetWindowUserPointer(window);

  if (!win->handle || !win->hasEventHandler) {
    return;
  }

  Local<Object> event = NanNew<Object>();
  event->Set(NanNew<String>("type"), NanNew<String>("move"));
  event->Set(NanNew<String>("objectType"), NanNew<String>("Event"));
  event->Set(NanNew<String>("x"), NanNew<Number>(x));
  event->Set(NanNew<String>("y"), NanNew<Number>(y));
  event->Set(NanNew<String>("_defaultPrevented"), NanNew<Boolean>(false));

  EMIT_EVENT

  if (!event->Get(NanNew<String>("_defaultPrevented"))->BooleanValue()) {
    win->x = x;
    win->y = y;
  } else {
    glfwSetWindowPos(win->handle, win->x, win->y);
  }
};


void APIENTRY closeCallback(GLFWwindow* window) {
  NanScope();

  Window *win = (Window *)glfwGetWindowUserPointer(window);

  if (!win->handle || !win->hasEventHandler) {
    return;
  }

  Local<Object> event = NanNew<Object>();
  event->Set(NanNew<String>("objectType"), NanNew<String>("Event"));
  event->Set(NanNew<String>("type"), NanNew<String>("close"));
  event->Set(NanNew<String>("_defaultPrevented"), NanNew<Boolean>(false));

  EMIT_EVENT

  bool closePrevented = event->Get(NanNew<String>("_defaultPrevented"))->BooleanValue();

  if (!closePrevented) {
    win->destroy();
  }
};


void APIENTRY focusCallback(GLFWwindow* window, int hasFocus) {
  NanScope();

  Window *win = (Window *)glfwGetWindowUserPointer(window);

  if (!win->handle || !win->hasEventHandler) {
    return;
  }

  Local<Object> event = NanNew<Object>();
  event->Set(NanNew<String>("objectType"), NanNew<String>("Event"));
  if (hasFocus) {
    event->Set(NanNew<String>("type"), NanNew<String>("focus"));
  } else {
    event->Set(NanNew<String>("type"), NanNew<String>("blur"));
  }

  EMIT_EVENT
};

void APIENTRY mouseMoveCallback(GLFWwindow* window, double x, double y) {
  NanScope();

  Window *win = (Window *)glfwGetWindowUserPointer(window);

  if (!win->handle || !win->hasEventHandler) {
    return;
  }

  Local<Object> event = NanNew<Object>();
  event->Set(NanNew<String>("type"), NanNew<String>("mousemove"));
  event->Set(NanNew<String>("objectType"), NanNew<String>("MouseEvent"));
  event->Set(NanNew<String>("x"), NanNew<Number>(x));
  event->Set(NanNew<String>("y"), NanNew<Number>(y));

  EMIT_EVENT
};

void APIENTRY mouseEnterExitCallback(GLFWwindow* window, int inside) {
  NanScope();

  Window *win = (Window *)glfwGetWindowUserPointer(window);

  if (!win->handle || !win->hasEventHandler) {
    return;
  }

  Local<Object> event = NanNew<Object>();

  if (inside) {
    event->Set(NanNew<String>("type"), NanNew<String>("mouseenter"));
  } else {
    event->Set(NanNew<String>("type"), NanNew<String>("mouseleave"));
  }
  event->Set(NanNew<String>("objectType"), NanNew<String>("MouseEvent"));

  double x, y;
  glfwGetCursorPos(win->handle, &x, &y);
  event->Set(NanNew<String>("x"), NanNew<Number>(x));
  event->Set(NanNew<String>("y"), NanNew<Number>(y));

  EMIT_EVENT
};

void APIENTRY mouseButtonCallback(GLFWwindow* window, int button, int pressed, int mods) {
  NanScope();

  Window *win = (Window *)glfwGetWindowUserPointer(window);

  if (!win->handle || !win->hasEventHandler) {
    return;
  }

  Local<Object> event = NanNew<Object>();

  switch (pressed) {
    case GLFW_PRESS:
      event->Set(NanNew<String>("type"), NanNew<String>("mousedown"));
    break;

    case GLFW_RELEASE:
      event->Set(NanNew<String>("type"), NanNew<String>("mouseup"));
    break;
  }

  switch (button) {
    // left
    case GLFW_MOUSE_BUTTON_1:
      event->Set(NanNew<String>("button"), NanNew<Number>(0));
    break;

    // middle
    case GLFW_MOUSE_BUTTON_3:
      event->Set(NanNew<String>("button"), NanNew<Number>(1));
    break;

    // right
    case GLFW_MOUSE_BUTTON_2:
      event->Set(NanNew<String>("button"), NanNew<Number>(2));
    break;

    // handle the other buttons
    default:
      event->Set(NanNew<String>("button"), NanNew<Number>(button));
    break;
  }

  event->Set(NanNew<String>("objectType"), NanNew<String>("MouseEvent"));

  double x, y;
  glfwGetCursorPos(win->handle, &x, &y);
  event->Set(NanNew<String>("x"), NanNew<Number>(x));
  event->Set(NanNew<String>("y"), NanNew<Number>(y));

  EMIT_EVENT
};

void APIENTRY mouseScrollCallback(GLFWwindow* window, double x, double y) {
  NanScope();

  Window *win = (Window *)glfwGetWindowUserPointer(window);

  if (!win->handle || !win->hasEventHandler) {
    return;
  }

  double dx = win->scrollX - x;
  double dy = win->scrollY - y;

  win->scrollX = x;
  win->scrollY = y;

  Local<Object> event = NanNew<Object>();
  event->Set(NanNew<String>("type"), NanNew<String>("mousewheel"));
  event->Set(NanNew<String>("wheelDelta"), NanNew<Number>(dy));
  event->Set(NanNew<String>("wheelDeltaX"), NanNew<Number>(dx));
  event->Set(NanNew<String>("wheelDeltaY"), NanNew<Number>(dy));

  EMIT_EVENT
}


void APIENTRY keyboardKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  NanScope();

  Window *win = (Window *)glfwGetWindowUserPointer(window);

  if (!win->handle || !win->hasEventHandler) {
    return;
  }

  if (key == GLFW_KEY_RIGHT_SHIFT || key == GLFW_KEY_LEFT_SHIFT) {
    return;
  }

  Local<Object> event = NanNew<Object>();

  bool repeat = false;

  switch (action) {
    case GLFW_PRESS:
      event->Set(NanNew<String>("type"), NanNew<String>("keydown"));
    break;

    case GLFW_RELEASE:
      event->Set(NanNew<String>("type"), NanNew<String>("keyup"));
    break;

    case GLFW_REPEAT:
      event->Set(NanNew<String>("type"), NanNew<String>("keydown"));
      repeat = true;
    break;
  }

  bool shift = glfwGetKey(win->handle, GLFW_KEY_RIGHT_SHIFT) ||
              glfwGetKey(win->handle, GLFW_KEY_RIGHT_SHIFT);

  bool meta =  glfwGetKey(win->handle, GLFW_KEY_LEFT_SUPER) ||
              glfwGetKey(win->handle, GLFW_KEY_RIGHT_SUPER);

  bool alt = glfwGetKey(win->handle, GLFW_KEY_LEFT_ALT) ||
            glfwGetKey(win->handle, GLFW_KEY_LEFT_ALT);

  bool control = glfwGetKey(win->handle, GLFW_KEY_RIGHT_CONTROL) ||
                glfwGetKey(win->handle, GLFW_KEY_RIGHT_CONTROL);

  bool capsLock =  glfwGetKey(win->handle, GLFW_KEY_CAPS_LOCK) == 1;

  int location = 0;

  switch(key) {
    case GLFW_KEY_ESCAPE: key = 27; break;
    case GLFW_KEY_ENTER: key = 13; break;
    case GLFW_KEY_TAB: key = 9; break;
    case GLFW_KEY_BACKSPACE: key = 8; break;
    case GLFW_KEY_INSERT: key = 45; break;
    case GLFW_KEY_DELETE: key = 46; break;
    case GLFW_KEY_RIGHT: key = 39; break;
    case GLFW_KEY_LEFT: key = 37; break;
    case GLFW_KEY_DOWN: key = 40; break;
    case GLFW_KEY_UP: key = 38; break;
    case GLFW_KEY_PAGE_UP: key = 33; break;
    case GLFW_KEY_PAGE_DOWN: key = 34; break;
    case GLFW_KEY_HOME: key = 36; break;
    case GLFW_KEY_END: key = 35; break;
    case GLFW_KEY_CAPS_LOCK: key = 20; break;
    case GLFW_KEY_SCROLL_LOCK: key = 145; break;
    case GLFW_KEY_NUM_LOCK: key = 144; break;
    case GLFW_KEY_PRINT_SCREEN: key = 44; break;
    case GLFW_KEY_PAUSE: key = 19; break;
    case GLFW_KEY_F1: key = 112; break;
    case GLFW_KEY_F2: key = 113; break;
    case GLFW_KEY_F3: key = 114; break;
    case GLFW_KEY_F4: key = 115; break;
    case GLFW_KEY_F5: key = 116; break;
    case GLFW_KEY_F6: key = 117; break;
    case GLFW_KEY_F7: key = 118; break;
    case GLFW_KEY_F8: key = 119; break;
    case GLFW_KEY_F9: key = 120; break;
    case GLFW_KEY_F10: key = 121; break;
    case GLFW_KEY_F11: key = 122; break;
    case GLFW_KEY_F12: key = 123; break;
    case GLFW_KEY_F13: key = 124; break;
    case GLFW_KEY_F14: key = 125; break;
    case GLFW_KEY_F15: key = 126; break;
    case GLFW_KEY_F16: key = 127; break;
    case GLFW_KEY_F17: key = 128; break;
    case GLFW_KEY_F18: key = 129; break;
    case GLFW_KEY_F19: key = 130; break;
    case GLFW_KEY_F20: key = 131; break;
    case GLFW_KEY_F21: key = 132; break;
    case GLFW_KEY_F22: key = 133; break;
    case GLFW_KEY_F23: key = 134; break;
    case GLFW_KEY_F24: key = 135; break;
    case GLFW_KEY_F25: key = 136; break;
    case GLFW_KEY_KP_0: key = 96; location=3; break;
    case GLFW_KEY_KP_1: key = 97; location=3; break;
    case GLFW_KEY_KP_2: key = 98; location=3; break;
    case GLFW_KEY_KP_3: key = 99; location=3; break;
    case GLFW_KEY_KP_4: key = 100; location=3; break;
    case GLFW_KEY_KP_5: key = 101; location=3; break;
    case GLFW_KEY_KP_6: key = 102; location=3; break;
    case GLFW_KEY_KP_7: key = 103; location=3; break;
    case GLFW_KEY_KP_8: key = 104; location=3; break;
    case GLFW_KEY_KP_9: key = 105; location=3; break;
    case GLFW_KEY_KP_DECIMAL: key = 110; location=3; break;
    case GLFW_KEY_KP_DIVIDE: key = 111; location=3; break;
    case GLFW_KEY_KP_MULTIPLY: key = 106; location=3; break;
    case GLFW_KEY_KP_SUBTRACT: key = 109; location=3; break;
    case GLFW_KEY_KP_ADD: key = 107; location=3; break;
    case GLFW_KEY_KP_ENTER: key = 13; location=3; break;
    case GLFW_KEY_KP_EQUAL: key = 187; location=3; break;
    case GLFW_KEY_LEFT_SHIFT: key = 16; location=1; break;
    case GLFW_KEY_LEFT_CONTROL: key = 17; location=1; break;
    case GLFW_KEY_LEFT_ALT: key = 18; location=1; break;
    case GLFW_KEY_LEFT_SUPER: key = 91; location=1; break;
    case GLFW_KEY_RIGHT_SHIFT: key = 16; location=2; break;
    case GLFW_KEY_RIGHT_CONTROL: key = 17; location=2; break;
    case GLFW_KEY_RIGHT_ALT: key = 18; location=2; break;
    case GLFW_KEY_RIGHT_SUPER: key = 92; location=2; break;
    case GLFW_KEY_MENU: key = 18; break;
    case GLFW_KEY_M: key = 0; break;
  }

  event->Set(NanNew<String>("keyCode"), NanNew<Integer>(key));
  event->Set(NanNew<String>("which"), NanNew<Integer>(key));
  // event->Set(NanNew<String>("keyIdentifier"), glfw);

  event->Set(NanNew<String>("ctrlKey"), NanNew<Boolean>(control));
  event->Set(NanNew<String>("shiftKey"), NanNew<Boolean>(shift));
  event->Set(NanNew<String>("altKey"), NanNew<Boolean>(alt));
  event->Set(NanNew<String>("metaKey"), NanNew<Boolean>(meta));
  event->Set(NanNew<String>("repeat"), NanNew<Boolean>(repeat));
  event->Set(NanNew<String>("location"), NanNew<Integer>(location));

  event->Set(NanNew<String>("objectType"), NanNew<String>("KeyboardEvent"));

  EMIT_EVENT
};


int Window::window_count = 0;

Window::Window(int width, int height, const char *title, bool fullscreen)
  : ObjectWrap()
{
  this->width = width;
  this->height = height;
  this->scrollX = 0.0;
  this->scrollY = 0.0;

  glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
  // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
  // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL


  this->handle = glfwCreateWindow(
    width,
    height,
    title,
    fullscreen ? glfwGetPrimaryMonitor() : NULL,
    NULL
  );

  assert(this->handle);

  glfwMakeContextCurrent(this->handle);

  glewExperimental = true;
  if (glewInit() != GLEW_OK) {
    printf("failed to init glew\n");
    assert(0);
  }

  Window::window_count++;
  this->hasEventHandler = false;

  glfwGetWindowPos(this->handle, &this->x, &this->y);

  glfwSetWindowUserPointer(this->handle, this);

  glfwSetWindowSizeCallback(this->handle, &resizedCallback);
  glfwSetWindowPosCallback(this->handle, &movedCallback);
  glfwSetWindowCloseCallback(this->handle, &closeCallback);
  glfwSetWindowFocusCallback(this->handle, &focusCallback);

  // Mouse
  glfwSetMouseButtonCallback(this->handle, &mouseButtonCallback);
  glfwSetCursorPosCallback(this->handle, &mouseMoveCallback);
  glfwSetCursorEnterCallback(this->handle, &mouseEnterExitCallback);
  glfwSetScrollCallback(this->handle, &mouseScrollCallback);

  // Keyboard
  glfwSetKeyCallback(this->handle, &keyboardKeyCallback);

  // TODO: need to integrate this and keycallback
  // glfwSetCharCallback(this->handle, &keyboardCharCallback);

  // TODO: joystick / gamepad

  this->input_timer = (uv_timer_t *)malloc(sizeof(uv_timer_t));
  uv_timer_init(uv_default_loop(), this->input_timer);
  uv_timer_start(this->input_timer, uv_gui_idler, 5, 10);
}

Window::~Window() {
  if (Window::window_count <= 0) {
    glfwTerminate();
  }
}

void Window::destroy() {
  if (this->handle) {
    glfwDestroyWindow(this->handle);
    this->handle = NULL;
    Window::window_count--;

    uv_timer_stop(this->input_timer);
    free(this->input_timer);
    this->input_timer = NULL;
  }
}

Persistent<Function> Window::constructor;

void Window::Init(Handle<Object> exports) {

  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
  tpl->SetClassName(NanNew<String>("Window"));
  tpl->InstanceTemplate()->SetInternalFieldCount(11);

  NODE_SET_PROTOTYPE_METHOD(tpl, "resizeTo", Window::resizeTo);
  NODE_SET_PROTOTYPE_METHOD(tpl, "moveTo", Window::moveTo);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getRect", Window::getRect);
  NODE_SET_PROTOTYPE_METHOD(tpl, "flush", Window::flush);
  NODE_SET_PROTOTYPE_METHOD(tpl, "eventHandler", Window::eventHandler);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setTitle", Window::setTitle);
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", Window::close);

  NanAssignPersistent<v8::Function>(constructor, tpl->GetFunction());
  exports->Set(NanNew<String>("Window"), tpl->GetFunction());
}

NAN_METHOD(Window::eventHandler) {
  NanScope();

  Window *win = ObjectWrap::Unwrap<Window>(args.This());

  win->eventCallback = new NanCallback(
    v8::Local<v8::Function>::Cast(args[0])
  );

  win->hasEventHandler = true;

  NanReturnUndefined();
}


NAN_METHOD(Window::setTitle) {
  NanScope();

  Window *win = ObjectWrap::Unwrap<Window>(args.This());

  size_t count;
  char* title = NanCString(args[0], &count);

  if (win->handle) {
    glfwSetWindowTitle(win->handle, title);
  }

  delete[] title;

  NanReturnUndefined();
}

NAN_METHOD(Window::close) {
  NanScope();
  Window *win = ObjectWrap::Unwrap<Window>(args.This());
  win->Unref();

  if (win->handle) {
    glfwSetWindowShouldClose(win->handle, 1);
    win->destroy();
  }

  NanReturnUndefined();
}

void Window::setupSize() {
  if (this->handle) {
    glfwMakeContextCurrent(this->handle);
  }
}

NAN_METHOD(Window::resizeTo) {
  NanScope();

  Window *win = ObjectWrap::Unwrap<Window>(args.This());

  win->width = args[0]->Int32Value();
  win->height = args[1]->Int32Value();

  if (win->handle) {
    glfwSetWindowSize(win->handle, win->width, win->height);
  }

  win->setupSize();

  NanReturnUndefined();
}

NAN_METHOD(Window::moveTo) {
  NanScope();

  Window *win = ObjectWrap::Unwrap<Window>(args.This());

  win->x = args[0]->Int32Value();
  win->y = args[1]->Int32Value();

  if (win->handle) {
    glfwSetWindowPos(win->handle, win->x, win->y);
  }

  NanReturnUndefined();
}

void Window::swapBuffers() {
  if (this->handle) {
    glfwMakeContextCurrent(this->handle);
    glfwSwapBuffers(this->handle);
  }
}

NAN_METHOD(Window::flush) {
  NanScope();
  Window *win = ObjectWrap::Unwrap<Window>(args.This());
  win->swapBuffers();
  NanReturnUndefined();
}

NAN_METHOD(Window::getRect) {
  NanScope();

  Window *win = ObjectWrap::Unwrap<Window>(args.This());
  if (win->handle) {
    int width, height, x, y;
    glfwGetWindowSize(win->handle, &width, &height);
    glfwGetWindowPos(win->handle, &x, &y);

    Local<Object> ret = NanNew<Object>();
    ret->Set(NanNew<String>("x"), NanNew<Number>(x));
    ret->Set(NanNew<String>("y"), NanNew<Number>(y));
    ret->Set(NanNew<String>("width"), NanNew<Number>(width));
    ret->Set(NanNew<String>("height"), NanNew<Number>(height));

    NanReturnValue(ret);
  } else {
    NanReturnUndefined();
  }
}


NAN_METHOD(Window::New) {
  NanScope();

  int width = args[0]->Int32Value();
  int height = args[1]->Int32Value();

  size_t count;
  char* title = NanCString(args[2], &count);

  Window *window = new Window(
    width,
    height,
    title,
    args[3]->BooleanValue()
  );

  window->Wrap(args.This());
  window->Ref();

  delete[] title;

  NanReturnValue(args.This());
}
