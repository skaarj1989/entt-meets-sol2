add_library(MetaHelper INTERFACE "meta_helper.hpp")
target_include_directories(MetaHelper INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
set_property(TARGET MetaHelper PROPERTY FOLDER "Utility")
