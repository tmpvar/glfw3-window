#ifndef WINDOW_H
#define WINDOW_H

#define GLFW_INCLUDE_GLU 1

#include <node.h>
#include <GL/glew.h>
#include <nan.h>


#ifdef _WIN32
#include <GL/glu.h>
#endif

#include <GLFW/glfw3.h>

using namespace v8;
using namespace node;

class Window : public ObjectWrap {
 public:
  static void Init(Handle<Object> exports);
  static Handle<Value> NewInstance(const Arguments& args);

  Persistent<Object> canvasHandle;
  GLFWwindow *handle;
  int width, height, x, y;
  double scrollX, scrollY;
  GLuint surfaceTexture[1];
  NanCallback *eventCallback;
  bool hasEventHandler;
  void setupSize();
  void destroy();
  void swapBuffers();

 private:
  Window(int width, int height, const char *title, bool fullscreen);
  ~Window();

  uv_timer_t *input_timer;
  static int window_count;

  static Persistent<Function> constructor;

  static Handle<Value> New(const Arguments& args);

  static NAN_METHOD(resizeTo);
  static NAN_METHOD(moveTo);
  static NAN_METHOD(getRect);
  static NAN_METHOD(flush);
  static NAN_METHOD(eventHandler);
  static NAN_METHOD(setTitle);
  static NAN_METHOD(close);
  static NAN_METHOD(focus);
  static NAN_METHOD(blur);
};

#endif
