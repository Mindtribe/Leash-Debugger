static_library_project :FreeRTOS, File.dirname(__FILE__) do |lib|

  lib.add_api_headers ['.', './include', './portable/GCC/ARM_CM4']

  lib.add_configuration :Debug,
    sources: ['./**/*.c'],
    toolchain: toolchain(:arm_none_eabi_gcc)

end

