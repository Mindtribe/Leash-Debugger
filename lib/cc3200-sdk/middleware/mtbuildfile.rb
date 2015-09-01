static_library_project :CC3200_SDK_middleware, File.dirname(__FILE__) do |lib|

  lib.add_api_headers ['.', './soc', './framework/pm', './framework/timer', './driver', './driver/hal']

  lib.add_configuration :SDK,
    sources: ['./**/*.c'],
    toolchain: toolchain(:arm_none_eabi_gcc,
      cflags: ['-mthumb', '-mcpu=cortex-m4', '-ffunction-sections', '-fdata-sections', '-std=c99', '-g', '-O0', '-Dgcc']
    ),dependencies: [
      'CC3200_SDK_driverlib:SDK', 'CC3200_SDK_oslib:SDK', 'CC3200_SDK_inc:SDK'
    ]

end

