var Window = require('bindings')('glfw3-window').Window;

var window = new Window(800, 600, 'hello glfw3 world', false);
window.eventHandler(function(ev) {
  console.log(ev);
})

