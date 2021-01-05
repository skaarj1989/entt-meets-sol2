add_executable(registry "main.cpp")
target_link_libraries(registry
	entt::entt_header_only
	sol2::sol2_header_only
)