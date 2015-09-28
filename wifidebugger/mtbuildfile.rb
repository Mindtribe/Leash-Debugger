application_project :wifidebugger, File.dirname(__FILE__) do |app|

  app.add_configuration :Debug,
    sources: ['src/**/*.c'],
    toolchain: toolchain(:arm_none_eabi_gcc,
      linker_script: 'linker.ld',
      cflags: ['-DUSE_FREERTOS'],
      include_paths: ['src', 'src/vendor', 'src/target', 'src/target/cc3200', 'src/common', 'src/interface', 'src/interface/jtag', 
        'src/gdbserver', 'src/thirdparty', 'src/thirdparty/freertos/include', 'src/thirdparty/freertos/portable/GCC/ARM_CM4',
        'src/interface/wifi', 'src/interface/uart']
    ),dependencies: [
      'CC3200_SDK_simplelink:SDK', 'CC3200_SDK_driverlib:SDK', 'FreeRTOS:Debug', 'CC3200_SDK_inc:SDK', 'CC3200_SDK_oslib:SDK'
    ]

end
