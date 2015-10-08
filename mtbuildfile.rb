workspace :leash, File.dirname(__FILE__) do |w|

	w.set_configuration_defaults :Debug,
    		toolchain: toolchain(:arm_none_eabi_gcc,
			cflags: ['-mthumb', '-mcpu=cortex-m4', '-fdata-sections', '-ffunction-sections', '-std=gnu99', '-Dgcc', '-g', '-O0', '-Wall', '-Werror', '-Wextra', '-pedantic-errors'],
      			ldflags:['-Wl,--entry,ResetISR', '-Wl,--gc-sections']
    		)

	w.add_project('lib/freertos')
	w.add_project('lib/cc3200-sdk/inc')
	w.add_project('lib/cc3200-sdk/driverlib')
	w.add_project('lib/cc3200-sdk/oslib')
	w.add_project('lib/cc3200-sdk/middleware')
	w.add_project('lib/cc3200-sdk/simplelink')
	w.add_project('testapp')
	w.add_project('leash')

	w.add_default_tasks(['testapp:Debug', 'leash:Debug'])
end

