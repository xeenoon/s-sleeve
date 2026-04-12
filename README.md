## Inspiration

In Australia, many people live with chronic pain because rehabilitation is often delayed or too expensive to access consistently. Without regular feedback, recovery becomes guesswork.

We built Smart Sleeve to make rehabilitation more accessible by giving users real-time insight into how they move, not just how much they move.

## What it does

Smart Sleeve is a wearable knee rehabilitation device that tracks both movement and movement quality in real time.

Instead of only counting repetitions, it evaluates how well each step is performed. The system detects issues like:
- stalling mid-step
- uneven acceleration
- lack of control during descent
- stiff or inconsistent motion

These signals are converted into simple scores so users can understand their progress and improve technique without constant supervision.

## How we built it

Smart Sleeve is a tightly integrated hardware and software system.

### Hardware

We designed a custom two-pulley sensing mechanism using:
- a spring-loaded pulley for constant tension
- a secondary pulley coupled to a potentiometer
- a string attached to the user’s leg

This converts knee motion into continuous rotational data.

A major challenge was durability. Early prototypes failed under real movement forces, so we redesigned the system with:
- soldered connections
- heatshrink reinforcement
- strain relief loops

This made the device stable enough for repeated use.

### Embedded Processing

An ESP32 continuously reads the potentiometer and processes the signal into more than just angle data.

We built logic to analyse movement quality by tracking how motion changes over time. Instead of just detecting movement, the system evaluates:
- smoothness
- control
- consistency

These are combined into step-quality metrics and progress scores that users can act on.

### Custom Embedded Compiler

One of the most technical parts of the project was bringing a modern web interface onto a microcontroller.

Frameworks like Angular and Express are too heavy to run on an ESP32, so we built a custom compiler that:
- takes Angular-style components and EJS-style server code
- tokenizes and parses them in C
- compiles them into static C/C++ assets at build time

This required translating object-oriented JavaScript concepts into C:
- component state → runtime slots
- bindings → direct lookups
- data models → tagged C structures (objects, arrays, primitives)

The compiler also pre-renders server-side pages by evaluating route data and resolving templates ahead of time.

The result is a fully embedded web app served directly from the ESP32 without running a heavy framework at runtime.

### Toolchain Integration

We integrated the compiler the PlatformIO toolchain using a pre-build script, so every firmware build automatically recompiles the web interface.

To make development manageable, we built and tested the compiler on a desktop environment with unit tests before deploying to hardware. There are two compilers in the github, windows_

## Challenges

- Hardware durability: early designs failed under real movement forces  
- Embedded constraints: delivering a modern UI on limited hardware  
- System coupling: hardware reliability directly affected analytics accuracy  

We solved this by iterating on mechanical design and shifting complexity from runtime into the compiler.

## Accomplishments

- Built a full end-to-end rehabilitation system  
- Designed custom mechanical sensing hardware  
- Implemented real-time movement quality analysis  
- Developed a custom compiler for embedded web apps  
- Delivered a modern interface from an ESP32  

## What we learned

- Rehabilitation tech requires tight integration between hardware, signal processing, and user experience  
- Moving complexity into build-time systems enables richer embedded applications  
- Testing complex systems off-device is critical for fast iteration  

## What’s next

- Design a custom 3D-printed enclosure for better usability  
- Extend the system to other joints (shoulder, wrist, hip)  
- Improve motion analysis and feedback accuracy  

## Compiler instructions
  You too can you our custom minimal ejs and angular compiler!

  write that, give instructions for compiling for both windows and embedded


• ## Compiler instructions

  You can also use our custom minimal Angular and EJS compiler.

  The compiler is designed to let you write a lightweight web app using Angular-style components
  and Express/EJS-style pages, then compile that app into static frontend assets and embedded C/
  C++ source suitable for an ESP32.

  ### What it compiles

  The compiler reads:

  - app.component.ng
  - app.component.html
  - app.component.css
  - app.js
  - static route assets under routes/
  - Express/EJS-style pages under server/

  It then generates:

  - web_runtime_generated.h
  - web_runtime_generated.cpp
  - web_page_generated.cpp
  - compiled index.html
  - compiled styles.css
  - compiled app.js

  The runtime templates used for generation are:

  - embedded_compiler/assets/web_runtime_generated.h.tpl
  - embedded_compiler/assets/web_runtime_generated.cpp.tpl

  It also uses:

  - embedded_compiler/assets/ng_runtime.js

  ### Windows host build

  Use the Windows compiler first when you want fast iteration and easier debugging.

  From the repo root:

  cd embedded_compiler
  make test

  This builds the desktop compiler, runs the unit tests, and validates the generated output.

  If you want to run the compiler manually against an app:

  cd embedded_compiler
  make
  bin\main.exe ..\angular_app

  That compiles the app in ..\angular_app and emits generated assets and source files.

  Why use the Windows path first:

  - faster iteration
  - easier to inspect generated files
  - safer than debugging compiler changes on-device
  - unit tests catch parser/rendering/codegen regressions before flashing hardware

  ### Embedded build

  For the ESP32 build, the compiler is already attached to the PlatformIO toolchain.

  This is configured in platformio.ini with:

  extra_scripts = pre:embedded_compiler/platformio_prebuild.py

  That means every embedded build automatically runs the compiler first, then builds the
  firmware with the newly generated web assets.
