#include <thread>
#include <chrono>
#include <conio.h>
#include "../registry/bond.hpp"
#include "../common/transform.hpp"

#define AUTO_ARG(x) decltype(x), x

using namespace std::chrono_literals;
using fsec = std::chrono::duration<float>;

struct ScriptComponent {
  sol::table self;
  struct {
    sol::function update;
  } hooks;
};

void init_script(entt::registry &registry, entt::entity entity) {
  auto &script = registry.get<ScriptComponent>(entity);
  assert(script.self.valid());
  script.hooks.update = script.self["update"];
  assert(script.hooks.update.valid());

  script.self["id"] = entity;
  script.self["owner"] = std::ref(registry);
  if (auto &f = script.self["init"]; f.valid()) f(script.self);
}

void release_script(entt::registry &registry, entt::entity entity) {
  auto &script = registry.get<ScriptComponent>(entity);
  if (auto &f = script.self["destroy"]; f.valid()) f(script.self);
  script.self.abandon();
}

void script_system_update(entt::registry &registry, fsec delta_time) {
  auto view = registry.view<ScriptComponent>();
  for (auto entity : view) {
    auto &script = view.get<ScriptComponent>(entity);
    assert(script.self.valid());
    script.hooks.update(script.self, delta_time);
  }
}

int main(int argc, char* argv[]) {
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif

  try {
    sol::state lua{};
    lua.open_libraries(sol::lib::base, sol::lib::package);
    lua.require("registry", sol::c_call<AUTO_ARG(&open_registry)>, false);
    register_transform(lua);
    register_meta_component<Transform>();

    entt::registry registry{};
    registry.on_construct<ScriptComponent>().connect<&init_script>();
    registry.on_destroy<ScriptComponent>().connect<&release_script>();

    auto behaviorScript = lua.load(R"(
      node = {}
      function node:init()
        print('node [#' .. self.id .. '] init()', self)
      end
      function node:update(dt)
        local transform = self.owner:get(self.id, Transform)
        transform.x = transform.x + 1
        print('node [#' .. self.id .. '] update()', transform)
      end
      function node:destroy()
        print('bye, bye! from: node #' .. self.id)
      end

      return node
    )");
    assert(behaviorScript.valid());
    for (int i = 0; i < 5; ++i) {
      auto e = registry.create();
      registry.emplace<Transform>(e, Transform{ i, i });
      registry.emplace<ScriptComponent>(e, behaviorScript.call());
    }

    using clock = std::chrono::high_resolution_clock;
    constexpr auto target_frame_time = 500ms;
    fsec delta_time{ target_frame_time };

    while (true) {
      auto beginTicks = clock::now();
      script_system_update(registry, delta_time);
      std::this_thread::sleep_for(target_frame_time);
      delta_time = std::chrono::duration_cast<fsec>(clock::now() - beginTicks);
      if (delta_time > 1s) delta_time = target_frame_time;

      if (_kbhit()) break;
    }

    registry.clear();

  } catch (const std::exception &e) {
    std::cout << "exception: " << e.what();
    return -1;
  }

  return 0;
}