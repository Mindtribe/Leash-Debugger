application_project :wifidebugger, File.dirname(__FILE__) do |app|

  app.add_configuration :Debug,
    sources: ['src/**/*.c'],
    toolchain: toolchain(:arm_none_eabi_gcc,
      linker_script: 'linker.ld',
      include_paths: ['src', 'src/vendor', 'src/target', 'src/target/cc3200', 'src/common', 'src/interface', 'src/interface/jtag', 'src/gdbserver', 'src/thirdparty']
    ),dependencies: [
      'CC3200_SDK:SDK'
    ]

end
