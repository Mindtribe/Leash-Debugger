application_project :testapp, File.dirname(__FILE__) do |app|

  app.add_configuration :Debug,
    sources: ['src/**/*.c'],
    toolchain: toolchain(:arm_none_eabi_gcc,
      linker_script: 'linker.ld'
    ),dependencies: [
      'CC3200_SDK:SDK'
    ]

end
