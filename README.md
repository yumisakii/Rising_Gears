# Rising Gear

## Overview
A fast-paced first-person factory platformer game set in a Belle Époque theme, built from scratch in a strict one-month production cycle. The core experience is driven by a custom C++ grappling hook and momentum-based traversal system, challenging players to chain swings, dashes, and jumps to navigate complex industrial hazards in under 60 seconds.

<a href="https://vimeo.com/1188392499?fl=pl&fe=sh">
  <img src="https://github.com/user-attachments/assets/223d8dc2-1406-42d3-b882-bb5e4cd4038c" width="640" alt="Watch the video" />
</a>

## Project Context
- **Role:** Lead Developer & Core Gameplay Engineer
- **Engine / Tech Stack:** Unreal Engine / C++ & Blueprints
- **Team Size:** 12 (5 Developers, 7 Artists)
- **Development Time:** 1 Month
- **Context:** Third-year school project at G.Tech

## Core Systems & Architecture

### System A: Physics-Driven Traversal & Input Forgiveness (C++)
Engineered a high-speed platforming experience that feels heavy, responsive, and fair, avoiding the "floaty" nature of default character movement while chaining complex abilities under a 60-second timer.
- **Pendulum Math & Custom Gravity:** Engineered a custom grappling state that overrides movement to MOVE_Flying. It projects the player's velocity onto a 2D plane using FVector::VectorPlaneProject to calculate a true pendulum swing, while applying a custom gravity force (1500.0f) to simulate weight.
- **Coyote Time & Jump Buffering:** Implemented custom timing variables (TimeSinceLeftGround and JumpBufferTimeLeft) within the overridden Jump() and Landed() functions. This allows players to execute jumps slightly after walking off an edge or queue a jump just before hitting the ground, ensuring inputs are never unfairly dropped.
- **Dynamic Visual Feedback:** Utilized FMath::GetMappedRangeValueClamped to dynamically interpolate the First-Person Camera's Field of View based on the velocity vector size, amplifying the sensation of speed during dashes and swings.

### System B: Animation Blending & Cinematic Flow
Ensured that the high-speed C++ physics are accurately reflected in the character's visual state, and managed safe, lock-free transitions between UI, video playback, and gameplay.
- **State Machine Integration:** Architected the communication layer between the C++ character class and the Unreal Animation Blueprint (ABP). Hooked up movement variables (velocity, airborne state, grappling state) directly to the animation graph to trigger seamless visual transitions.
- **Media Framework & Event Flow:** Built a custom splash screen handling H.264 video playback. Solved critical race condition bugs by abandoning delays and instead binding the execution flow directly to the OnEndReached delegate. This ensured a clean, programmatic transfer of the player controller from "UI Only" to "Game Only" modes precisely when the cinematic finished.

### System C: Lead Developer & Pipeline Management
- **Build & Release Management:** Coordinated the technical execution of a 12-person team over a strict 1-month production cycle. Managed version control via GitHub Releases and handled the final project compilation, optimizing the build for a Full Rebuild "Shipping" package.

## The Team

**Tech (Development)**
- Jonathan de Vaulchier - Lead Developer
- Gabriel Funaro Tourbez - Generalist Programmer
- Salomon Cohen - UI Programmer
- Amélie Heinrich - Technical Artist
- Bastien N'Diaye - Audio Programmer

**Art**
- Emma Groski - Generalist Artist
- Johanna Goncalves - Environment Artist
- Ameli Mazure - Rigger/Animator
- Amandine - 2D Artist & UI/UX Artist
- Ambre Michel - 2D/3D Artist
- Mael Gourdet-Rossay - 3D Artist
