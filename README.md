# ZGameFrame
A cross platform game framework that implements (almost) everything from the pixel up. 
The framework uses threading (almost to an extreme) where every gui element (aka ZWin) has its own message queue and thread. Vast majority of the time the threads are asleep waiting on a condition variable, so the application uses very little CPU. I take advantage of OS thread scheduling efficiencies wherever possible.

The system determines visibility lists and in combination with dirty rect system, renders only portions of the screen that have changed.


Libraries are extensive so I still need to describe them and their uses here. <tbd>

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

