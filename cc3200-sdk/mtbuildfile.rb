static_library_project :CC3200_SDK, File.dirname(__FILE__) do |lib|

  lib.add_api_headers 'driverlib'
  lib.add_api_headers 'inc'

  lib.add_configuration :SDK,
    sources: ['driverlib/*.c'],
    toolchain: toolchain(:arm_none_eabi_gcc,
      cflags: ['-mthumb', '-mcpu=cortex-m4', '-fdata-sections', '-std=gnu99', '-g', '-O0']
    )

end

