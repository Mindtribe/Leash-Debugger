framework_project :CC3200_SDK_inc, File.dirname(__FILE__) do |fwk|

  fwk.add_api_headers ['.', './..']
  fwk.add_configuration :SDK, objects: []

end

