static_library_project :CC3200_SDK_driverlib, File.dirname(__FILE__) do |lib|

  lib.add_api_headers ['.', './../inc', './..']

  lib.add_configuration :SDK,
    sources: ['./*.c'],
    toolchain: toolchain(:arm_none_eabi_gcc,
      cflags: ['-mthumb', '-mcpu=cortex-m4', '-ffunction-sections', '-fdata-sections', '-std=c99', '-g', '-O0', '-Dgcc']
    )

end

