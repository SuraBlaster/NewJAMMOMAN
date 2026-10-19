# Naming conventions

- Types, functions, and enum values use `PascalCase`: `Player`, `UpdateVelocity`, `WallSlide`.
- Variables, members, and parameters use `camelCase`: `moveSpeed`, `elapsedTime`, `isGround`.
- Fixed constants use `UPPER_SNAKE_CASE`: `DASH_DURATION`, `SWORD_COLLISION_RADIUS`.
- Runtime values remain `camelCase` even when declared `const`.
- Use the same parameter names in declarations and definitions.
- Keep names required by external APIs, C++ conventions, and shader entry points: `serialize`, `main`, `WinMain`, standard-library methods, and external-library members.
- Preserve JSON keys, asset names, shader semantics, and serialized field names. When renaming a serialized member, explicitly preserve its stored key.
- Apply the same conventions to generated C++ code and update its generator together with the output.

Source files retain their existing encodings and line endings. Vendored code under `External` follows its upstream conventions.
