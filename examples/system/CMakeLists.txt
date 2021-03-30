add_executable(system "main.cpp")
target_link_libraries(system
	entt::entt_header_only
	sol2::sol2
)