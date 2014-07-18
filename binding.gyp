{
  'variables': {
    'platform': '<(OS)',
  },

  'targets': [
    {
      'target_name' : 'glfw3-window',
      'dependencies' : [
        'node_modules/glfw3/binding.gyp:glfw3-static',
      ],
      'sources' : [
        'src/binding.cc',
        'src/window.cc'
      ],
      'include_dirs' : [
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}
