#include <set>
#include <conio.h>
#include "entt/entt.hpp"
#include "sol/sol.hpp"

#define AUTO_ARG(x) decltype(x), x

entt::registry gRegistry{};

template <typename Component>
void emplace_component(entt::registry &registry, entt::entity entity,
                       const sol::object &component) {
  const auto comp =
    component.valid() ? component.as<Component>() : Component{};
  registry.emplace_or_replace<Component>(entity, comp);
}
template <typename Component>
sol::object get_component(entt::registry &registry, entt::entity entity,
                          const sol::this_state &s) {
  auto &comp = registry.get_or_emplace<Component>(entity);
  return sol::make_object(s, comp); // or use std::ref(comp) to give more permission
}
template <typename Component>
bool has_component(entt::registry &registry, entt::entity entity) {
  return registry.has<Component>(entity);
}
template <typename Component>
void remove_component(entt::registry &registry, entt::entity entity) {
  registry.remove_if_exists<Component>(entity);
}

template <typename Component> void register_meta_component() {
  entt::meta<Component>().type();
  entt::meta<Component>()
    .template func<&emplace_component<Component>>("emplace"_hs)
    .template func<&get_component<Component>>("get"_hs)
    .template func<&has_component<Component>>("has"_hs)
    .template func<&remove_component<Component>>("remove"_hs);
}

sol::table open_registry(const sol::this_state &s) {
  sol::state_view lua{ s };
  auto registry_module = lua.create_table();

  registry_module.set_function("create", [&] { return gRegistry.create(); });
  registry_module.set_function(
    "destroy", [&](entt::entity entity) { return gRegistry.destroy(entity); });

  registry_module.set_function("emplace", [&](entt::entity entity, entt::id_type id,
                                          const sol::object &component) {
    if (auto component_type = entt::resolve_type(id); component_type) {
      component_type.func("emplace"_hs).invoke({}, std::ref(gRegistry), entity,
                                           component);
    }
  });
  registry_module.set_function("get", [&](entt::entity entity, entt::id_type id,
                                          const sol::this_state &s) {
    if (auto component_type = entt::resolve_type(id); component_type) {
      auto any = component_type.func("get"_hs).invoke({}, std::ref(gRegistry),
                                                      entity, s);
      return any.cast<sol::object>();
    }
    return sol::make_object(s, sol::lua_nil);
  });
  registry_module.set_function(
    "has", [](entt::entity entity, entt::id_type id) {
      if (auto component_type = entt::resolve_type(id); component_type) {
        auto any =
          component_type.func("has"_hs).invoke({}, std::ref(gRegistry), entity);
        return any.cast<bool>();
      }
      return false;
    });
  registry_module.set_function("remove", [&](entt::entity entity,
                                             entt::id_type id) {
    if (auto component_type = entt::resolve_type(id); component_type)
      component_type.func("remove"_hs).invoke({}, std::ref(gRegistry), entity);
  });

  registry_module.set_function("size", [&] { return gRegistry.size(); });
  registry_module.set_function("empty", [&] { return gRegistry.empty(); });

  // clang-format off
  registry_module.new_usertype<entt::runtime_view>("runtime_view",
    sol::no_constructor,
    "size", &entt::runtime_view::size,
    "empty", &entt::runtime_view::empty,
    "contains", &entt::runtime_view::contains,
    "each", [&](const sol::object &self, const sol::function &callback) {
      for (auto entity : self.as<entt::runtime_view>())
        callback(entity);
    }
  );
  // clang-format on

  registry_module.set_function("view", [&](entt::id_type type,
                                           const sol::variadic_args &va,
                                           const sol::this_state &s) {
    std::set<entt::id_type> types{ type };
    std::transform(va.begin(), va.end(), std::inserter(types, types.begin()),
                   [](const auto &p) { return p.as<entt::id_type>(); });
    return gRegistry.runtime_view(std::cbegin(types), std::cend(types));
  });

  return registry_module;
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
    lua.require("registry", sol::c_call<AUTO_ARG(&open_registry)>, false);

    struct Transform {
      int x, y;
    };

    register_meta_component<Transform>();

    // clang-format off
    lua.new_usertype<Transform>("Transform",
      "type_id", &entt::type_info<Transform>::id,
      sol::call_constructor,
      sol::factories([](int x, int y) {
        return Transform{ x, y };
      }),
      "x", &Transform::x,
      "y", &Transform::y
    );
    // clang-format on

    lua.script(R"(
      registry = require('registry')

      bowser = registry.create()
      assert(bowser == 0 and registry.size() == 1)
      registry.emplace(bowser, Transform.type_id(), Transform(5, 6))
      assert(registry.has(bowser, Transform.type_id()))

      transform = registry.get(bowser, Transform.type_id())
      print('Bowser position = ' .. transform.x .. ', ' .. transform.y)
    )");

    entt::entity bowser{ lua["bowser"] };
    auto t = gRegistry.try_get<Transform>(bowser);
    assert(t);
    Transform transform = lua["transform"];
    assert(t->x == transform.x && t->y == transform.y);

    lua.script(R"(
      view = registry.view(Transform.type_id())
      assert(not view:empty())

      local koopa = registry.create()
      registry.emplace(koopa, Transform.type_id()) -- default constructor (0, 0)
      transform = registry.get(koopa, Transform.type_id())
      print('Koopa position = ' .. transform.x .. ', ' .. transform.y)
      
      assert(view:size() == 2)

      view:each(function(entity)
        registry.remove(entity, Transform.type_id())
      end)

      assert(view:size() == 0)
    )");
  } catch (const std::exception &e) {
    std::cout << "exception: " << e.what();
    return -1;
  }

  return 0;
}