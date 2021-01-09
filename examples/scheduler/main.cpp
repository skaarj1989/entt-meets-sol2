#include <thread>
#include <chrono>
#include <conio.h>
#include "entt/entt.hpp"
#include "sol/sol.hpp"

#define AUTO_ARG(x) decltype(x), x

using namespace std::chrono_literals;
using fsec = std::chrono::duration<float>;

template <typename Delta>
class ScriptProcess : public entt::process<ScriptProcess<Delta>, Delta> {
public:
  ScriptProcess(sol::table &&t, Delta frequency = 250ms)
      : m_self{ t }, m_update{ m_self["update"] }, m_frequency{ frequency } {
    m_self["succeed"] = [&] { succeed(); };
    m_self["abort"] = [&] { abort(); };
    m_self["fail"] = [&] { fail(); };
    m_self["pause"] = [&] { pause(); };
    m_self["unpause"] = [&] { unpause(); };

    m_self["alive"] = [&] { return alive(); };
    m_self["dead"] = [&] { return dead(); };
    m_self["paused"] = [&] { return paused(); };
    m_self["rejected"] = [&] { return rejected(); };
  }
  ~ScriptProcess() {
    std::cout << "ScriptProcess: " << m_self.pointer() << " terminated"
              << std::endl;
    m_self.clear();
    m_self.abandon();
  }

  void init() {
    std::cout << "ScriptProcess: " << m_self.pointer() << " joined"
              << std::endl;
    _call("init");
  }

  void update(Delta dt, void *) {
    if (!m_update.valid()) return fail();

    m_time += dt;
    if (m_time >= m_frequency) {
      m_update(m_self, dt.count());
      m_time = 0s;
    }
  }
  void succeeded() { _call("succeeded"); }
  void failed() { _call("failed"); }
  void aborted() { _call("aborted"); }

private:
  void _call(const std::string_view function_name) {
    if (auto &f = m_self[function_name]; f.valid()) f(m_self);
  }

private:
  sol::table m_self;
  sol::function m_update;
  Delta m_frequency, m_time{ 0 };
};

template <typename Delta> sol::table open_scheduler(const sol::this_state &s) {
  sol::state_view lua{ s };
  auto entt_module = lua["entt"].get_or_create<sol::table>();

  // clang-format off
  entt_module.new_usertype<entt::scheduler<Delta>>("scheduler",
    sol::meta_function::construct,
    sol::factories([]{ return entt::scheduler<Delta>{}; }),

    "size", &entt::scheduler<Delta>::size,
    "empty", &entt::scheduler<Delta>::empty,
    "clear", &entt::scheduler<Delta>::clear,
    "attach",
      [](entt::scheduler<Delta> &self, sol::table &&process,
         const sol::variadic_args &va) {
        // @todo validate process before attach?
        auto continuator =
          self.attach<ScriptProcess<Delta>>(std::move(process));
        for (sol::table &&child_process : va) {
          continuator =
            continuator.then<ScriptProcess<Delta>>(std::move(child_process));
        }
      },
    "update", &entt::scheduler<Delta>::update,
    "abort", &entt::scheduler<Delta>::abort
  );
  // clang-format on

  return entt_module;
}

int lua_custom_require(lua_State *L) {
  const auto path = sol::stack::get<std::string>(L, 1);
  if (path == "test_process") {
    const auto script = R"(
      local exports = {}

      local process = {
        name = '',
        count = 0,
        iterations = 5
      }

      function process:init()
        print('[lua] ' .. self.name .. ': init()')
      end
      function process:update(dt)
        print('[lua] ' .. self.name .. ': update('
          .. string.format("%.2f", dt) .. ')', self.count)
        self.count = self.count + 1
        if (self.count >= self.iterations) then
          self.succeed()
        end
      end
      function process:succeeded()
        print('[lua] ' .. self.name .. ': succeeded()')
      end
      function process:failed()
        print('[lua] ' .. self.name .. ': failed()')
      end
      function process:aborted()
        print('[lua] ' .. self.name .. ': aborted()')
      end

      function exports.new(name, iterations)
        local self = {}
        self.name = name or 'unknown'
        self.iterations = iterations or 5
        setmetatable(self, { __index = process })
        return self
      end

      return exports
    )";
    luaL_loadbuffer(L, script, strlen(script), path.c_str());
    return 1;
  }
  return 1;
}

//
//
//

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

    lua.clear_package_loaders();
    lua.add_package_loader(lua_custom_require);

    entt::scheduler<fsec> scheduler{};
    lua["scheduler"] = std::ref(scheduler);

    lua.script(R"(
      --local scheduler = entt.scheduler.new()
      
      local test_process = require('test_process')
      my_proc = test_process.new('deploy_missile')
      function my_proc:update(dt)
        self.fail()
      end

      scheduler:attach(
        test_process.new('open_silo'),
        my_proc,
        test_process.new('launch_missile') -- is rejected because 'my_proc' fails
      )

      assert(not scheduler:empty())
      assert(scheduler:size() == 1)
    )");

    using clock = std::chrono::high_resolution_clock;
    constexpr auto target_frame_time = 16ms;
    fsec delta_time{ target_frame_time };

    while (!scheduler.empty()) {
      auto beginTicks = clock::now();
      lua.step_gc(4);

      scheduler.update(delta_time);
      std::this_thread::sleep_for(target_frame_time);

      delta_time = std::chrono::duration_cast<fsec>(clock::now() - beginTicks);
      if (delta_time > 1s) delta_time = target_frame_time;

      if (_kbhit()) break;
    }
  } catch (const std::exception &e) {
    std::cout << "exception: " << e.what();
    return -1;
  }

  return 0;
}