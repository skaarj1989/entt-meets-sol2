add_executable(registry "main.cpp" "bond.hpp")
target_link_libraries(registry
	entt::entt_header_only
	sol2::sol2
)