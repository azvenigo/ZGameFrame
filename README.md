# ZGameFrame
A cross platform game framework that implements (almost) everything from the pixel up. 
The framework uses threading (almost to an extreme) where every gui element (aka ZWin) has its own message queue and thread. Vast majority of the time the threads are asleep waiting on a condition variable, so the application uses very little CPU. I take advantage of OS thread scheduling efficiencies wherever possible.

The system determines visibility lists and in combination with dirty rect system, renders only portions of the screen that have changed.

# Framework Design and Philosophy
Libraries are extensive so I still need to describe them and their uses here. At a high level the idea is not do computation that doesn't need to be done. If a window hasn't changed, don't spend time repainting it.
So rather than spend time doing micro optimization on getting some image processing sped up, determine if the processing needs to be done at all.

ZBuffer is a 2D surface class that can be clipped, Blt, rasterized, alpha blended. It provides an interface to raw uint32_t pixels in ARGB format. Because it is a software buffer, no stride restrictions are needed and buffers have a contiguous block of pixel data.

All UI is via a heirarchical window called ZWin (aka widget in some systems) elements. Each ZWin spawns its own thread and has its own message queue. Each window renders to its own ZBuffer when it is "invalid", i.e. needs painting. Each window has it's own Process function to perform any periodic logic. So paint and process are all called by that ZWin's thread.

The main thread loop handles the interface to the OS, taking input and delivering it to the appropriate internal queues.

High level singletons (via global function pointers with explicit initialization and shutdown to avoid any timing dependencies with lazy instantiation) include:
ZGraphicSystem - 
ZRasterizer
ZFontSystem
ZRegistry
ZMessageSystem
ZInput
ZAnimator
ZTickManager
ZWinDebugConsole






* Graphics 
  * 2D Windowing system
  * Color conversion and blending
  * font system
  * Rasterization
  * Transformations
  * Animation system
  * Styling
  * Thumbnail generation and cache
* Messaging
* Persistence
* Threading
* Compression
* Debugging, Logging, Timing


## Building does require ZLibraries to be at the same directory level as ZGameFrame. tbd: import them as a library

## Two projects here that use the framework:

### ZImageViewer
Fast as possible photo management tool to help with my photography.

### SandboxApps
Collection of mini applications, each in its own ZWin that I've used to experiment with various framework elements.

Move the mouse to the upper right corner to bring up the menu to select from the following:

Chess can load PGNs and step through to study the game. 
Font Tool lets me visualize the TrueType fonts available in the OS and export them into a format that can be loaded on platforms without a font system.
Image Processor is an experimental sandbox where I can play with various image techniques. One feature is focus stacking.
FloatLinesWin is just a pretty screensaver; something I've written on every platform I've ever programmed on.
3D TestWin is an experimental ray tracer.
Checkerboard is an experiment to push an OS's threading to the limit. It creates a checkerboard of windows of the above just to see how many threads I can get to run at once.

