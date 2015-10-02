static_library_project :CC3200_SDK_simplelink, File.dirname(__FILE__) do |lib|

  lib.add_api_headers ['.', './include']

  lib.add_configuration :SDK_freertos_multithread_full,
    sources: ['./**/*.c'],
    toolchain: toolchain(:arm_none_eabi_gcc,
      cflags: ['-mthumb', '-mcpu=cortex-m4', '-ffunction-sections', '-fdata-sections', '-std=c99', '-g', '-O0', '-Dgcc', '-DSIMPLELINK_FREERTOS_MULTITHREAD_FULL']
    ), dependencies: [
      'CC3200_SDK_oslib:SDK', 'CC3200_SDK_driverlib:SDK', 'CC3200_SDK_middleware:SDK', 'CC3200_SDK_inc:SDK'
    ]
    
  lib.add_configuration :SDK_nonos_singlethread_tiny,
    sources: ['./**/*.c'],
    toolchain: toolchain(:arm_none_eabi_gcc,
      cflags: ['-mthumb', '-mcpu=cortex-m4', '-ffunction-sections', '-fdata-sections', '-std=c99', '-g', '-O0', '-Dgcc', '-DSIMPLELINK_NONOS_SINGLETHREAD_TINY']
    ), dependencies: [
      'CC3200_SDK_oslib:SDK', 'CC3200_SDK_driverlib:SDK', 'CC3200_SDK_middleware:SDK', 'CC3200_SDK_inc:SDK'
    ]

end

