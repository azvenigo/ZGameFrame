#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>
#include <utility>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <cmath>
#include <sstream>
#include <chrono>

#include <random>

#include "Z3DMath.h"
#include "teapotdata.h"


// not define on every system
#ifndef M_PI
#define M_PI 3.14159265358979323846  /* pi */
#endif


// [comment]
// In the main function of the program, we create the scene (create objects and lights)
// as well as set the options for the render (image widht and height, maximum recursion
// depth, field-of-view, etc.). We then call the render function().
// [/comment]
/*int main(int argc, char **argv)
{
    // loading gemetry
    std::vector<std::unique_ptr<Object>> objects;

    createPolyTeapot(Matrix44f(1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1), objects);
    //createCurveGeometry(objects);

    // lights
    std::vector<std::unique_ptr<Light>> lights;
    Options options;

    // aliasing example
    options.fov = 39.89;
    options.width = 512;
    options.height = 512;
    options.maxDepth = 1;

    // to render the teapot
    options.cameraToWorld = Matrix44f(0.897258, 0, -0.441506, 0, -0.288129, 0.757698, -0.585556, 0, 0.334528, 0.652606, 0.679851, 0, 5.439442, 11.080794, 10.381341, 1);
    
    // to render the curve as geometry
    //options.cameraToWorld = Matrix44f(0.707107, 0, -0.707107, 0, -0.369866, 0.85229, -0.369866, 0, 0.60266, 0.523069, 0.60266, 0, 2.634, 3.178036, 2.262122, 1);

    // finally, render
    render(options, objects, lights);

    return 0;
}*/
