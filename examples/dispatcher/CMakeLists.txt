add_executable(dispatcher "main.cpp" "bond.hpp")
target_link_libraries(dispatcher
	entt::entt_header_only
	sol2::sol2_header_only
)