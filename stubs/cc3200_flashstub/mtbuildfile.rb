application_project :cc3200_flashstub, File.dirname(__FILE__) do |app|

  app.add_configuration :Debug,
    sources: ['src/**/*.c'],
    toolchain: toolchain(:arm_none_eabi_gcc,
      linker_script: 'linker.ld',
      cflags: ['-DSIMPLELINK_NONOS_SINGLETHREAD_TINY', '-Os'],
      include_paths: ['src', 'src/vendor', 'include']
    ),dependencies: [
      'CC3200_SDK_simplelink:SDK_nonos_singlethread_tiny', 'CC3200_SDK_driverlib:SDK', 'CC3200_SDK_inc:SDK'
    ]

end
