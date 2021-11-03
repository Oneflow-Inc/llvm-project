file(GLOB DEMO_SRC generated/*.cpp)

add_library(of-user-op-def-demo ${DEMO_SRC})
target_include_directories(of-user-op-def-demo PRIVATE generated include)
target_include_directories(of-user-op-def-demo SYSTEM PRIVATE 
  ../inja/single_include 
  ../json/single_include
) # only for string_view
