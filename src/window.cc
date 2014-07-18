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

  Local<Object> event = Object::New();
  event->Set(String::NewSymbol("type"), String::NewSymbol("resize"));
  event->Set(String::NewSymbol("objectType"), String::NewSymbol("Event"));
  event->Set(String::NewSymbol("width"), Number::New(width));
  event->Set(String::NewSymbol("height"), Number::New(height));
  event->Set(String::NewSymbol("_defaultPrevented"), Boolean::New(false));

  EMIT_EVENT

  if (!event->Get(String::NewSymbol("_defaultPrevented"))->BooleanValue()) {
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

  Local<Object> event = Object::New();
  event->Set(String::NewSymbol("type"), String::NewSymbol("move"));
  event->Set(String::NewSymbol("objectType"), String::NewSymbol("Event"));
  event->Set(String::NewSymbol("x"), Number::New(x));
  event->Set(String::NewSymbol("y"), Number::New(y));
  event->Set(String::NewSymbol("_defaultPrevented"), Boolean::New(false));

  EMIT_EVENT

  if (!event->Get(String::NewSymbol("_defaultPrevented"))->BooleanValue()) {
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

  Local<Object> event = Object::New();
  event->Set(String::NewSymbol("objectType"), String::NewSymbol("Event"));
  event->Set(String::NewSymbol("type"), String::NewSymbol("close"));
  event->Set(String::NewSymbol("_defaultPrevented"), Boolean::New(false));

  EMIT_EVENT

  bool closePrevented = event->Get(String::NewSymbol("_defaultPrevented"))->BooleanValue();

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

  Local<Object> event = Object::New();
  event->Set(String::NewSymbol("objectType"), String::NewSymbol("Event"));
  if (hasFocus) {
    event->Set(String::NewSymbol("type"), String::NewSymbol("focus"));
  } else {
    event->Set(String::NewSymbol("type"), String::NewSymbol("blur"));
  }

  EMIT_EVENT
};

void APIENTRY mouseMoveCallback(GLFWwindow* window, double x, double y) {
  NanScope();

  Window *win = (Window *)glfwGetWindowUserPointer(window);

  if (!win->handle || !win->hasEventHandler) {
    return;
  }

  Local<Object> event = Object::New();
  event->Set(String::NewSymbol("type"), String::NewSymbol("mousemove"));
  event->Set(String::NewSymbol("objectType"), String::NewSymbol("MouseEvent"));
  event->Set(String::NewSymbol("x"), Number::New(x));
  event->Set(String::NewSymbol("y"), Number::New(y));

  EMIT_EVENT
};

void APIENTRY mouseEnterExitCallback(GLFWwindow* window, int inside) {
  NanScope();

  Window *win = (Window *)glfwGetWindowUserPointer(window);

  if (!win->handle || !win->hasEventHandler) {
    return;
  }

  Local<Object> event = Object::New();

  if (inside) {
    event->Set(String::NewSymbol("type"), String::NewSymbol("mouseenter"));
  } else {
    event->Set(String::NewSymbol("type"), String::NewSymbol("mouseleave"));
  }
  event->Set(String::NewSymbol("objectType"), String::NewSymbol("MouseEvent"));

  double x, y;
  glfwGetCursorPos(win->handle, &x, &y);
  event->Set(String::NewSymbol("x"), Number::New(x));
  event->Set(String::NewSymbol("y"), Number::New(y));

  EMIT_EVENT
};


void APIENTRY mouseButtonCallback(GLFWwindow* window, int button, int pressed, int mods) {
  NanScope();

  Window *win = (Window *)glfwGetWindowUserPointer(window);

  if (!win->handle || !win->hasEventHandler) {
    return;
  }

  Local<Object> event = Object::New();

  switch (pressed) {
    case GLFW_PRESS:
      event->Set(String::NewSymbol("type"), String::NewSymbol("mousedown"));
    break;

    case GLFW_RELEASE:
      event->Set(String::NewSymbol("type"), String::NewSymbol("mouseup"));
    break;
  }

  switch (button) {
    // left
    case GLFW_MOUSE_BUTTON_1:
      event->Set(String::NewSymbol("button"), Number::New(0));
    break;

    // middle
    case GLFW_MOUSE_BUTTON_3:
      event->Set(String::NewSymbol("button"), Number::New(1));
    break;

    // right
    case GLFW_MOUSE_BUTTON_2:
      event->Set(String::NewSymbol("button"), Number::New(2));
    break;

    // handle the other buttons
    default:
      event->Set(String::NewSymbol("button"), Number::New(button));
    break;
  }

  event->Set(String::NewSymbol("objectType"), String::NewSymbol("MouseEvent"));

  double x, y;
  glfwGetCursorPos(win->handle, &x, &y);
  event->Set(String::NewSymbol("x"), Number::New(x));
  event->Set(String::NewSymbol("y"), Number::New(y));

  EMIT_EVENT
};

void APIENTRY keyboardKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  NanScope();

  Window *win = (Window *)glfwGetWindowUserPointer(window);

  if (!win->handle || !win->hasEventHandler) {
    return;
  }

  if (key == GLFW_KEY_RIGHT_SHIFT || key == GLFW_KEY_LEFT_SHIFT) {
    return;
  }

  Local<Object> event = Object::New();

  bool repeat = false;

  switch (action) {
    case GLFW_PRESS:
      event->Set(String::NewSymbol("type"), String::NewSymbol("keydown"));
    break;

    case GLFW_RELEASE:
      event->Set(String::NewSymbol("type"), String::NewSymbol("keyup"));
    break;

    case GLFW_REPEAT:
      event->Set(String::NewSymbol("type"), String::NewSymbol("keydown"));
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

  event->Set(String::NewSymbol("keyCode"), Integer::New(key));
  event->Set(String::NewSymbol("which"), Integer::New(key));
  // event->Set(String::NewSymbol("keyIdentifier"), glfw);

  event->Set(String::NewSymbol("ctrlKey"), Boolean::New(control));
  event->Set(String::NewSymbol("shiftKey"), Boolean::New(shift));
  event->Set(String::NewSymbol("altKey"), Boolean::New(alt));
  event->Set(String::NewSymbol("metaKey"), Boolean::New(meta));
  event->Set(String::NewSymbol("repeat"), Boolean::New(repeat));
  event->Set(String::NewSymbol("location"), Integer::New(location));

  event->Set(String::NewSymbol("objectType"), String::NewSymbol("KeyboardEvent"));

  EMIT_EVENT
};


int Window::window_count = 0;

Window::Window(int width, int height, const char *title, bool fullscreen)
  : ObjectWrap()
{
  this->width = width;
  this->height = height;

  this->handle = glfwCreateWindow(
    width,
    height,
    title,
    fullscreen ? glfwGetPrimaryMonitor() : NULL,
    NULL
  );

  Window::window_count++;
  this->hasEventHandler = false;

  glfwGetWindowPos(this->handle, &this->x, &this->y);

  glfwSetWindowUserPointer(this->handle, this);

  glfwSetWindowSizeCallback(this->handle, &resizedCallback);
  glfwSetWindowPosCallback(this->handle, &movedCallback);
  glfwSetWindowCloseCallback(this->handle, &closeCallback);
  glfwSetWindowFocusCallback(this->handle, &focusCallback);
  glfwSetMouseButtonCallback(this->handle, &mouseButtonCallback);
  // Mouse
  glfwSetCursorPosCallback(this->handle, &mouseMoveCallback);
  glfwSetCursorEnterCallback(this->handle, &mouseEnterExitCallback);

  // Keyboard
  glfwSetKeyCallback(this->handle, &keyboardKeyCallback);

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

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  exports->Set(NanNew<String>("Window"), constructor);
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

    glViewport(0,0, this->width, this->height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, this->width, 0, this->height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
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

    Local<Object> ret = Object::New();
    ret->Set(String::NewSymbol("x"), Number::New(x));
    ret->Set(String::NewSymbol("y"), Number::New(y));
    ret->Set(String::NewSymbol("width"), Number::New(width));
    ret->Set(String::NewSymbol("height"), Number::New(height));

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
