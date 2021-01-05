add_executable(scheduler "main.cpp")
target_link_libraries(scheduler
	entt::entt_header_only
	sol2::sol2_header_only
)