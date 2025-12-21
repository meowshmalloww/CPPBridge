require 'json'

package = JSON.parse(File.read(File.join(__dir__, 'package.json')))

Pod::Spec.new do |s|
  s.name         = "react-native-cppbridge"
  s.version      = package['version']
  s.summary      = package['description']
  s.homepage     = package['repository']
  s.license      = package['license']
  s.authors      = { "CPPBridge Team" => "team@cppbridge.dev" }
  s.platforms    = { :ios => "11.0" }
  s.source       = { :git => "https://github.com/user/react-native-cppbridge.git", :tag => "v#{s.version}" }

  s.source_files = "ios/**/*.{h,m,mm}", "../hub/**/*.{h,cpp}"
  s.exclude_files = "../hub/modules/database/sqlite3.c"
  
  s.dependency "React-Core"
  
  # SQLite
  s.library = 'sqlite3'
  
  # C++ settings
  s.pod_target_xcconfig = {
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
    'HEADER_SEARCH_PATHS' => '$(inherited) "${PODS_ROOT}/../../../hub" "${PODS_ROOT}/../../../hub/core" "${PODS_ROOT}/../../../hub/modules"'
  }
end
