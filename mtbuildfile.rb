application_project :WirelessDebugger, File.dirname(__FILE__) do |app|

  app.add_configuration :Debug,
    sources: ['src/*.c'],
    toolchain: toolchain(:arm_none_eabi_gcc,
      cppflags: "-Dgcc",
      cflags: '-std=c99 -mcpu=cortex-m4 -mthumb -mlittle-endian -mfpu=fpv4-sp-d16 -mfloat-abi=hard -ffunction-sections -fdata-sections -Wall -Werror -Wextra -pedantic-errors',
      cxxflags: '-std=c++03 -mcpu=cortex-m4 -mthumb -mlittle-endian -mfpu=fpv4-sp-d16 -mfloat-abi=hard -ffunction-sections -fdata-sections -Wall -Werror -Wextra -pedantic-errors',
      ldflags: '-Wl,--entry,ResetISR -Wl,--gc-sections',
      linker_script: 'src/blinky.ld'
    )

    app.add_default_tasks('WirelessDebugger:Debug')

end
