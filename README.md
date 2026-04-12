## Inspiration

In Australia, many people live with chronic pain because rehabilitation often does not happen early enough. By the time treatment becomes affordable or subsidised, the injury may already have progressed into a long-term condition. For many people, regular access to physical therapy is simply too expensive, which means they are left to recover alone and without feedback.

We built Smart Sleeve to help address that gap. Our goal was to create an affordable rehabilitation device that gives people meaningful, real-time guidance during recovery, so they can begin improving movement quality early rather than waiting until pain becomes chronic.

---

## What it does

Smart Sleeve is a wearable knee rehabilitation device that tracks both movement and movement quality in real time. Instead of only counting repetitions, it evaluates how well each step is performed.

The system measures knee movement, detects changes in control and consistency, and translates those signals into clear feedback.

It can identify issues such as:
- stalling mid-step  
- moving too stiffly  
- descending in an uncontrolled way  
- compensating during motion  

Those measurements are then summarized into easy-to-understand scores so users can monitor not just how much rehabilitation they are doing, but how effectively they are doing it.

The result is a tool that gives patients better visibility into their recovery and helps them take a more active role in rehabilitation without relying on constant in-person supervision.

---

## How we built it

Smart Sleeve combines a custom mechanical sensing system, embedded motion analysis, and a web application served directly from an ESP32.

From the beginning, the challenge was not just building a sensor, but building a complete system that could:
- physically survive real rehabilitation movement  
- interpret that movement meaningfully  
- present the results in a way that felt modern and easy to use  

### Hardware

We started with the hardware.

The core sensing mechanism is a two-pulley system made from:
- a spring-loaded pulley  
- a secondary pulley  
- a potentiometer  
- a custom mounting bracket  

We deliberately chose parts with similar dimensions early on so that machining and friction-fitting them together would be easier.

One pulley was modified to couple directly to the potentiometer, while the spring-loaded pulley kept constant tension on a string attached to the user’s leg. That constant tension allowed us to measure both movement and release, giving continuous rotational feedback as the knee bent and straightened.

That rotational signal was then fed into the ESP32 through the potentiometer.

However, the first real challenge was durability.

Human leg movement applies far more force than a simple electronics prototype is usually designed to handle. Our earliest wiring solutions failed quickly:
- jumper leads tore off  
- hot glue was pulled loose  
- mounting points shifted under repeated tension  

We had to repeatedly redesign the physical assembly so the electronics could survive real use.

Our final hardware revision used:
- soldered wires  
- heatshrink reinforcement  
- strain-relief loops secured with a zip tie  

This absorbed force before it reached the potentiometer pins.

That redesign turned the device from a fragile prototype into something stable enough for repeated rehabilitation testing.

---

### Firmware

Once we had stable input from the hardware, we focused on the firmware.

The ESP32 continuously:
- polls the potentiometer  
- processes the signal into more than raw angle data  

We built logic to track relative change over time so the system could evaluate the quality of each rehabilitation step.

Instead of asking “did the user move?”, we asked:
- did they stall halfway through?  
- did motion accelerate unevenly?  
- did it look stiff or unstable?  
- was the descent controlled?  

These measurements were combined into:
- step-quality metrics  
- progress scores  

---

### Frontend + Embedded Compiler

The next challenge was presenting that information in a way that felt intuitive and accessible.

We wanted the interface to:
- work from a phone  
- be hosted directly on the device  
- feel like a modern web application  

Frameworks like Angular and Express are too heavy for an ESP32.

Instead, we built a custom embedded compiler that:
- allows Angular-style and Express/EJS-style development  
- compiles everything into C/C++ assets  

The compiler:
- tokenizes source files  
- parses structure  
- reconstructs components, bindings, styles, and observables  

We then flatten object-oriented structures into C-compatible representations:
- component state → runtime slots  
- bindings → direct lookups  
- observables → poll/update instructions  

For server-side rendering:
- JavaScript models → tagged C structures  
- objects → key/value arrays  
- arrays → typed value lists  

The compiler:
- evaluates route data  
- resolves EJS templates  
- renders everything at build time  

Final output becomes static embedded assets in firmware.

---

### Build Integration

We integrated the compiler into PlatformIO using:

embedded_compiler/platformio_prebuild.py

Every build:
1. runs the compiler  
2. regenerates assets  
3. compiles firmware  

---

### Development Workflow

We validated everything on Windows before hardware.

We built unit tests for:
- tokenization  
- parsing  
- route generation  
- asset generation  
- rendering  

This made debugging far easier than working directly on embedded hardware.

---

## Challenges we ran into

The biggest challenge was system interdependence.

The hardware needed to be durable, but the software depended on clean data. Early mechanical failures meant unreliable analytics, forcing us to repeatedly revisit the physical design.

At the same time:
- the ESP32 limited runtime complexity  
- modern UI expectations conflicted with embedded constraints  

We solved this by shifting complexity into the compiler.

Development speed was also difficult:
- mechanical iteration is slow  
- embedded debugging is slow  

So we built workflows to test each layer independently.

---

## Accomplishments that we’re proud of

Smart Sleeve is a full end-to-end rehabilitation platform.

We:
- designed the sensing hardware  
- built embedded analytics  
- created the mobile dashboard  
- developed the compiler infrastructure  

We are especially proud of:
- tight hardware + software integration  
- delivering modern UI from embedded hardware  
- building a robust testing workflow  

---

## What we learned

We learned that rehabilitation tech requires a full feedback loop between:
- physical design  
- signal reliability  
- embedded processing  
- user experience  

We also learned the value of build-time optimization for embedded systems.

Moving complexity into the compiler enabled a much richer experience than the hardware alone could support.

---

## What’s next for Smart Sleeve

Next step:
- a custom 3D-printed sheath for better fit and comfort  

Long term:
- extend to other joints  
- apply the same motion + quality analysis to wrists, hips, shoulders, and more  
