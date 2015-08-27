workspace :wifidebugger, File.dirname(__FILE__) do |w|

	w.set_configuration_defaults :Debug,
    		toolchain: toolchain(:arm_none_eabi_gcc,
      			cppflags: ['-Dgcc'],
			cflags: ['-mthumb', '-mcpu=cortex-m4', '-fdata-sections', '-std=gnu99', '-g', '-O0', '-Wall', '-Werror', '-Wextra', '-pedantic-errors'],
      			ldflags:['-Wl,--entry,ResetISR', '-Wl,--gc-sections']
    		)

	w.add_project('cc3200-sdk')
	w.add_project('testapp')
end

