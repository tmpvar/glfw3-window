var Window = require('bindings')('glfw3-window').Window;
console.log(Window);
var window = new Window(800, 600, 'hello glfw3 world', false);
window.eventHandler(function(ev) {
  var x = 0;
  for (var i = 0; i< 100000; i++) {
    x += Math.sqrt(i*(i * 2));
  }
  console.log(x);
})
