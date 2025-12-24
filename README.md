# Physics Engine

A **from-scratch 2D physics engine** written in **modern C++**, focused on correctness, performance, and learning-by-building.  
No external physics libraries. No shortcuts. Just raw math, data structures, and simulation logic.

This project is both:
- a **learning-driven deep dive** into physics simulation internals, and  
- a **foundational engine** that can be extended into games, simulations, or research tools.

---

##  Features

- Rigid body dynamics
- Gravity & external forces
- Collision detection
  - AABB
  - Circle–Circle
  - Circle–AABB
- Collision resolution
  - Impulse-based solver
  - Restitution (elasticity)
  - Basic friction
- Fixed timestep simulation
- Sub-stepping for stability
- Deterministic simulation (same input → same output)
- Minimal dependencies

---

##  Engine Philosophy

This engine is built with the following principles:

- **Ground-up implementation** — every system is written manually
- **Readable over clever** — clarity beats premature optimization
- **Data-oriented thinking** — cache-friendly where possible
- **Determinism first** — critical for debugging & reproducibility
- **No black boxes** — all math and algorithms are explicit

The goal is to *understand physics engines by building one*, not just to use one.

---
##  Tech Stack

- *Language:* C++17+
- *Build System:* CMake
- *Rendering (optional / demo only):* SDL2
- *Math:* Custom vector & geometry math (no glm)

---
