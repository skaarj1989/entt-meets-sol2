#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <conio.h>

#include "script_process.hpp"
#include "entt/process/scheduler.hpp"

#define AUTO_ARG(x) decltype(x), x

using namespace std::literals;
using namespace std::chrono_literals;

using fsec = std::chrono::duration<float>;

namespace {

template <typename Delta>
[[nodiscard]] sol::table open_scheduler(sol::this_state s) {
  // To create a scheduler inside a script: entt.scheduler.new()

  sol::state_view lua{s};
  auto entt_module = lua["entt"].get_or_create<sol::table>();

  // clang-format off
  entt_module.new_usertype<entt::scheduler<Delta>>("scheduler",
    sol::meta_function::construct,
    sol::factories([]{ return entt::scheduler<Delta>{}; }),

    "size", &entt::scheduler<Delta>::size,
    "empty", &entt::scheduler<Delta>::empty,
    "clear", &entt::scheduler<Delta>::clear,
    "attach",
      [](entt::scheduler<Delta> &self, const sol::table &process,
         const sol::variadic_args &va) {
        // TODO: validate process before attach?
        auto continuator =
          self.attach<script_process<Delta>>(process);
        for (sol::table child_process : va) {
          continuator =
            continuator.then<script_process<Delta>>(std::move(child_process));
        }
      },
    "update", &entt::scheduler<Delta>::update,
    "abort", &entt::scheduler<Delta>::abort
  );
  // clang-format on

  return entt_module;
}

#if 0
// Few ways to require a module from a script:
// 1. extend path
// 2. lua.require_file("test_process", "lua/test_process.lua");
// 3. lua.add_package_loader(lua_custom_require);

void extend_path(sol::state &lua) {
  auto path = lua["package"]["path"].get<std::string>();
  auto scripts_path = (std::filesystem::current_path() / "lua").string();
  if (path.find(scripts_path) == std::string::npos)
    lua["package"]["path"] = scripts_path + "\\?.lua;" + path;
}

[[nodiscard]] int lua_custom_require(lua_State *L) {
  if (const auto path = sol::stack::get<std::string>(L, 1);
      path == "test_process") {
    const auto filename = "lua/test_process.lua"s;
    std::ifstream f{filename};
    if (!f.is_open())
      throw std::runtime_error{"Failed to open file: " + filename};

    std::stringstream ss;
    ss << f.rdbuf();
    const auto script = ss.str();

    luaL_loadbuffer(L, script.c_str(), script.length(), path.c_str());
    return 1;
  }
  return 1;
}
#endif

} // namespace

int main(int argc, char *argv[]) {
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif

  try {
    sol::state lua{};
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string);
    lua.require("scheduler", sol::c_call<AUTO_ARG(&open_scheduler<fsec>)>,
                false);

    entt::scheduler<fsec> scheduler{};
    lua["scheduler"] =
      std::ref(scheduler); // Make the scheduler available to Lua

    lua.do_file("lua/process_chain.lua");

    constexpr auto target_frame_time = 16ms;
    fsec delta_time{target_frame_time};

    while (!scheduler.empty()) {
      using clock = std::chrono::high_resolution_clock;
      const auto begin_ticks = clock::now();

      lua.step_gc(4);

      scheduler.update(delta_time);
      std::this_thread::sleep_for(target_frame_time);

      delta_time = std::chrono::duration_cast<fsec>(clock::now() - begin_ticks);
      if (delta_time > 1s) delta_time = target_frame_time;

      if (_kbhit()) break;
    }
  } catch (const std::exception &e) {
    std::cout << "exception: " << e.what();
    return -1;
  }

  return 0;
}
