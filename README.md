# EnTT meets Sol2

## Build examples
```bash
mkdir build && cd build
cmake ..
```

# Registry
[entt/wiki/registry](https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system#the-registry-the-entity-and-the-component)

## Goal
- Expose `registry` to Lua scripts:
    * Manage entity lifetime
    * Set components
    * Get component by value or reference
    * `runtime_view`

### c++ setup
```cpp
struct Transform { int x, y; };

register_meta_component<Transform>();

sol::state lua{};
lua.require("dispatcher" ...);

lua.new_usertype<Transform>("Transform",
    "type_id", &entt::type_info<Transform>::id,
    sol::call_constructor,
    sol::factories([](int x, int y) {
        return Transform{ x, y };
    }),
    "x", &Transform::x,
    "y", &Transform::y
);
```
```cpp
entt::registry registry{};
lua["registry"] = std::ref(registry);
```
### lua
```lua
registry = entt.registry.new()
```

```lua
mario = registry:create()
registry:emplace(mario, Transform(1, 2))
material = registry:get(mario, Material)

if (registry:has(mario, Transform)) then
    registry:emplace(mario, DeletionFlag())
end
```

```lua
-- Utilizes variadic args - pass as many types as you want
registry:runtime_view(Transform, DeletionFlag):each(
    function(entity)
        registry:remove(entity, DeletionFlag)
    end
)
```

# Event dispatcher

[entt/wiki/dispatcher](https://github.com/skypjack/entt/wiki/Crash-Course:-events,-signals-and-everything-in-between#event-dispatcher)

## Goal
- Either create new (inside script) or connect existing (native c++) dispatcher
- Trigger/enqueue events from Lua side
- Listen for events in Lua scripts
- Support disconnection of listeners

### c++ setup
```cpp
struct an_event { int value; };

register_meta_event<an_event>();

sol::state lua{}
lua.require("dispatcher", ...)

lua.new_usertype<an_event>("an_event",
    "type_id", &entt::type_info<an_event>::id,
    sol::call_constructor,
    sol::factories([](int value) {
        return an_event{ value };
    }),
    "value", &an_event::value
);
```
```cpp
entt::dispatcher dispatcher{};
lua["dispatcher"] = std::ref(dispatcher);

// load script ...

dispatcher.update(); // inside loop
```

### lua script
```lua
dispatcher = entt.dispatcher.new()
```

```lua
listener = {}
function listener.receive(evt)
    -- ...
end
function listener.method(evt)
    -- ...
end

-- You have to store result of 'connect()', otherwise listener will be
-- disconnected in lua garbage collection step.
conn = dispatcher:connect(an_event, listener.receive)
dispatcher:connect(another_event, listener.method)

-- Event type is deduced from bound 'type_id()' method
dispatcher:trigger(an_event(42))
dispatcher:enqueue(another_event())

conn = nil -- to disconnect listener
```

# Cooperative scheduler

[entt/wiki/cooperative-scheduler](https://github.com/skypjack/entt/wiki/Crash-Course:-cooperative-scheduler)

## Goal
- Define process class in Lua and attach it to `scheduler` either native c++ or dedicated to script
- Allow process chaining
### c++ setup

```cpp
sol::state lua{};
lua.require("scheduler", ...);
```
```cpp
entt::scheduler scheduler{};
lua["scheduler"] = std::ref(scheduler);

scheduler.update(dt); // inside loop
```

### lua script
```lua
scheduler = entt.scheduler.new()
```

```lua
local process_a = {}
function process:update(dt)
    -- ...
end
-- define init, succeeded and other methods ...

-- This one differs from entt attach().then().then ... but works the same
scheduler:attach(
    process_a,
    process_b,
    process_n
    -- etc ...
)
```

# License
Code released under [CC0 1.0 Universal](LICENSE)