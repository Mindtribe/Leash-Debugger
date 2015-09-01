static_library_project :CC3200_SDK_oslib, File.dirname(__FILE__) do |lib|

  lib.add_api_headers ['.']

  lib.add_configuration :SDK,
    sources: ['./osi_freertos.c'],
    toolchain: toolchain(:arm_none_eabi_gcc,
      cflags: ['-mthumb', '-mcpu=cortex-m4', '-ffunction-sections', '-fdata-sections', '-std=c99', '-g', '-O0', '-Dgcc']
    ), dependencies: [
      'FreeRTOS:Debug', 'CC3200_SDK_driverlib:SDK', 'CC3200_SDK_inc:SDK'
    ]

end

