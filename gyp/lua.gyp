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
      '-g'
    ],
    'compiler_c_flags' : [
      '-std=c99',
      '<@(lua_specific_compiler_flags)',
      '<@(compiler_flags)',
    ],
    'compiler_cpp_flags' : [
      '-std=c++11',
      '<@(lua_specific_compiler_flags)',
      '<@(compiler_flags)',
    ],
    'linker_flags' : [
      #nothing so far
    ],
    'target_archs%': ['$(ARCHS_STANDARD_32_64_BIT)'],
    'conditions': [
        [ "OS=='android'", {
            'compiler_flags': [
              '-gcc-toolchain', '<(ndk_root)/toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64',
              '-fpic',
              '-ffunction-sections',
              '-funwind-tables',
              '-fstack-protector',
              '-no-canonical-prefixes',
              '-fno-integrated-as',
              '-target', 'armv7-none-linux-androideabi',
              '-march=armv7-a',
              '-mfloat-abi=softfp',
              '-mfpu=vfpv3-d16',
              '-mthumb',
              '-fomit-frame-pointer',
              '-fno-strict-aliasing',
              '-I<(ndk_root)/platforms/android-18/arch-arm/usr/include',
            ],
            'linker_flags' : [
                '--sysroot=<(ndk_root)/platforms/android-18/arch-arm',
                '-gcc-toolchain', '<(ndk_root)/toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64',
                '-no-canonical-prefixes',
                '-target armv7-none-linux-androideabi',
                '-Wl,--fix-cortex-a8',
                '-Wl,--no-undefined',
                '-Wl,-z,noexecstack',
                '-Wl,-z,relro',
                '-Wl,-z,now',
                '-mthumb',
                '-lgcc',
            ],
        }],
    ],
  },


  'target_defaults': {
    'cflags': ['<@(compiler_c_flags)'],
    'cflags_cc': ['<@(compiler_cpp_flags)'],
    'ldflags': ['<@(linker_flags)'],
    'xcode_settings': {
      'OTHER_CFLAGS': ['<@(compiler_c_flags)'],
      'OTHER_CPLUSPLUSFLAGS': ['<@(compiler_cpp_flags)'],
    },
    'configurations': {
      'Debug': {
          'cflags': ['-O0'],
          'cflags_cc': ['-O0'],
          'defines': [
            'DEBUG=1',
          ],
      },
      'Profile': {
          'cflags': ['-Os'],
          'cflags_cc': ['-Os'],
          'defines': [
            'NDEBUG=1',
            'PROFILE=1',
          ],
      },
      'Release': {
          'cflags': ['-Os'],
          'cflags_cc': ['-Os'],
          'defines': [
            'NDEBUG=1',
            'RELEASE=1',
          ],
      },
    },
  },


  'conditions': [
    ["OS=='android'",
      {
        'target_defaults': {
            'defines': [
              'LUA_USE_ANDROID'
            ],
            'target_archs%': ['armveabi-v7a'],
        },
      },
    ],
    ["OS=='ios'",
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
    ["OS=='mac'",
      {
        'xcode_settings': {
          'ARCHS': ['<@(target_archs)'],
        },
        'target_defaults': {
          'defines': [
            'LUA_USE_MACOSX'
          ]
        }
      },
    ],
  ],

  'targets': [
    {
      'target_name': 'libLua',
       'sources': [ '<!@(cat <(source_file_name))' ],
       'include_dirs': [ '../lua-5.2.2/src/' ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../exportedHeaders/'
        ]
      },
      'type': '<(lua_library_type)',
      'conditions':
      [
        ["OS=='android'",
        {
          'ldflags': [
            '--sysroot=<(ndk_root)/platforms/android-9/arch-arm',
            '-gcc-toolchain', '<(ndk_root)/toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64',
            '-no-canonical-prefixes',
            '-target armv7-none-linux-androideabi',
            '-Wl,--fix-cortex-a8',
            '-Wl,--no-undefined',
            '-Wl,-z,noexecstack',
            '-Wl,-z,relro',
            '-Wl,-z,now',
            '-mthumb',
            '-lgcc',
          ],
        }],
      ],
    }
  ]


}
