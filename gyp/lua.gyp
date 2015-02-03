{

  'variables': {
    
    'source_file_name': 'lua.lst',

    'lua_specific_compiler_flags' : [
      '-Wno-deprecated-declarations'
    ],
    'compiler_flags': [
      '-fdiagnostics-show-category=name',
      '-Wall',
      '-Woverloaded-virtual',
      '-Werror',
      '-Wno-invalid-noreturn',
      '-Wshorten-64-to-32',
      '-Winit-self',
      '-Wconditional-uninitialized',
      '-stdlib=libc++', 
      '-g'
    ],
    'compiler_c_flags' : [
      '-std=c11',
      '<@(lua_specific_compiler_flags)',
      '<@(compiler_flags)',
    ],
    'compiler_cpp_flags' : [
      '-std=c++11',
      '<@(lua_specific_compiler_flags)',
      '<@(compiler_flags)'
    ],
  },


  'target_defaults': {
    'cflags': ['<@(compiler_c_flags)'],
    'cflags_cc': ['<@(compiler_cpp_flags)'],
    'xcode_settings': {
      'OTHER_CFLAGS': ['<@(compiler_c_flags)'],
      'OTHER_CPLUSPLUSFLAGS': ['<@(compiler_cpp_flags)'],
    }
  },


  'conditions': [
    [
      "OS=='android'", 
      {
        'target_defaults': {
          'defines': [
            'LUA_USE_ANDROID'
          ],
        }
      },
    ],
    [ 
      "OS=='ios'", 
      {
        'xcode_settings': {
          'SDKROOT': 'iphoneos',
          #'TARGETED_DEVICE_FAMILY': '1,2',
          #'CODE_SIGN_IDENTITY': 'iPhone Developer',
          #'IPHONEOS_DEPLOYMENT_TARGET': '5.1',
        },
        'target_defaults': {
          'defines': [
            'LUA_USE_IOS'
          ]
        }
      },
    ],
    [ 
      "OS=='mac'", 
      {
        'target_defaults': {
          'defines': [
            'LUA_USE_MACOSX'
          ]
        }
      }, 
    ]
  ], 



  'targets': [
    {
      'target_name': 'lua', 
      'target_conditions': [
        [
          "OS=='android'",
          {
            'sources': [ "<!@(sed -e 's/^..\///' <(source_file_name))" ], 
            'include_dirs': [ 'lua-5.2.2/src/' ],
          },
          { #else (not android)
            'sources': [ '<!@(cat <(source_file_name))' ], 
            'include_dirs': [ '../lua-5.2.2/src/' ],
          } #end not android
        ],
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../exportedHeaders/'
        ]
      }, 
      'type': '<(lua_library_type)', 
    } 
  ]

  
}
