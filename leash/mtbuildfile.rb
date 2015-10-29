application_project :leash, File.dirname(__FILE__) do |app|

  app.add_configuration :Launchpad,
    sources: ['src/**/*.c', 'src/**/*.S'],
    toolchain: toolchain(:arm_none_eabi_gcc,
      linker_script: 'linker.ld',
      cflags: ['-DUSE_FREERTOS', '-DSIMPLELINK_FREERTOS_MULTITHREAD_FULL', '-DBOARD_LAUNCHPAD'],
      include_paths: ['src', 'src/vendor', 'src/target', 'src/target/cc3200', 'src/common', 'src/interface', 'src/interface/jtag', 
        'src/gdbserver', 'src/thirdparty', 'src/interface/wifi', 'src/interface/uart', 'src/interface/gpio']
    ),dependencies: [
      'CC3200_SDK_simplelink:SDK_freertos_multithread_full', 'CC3200_SDK_driverlib:SDK', 'FreeRTOS:Debug', 'CC3200_SDK_inc:SDK', 'CC3200_SDK_oslib:SDK', 'cc3200_flashstub_inc:Protocol', 'cc3200_flashstub:Debug'
    ]
    
  app.add_configuration :RBL_WifiMini,
    sources: ['src/**/*.c', 'src/**/*.S'],
    toolchain: toolchain(:arm_none_eabi_gcc,
      linker_script: 'linker.ld',
      cflags: ['-DUSE_FREERTOS', '-DSIMPLELINK_FREERTOS_MULTITHREAD_FULL', '-DBOARD_RBL_WIFIMINI'],
      include_paths: ['src', 'src/vendor', 'src/target', 'src/target/cc3200', 'src/common', 'src/interface', 'src/interface/jtag', 
        'src/gdbserver', 'src/thirdparty', 'src/interface/wifi', 'src/interface/uart', 'src/interface/gpio']
    ),dependencies: [
      'CC3200_SDK_simplelink:SDK_freertos_multithread_full', 'CC3200_SDK_driverlib:SDK', 'FreeRTOS:Debug', 'CC3200_SDK_inc:SDK', 'CC3200_SDK_oslib:SDK', 'cc3200_flashstub_inc:Protocol', 'cc3200_flashstub:Debug'
    ]

end
