#pragma once

#include "entt/entt.hpp"
#include "sol/sol.hpp"
#include "../meta_helper.hpp"

template <typename Component>
auto num_components(entt::registry &registry) {
  return registry.size<Component>();
}
template <typename Component>
void emplace_component(entt::registry &registry, entt::entity entity,
                       const sol::table &instance) {
  const auto comp = instance.valid() ? instance.as<Component>() : Component{};
  registry.emplace_or_replace<Component>(entity, comp);
}
template <typename Component>
sol::object get_component(entt::registry &registry, entt::entity entity,
                          const sol::this_state &s) {
  auto &comp = registry.get_or_emplace<Component>(entity);
  return sol::make_object(s, std::ref(comp));
}
template <typename Component>
bool has_component(entt::registry &registry, entt::entity entity) {
  return registry.has<Component>(entity);
}
template <typename Component>
auto remove_component(entt::registry &registry, entt::entity entity) {
  return registry.remove_if_exists<Component>(entity);
}
template <typename Component>
void clear_component(entt::registry &registry) {
  registry.clear<Component>();
}

template <typename Component> void register_meta_component() {
  entt::meta<Component>().type();
  entt::meta<Component>()
    .template func<&num_components<Component>>("size"_hs)
    //.template func<&empty_components<Component>>("empty"_hs)
    .template func<&emplace_component<Component>>("emplace"_hs)
    .template func<&get_component<Component>>("get"_hs)
    .template func<&has_component<Component>>("has"_hs)
    .template func<&clear_component<Component>>("clear"_hs)
    .template func<&remove_component<Component>>("remove"_hs);
}

auto collect_types(const sol::variadic_args &va) {
  std::set<entt::id_type> types;
  std::transform(va.cbegin(), va.cend(), std::inserter(types, types.begin()),
                 [](const auto &obj) { return deduce_type(obj); });
  return types;
}

sol::table open_registry(const sol::this_state &s) {
  sol::state_view lua{ s };
  auto entt_module = lua["entt"].get_or_create<sol::table>();

  // clang-format off
  entt_module.new_usertype<entt::runtime_view>("runtime_view",
    sol::no_constructor,

    "size", &entt::runtime_view::size,
    "empty", &entt::runtime_view::empty,
    "contains", &entt::runtime_view::contains,
    "each",
      [](const sol::object &self, const sol::function &callback) {
        if (!callback.valid()) return;
        for (auto entity : self.as<entt::runtime_view>())
          callback(entity);
      }
  );

#define AS_REGISTRY(self) self.as<entt::registry>()
#define AS_REGISTRY_REF(self) std::ref(AS_REGISTRY(self))

  entt_module.new_usertype<entt::registry>("registry",
    sol::meta_function::construct,
    sol::factories([]{ return entt::registry{}; }),

    "size",
      sol::overload(
      &entt::registry::size,
        [](const sol::object &self, const sol::object &type_or_id) {
          auto maybe_any =
            invoke_meta_func(deduce_type(type_or_id), "size"_hs, AS_REGISTRY_REF(self));
          return maybe_any ? maybe_any.cast<size_t>() : 0;
        }
      ),
    "alive", &entt::registry::alive,
    "empty",
      sol::overload(
        &entt::registry::empty<>,
        [](const sol::object &self, entt::id_type type_id) {
          // @todo
          //invoke_meta_func(entt::resolve_type(id), "empty"_hs,)
        }
      ),
    "valid", &entt::registry::valid,
    "entity", &entt::registry::entity,
    "version", &entt::registry::version,
    "current", &entt::registry::current,

    "create",
      [](const sol::object &self) { return AS_REGISTRY(self).create(); },
    "destroy",
      [](const sol::object &self, entt::entity entity) {
        return AS_REGISTRY(self).destroy(entity);
      },

    "emplace",
      [](const sol::object &self, entt::entity entity, const sol::table &comp) {
        if (!comp.valid()) return;
          invoke_meta_func(get_type_id(comp), "emplace"_hs,
            AS_REGISTRY_REF(self), entity, comp);
        },
    "remove",
      [](const sol::object &self, entt::entity entity, const sol::object &type_or_id) {
        auto maybe_any =
          invoke_meta_func(deduce_type(type_or_id), "remove"_hs,
            AS_REGISTRY_REF(self), entity);
        
        return maybe_any ? maybe_any.cast<size_t>() : 0;
      },
    "has",
      [](const sol::object &self, entt::entity entity, const sol::object &type_or_id) {
        auto maybe_any =
          invoke_meta_func(deduce_type(type_or_id), "has"_hs,
            AS_REGISTRY_REF(self), entity);
        
        return maybe_any ? maybe_any.cast<bool>() : false;
      },
    "any",
      [](const sol::table &self, entt::entity entity, const sol::variadic_args &va) {
        const auto types = collect_types(va);
        const sol::function &has{ self["has"] };
        return std::any_of(types.begin(), types.end(),
          [&](auto type_id) { return has(self, entity, type_id).get<bool>(); }
        );
      },
    "get",
      [](const sol::object &self, entt::entity entity, const sol::object &type_or_id,
         const sol::this_state &s) {
      auto maybe_any =
        invoke_meta_func(deduce_type(type_or_id), "get"_hs,
          AS_REGISTRY_REF(self), entity, s);
      
      return maybe_any ? maybe_any.cast<sol::object>() : sol::nil_t{};
    },
    "clear",
      sol::overload(
        &entt::registry::clear<>,
        [](const sol::object &self, const sol::object &type_or_id) {
          invoke_meta_func(deduce_type(type_or_id), "clear"_hs, AS_REGISTRY_REF(self));
        }
      ),

    "orphan", &entt::registry::orphan,
    "orphans",
      [](const sol::object &self, const sol::function &callback) {
        if (!callback.valid()) return;
        AS_REGISTRY(self).orphans([&callback](auto entity) { callback(entity); });
      },

    "runtime_view",
      [](const sol::object &self, const sol::variadic_args &va) {
        const auto types = collect_types(va);
        return AS_REGISTRY(self).runtime_view(std::cbegin(types), std::cend(types));
      }
  );
  // clang-format on

#undef AS_REGISTRY
#undef AS_REGISTRY_REF

  return entt_module;
}