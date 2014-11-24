{
  'targets': [
    {
      'target_name' : 'glfw3-window',
      'dependencies' : [
        'node_modules/glfw3/binding.gyp:glfw3-static',
        'node_modules/glew/binding.gyp:glew-static'
      ],
      'sources' : [
        'src/binding.cc',
        'src/window.cc'
      ],
      'include_dirs' : [
        'node_modules/nan'
      ]
    }
  ]
}
