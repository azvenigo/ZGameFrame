#include "3DTestWin.h"
#include "3DTestWin.h"
#include "ZRandom.h"
#include "ZFont.h"
#include "ZRasterizer.h"
#include "ZTypes.h"
#include "Z3DMath.h"
#include <cmath> 
#include <limits>
#include <algorithm>
#include "helpers/ThreadPool.h"
#include "ZWinControlPanel.h"
#include "Resources.h"
#include "helpers/Registry.h"
#include "ZWinBtn.H"
#include "ZInput.h"
#include "ZD3D.h"

using namespace std;
using namespace Z3D;




#ifdef RENDER_TEAPOT
#include "teapotdata.h"
#endif


inline
double clamp(const double& lo, const double& hi, const double& v)
{
    return std::max<double>(lo, std::min<double>(hi, v));
}


#ifdef RENDER_TEAPOT

static const double kInfinity = FLT_MAX;
static const double kEpsilon = 1e-8;
static const Vec3d kDefaultBackgroundColor = Vec3d(0.235294, 0.67451, 0.843137);
template <> const Matrix44d Matrix44d::kIdentity = Matrix44d();

inline
double deg2rad(const double& deg)
{
    return deg * M_PI / 180;
}

inline
Vec3d mix(const Vec3d& a, const Vec3d& b, const double& mixValue)
{
    return a * (1 - mixValue) + b * mixValue;
}

struct Options
{
    uint32_t width = 640;
    uint32_t height = 480;
    double fov = 90;
    Vec3d backgroundColor = kDefaultBackgroundColor;
    Matrix44d cameraToWorld;
    double bias = 0.0001;
    uint32_t maxDepth = 2;
};

enum MaterialType { kDiffuse, kNothing };

class Object
{
public:
    // [comment]
    // Setting up the object-to-world and world-to-object matrix
    // [/comment]
    Object(const Matrix44d& o2w) : objectToWorld(o2w), worldToObject(o2w.inverse()) {}
    virtual ~Object() {}
    virtual bool intersect(const Vec3d&, const Vec3d&, double&, uint32_t&, Vec2f&) const = 0;
    virtual void getSurfaceProperties(const Vec3d&, const Vec3d&, const uint32_t&, const Vec2f&, Vec3d&, Vec2f&) const = 0;
    virtual void displayInfo() const = 0;
    Matrix44d objectToWorld, worldToObject;
    MaterialType type = kDiffuse;
    Vec3d albedo = 0.18;
    double Kd = 0.8; // phong model diffuse weight
    double Ks = 0.2; // phong model specular weight
    double n = 10;   // phong specular exponent
    Vec3d BBox[2] = { kInfinity, -kInfinity };
};

bool rayTriangleIntersect(
    const Vec3d& orig, const Vec3d& dir,
    const Vec3d& v0, const Vec3d& v1, const Vec3d& v2,
    double& t, double& u, double& v)
{
    Vec3d v0v1 = v1 - v0;
    Vec3d v0v2 = v2 - v0;
    Vec3d pvec = dir.crossProduct(v0v2);
    double det = v0v1.dotProduct(pvec);

    // ray and triangle are parallel if det is close to 0
    if (fabs(det) < kEpsilon) return false;

    double invDet = 1 / det;

    Vec3d tvec = orig - v0;
    u = tvec.dotProduct(pvec) * invDet;
    if (u < 0 || u > 1) return false;

    Vec3d qvec = tvec.crossProduct(v0v1);
    v = dir.dotProduct(qvec) * invDet;
    if (v < 0 || u + v > 1) return false;

    t = v0v2.dotProduct(qvec) * invDet;

    return (t > 0) ? true : false;
}

class TriangleMesh : public Object
{
public:
    // Build a triangle mesh from a face index array and a vertex index array
    TriangleMesh(
        const Matrix44d& o2w,
        uint32_t nfaces,
        std::vector<uint32_t>& faceIndex,
        std::vector<uint32_t>& vertsIndex,
        std::vector<Vec3d>& verts,
        std::vector<Vec3d>& normals,
        std::vector<Vec2f>& st,
        bool singleVertAttr = true) :
        Object(o2w),
        numTris(0),
        isSingleVertAttr(singleVertAttr)
    {
        uint32_t k = 0, maxVertIndex = 0;
        // find out how many triangles we need to create for this mesh
        for (uint32_t i = 0; i < nfaces; ++i) {
            numTris += faceIndex[i] - 2;
            for (uint32_t j = 0; j < faceIndex[i]; ++j)
                if (vertsIndex[k + j] > maxVertIndex)
                    maxVertIndex = vertsIndex[k + j];
            k += faceIndex[i];
        }
        maxVertIndex += 1;

        // allocate memory to store the position of the mesh vertices
        P.resize(maxVertIndex);
        for (uint32_t i = 0; i < maxVertIndex; ++i) {
            objectToWorld.multVecMatrix(verts[i], P[i]);
        }

        // allocate memory to store triangle indices
        trisIndex.resize(numTris * 3);
        Matrix44d transformNormals = worldToObject.transpose();
        // [comment]
        // Sometimes we have 1 vertex attribute per vertex per face. So for example of you have 2
        // quads this would be defefined by 6 vertices but 2 * 4 vertex attribute values for
        // each vertex attribute (normal, tex. coordinates, etc.). But in some cases you may
        // want to have 1 single value per vertex. So in the quad example this would be 6 vertices
        // and 6 vertex attributes values per attribute. We need to provide both option to users.
        // [/comment]
        if (isSingleVertAttr) {
            N.resize(maxVertIndex);
            texCoordinates.resize(maxVertIndex);
            for (uint32_t i = 0; i < maxVertIndex; ++i) {
                texCoordinates[i] = st[i];
                transformNormals.multDirMatrix(normals[i], N[i]);
            }
        }
        else {
            N.resize(numTris * 3);
            texCoordinates.resize(numTris * 3);
            for (uint32_t i = 0, k = 0, l = 0; i < nfaces; ++i) { // for each  face
                for (uint32_t j = 0; j < faceIndex[i] - 2; ++j) {
                    transformNormals.multDirMatrix(normals[k], N[l]);
                    transformNormals.multDirMatrix(normals[k + j + 1], N[l + 1]);
                    transformNormals.multDirMatrix(normals[k + j + 2], N[l + 2]);
                    N[l].normalize();
                    N[l + 1].normalize();
                    N[l + 2].normalize();
                    texCoordinates[l] = st[k];
                    texCoordinates[l + 1] = st[k + j + 1];
                    texCoordinates[l + 2] = st[k + j + 2];
                }
                k += faceIndex[i];
            }
        }

        // generate the triangle index array and set normals and st coordinates
        for (uint32_t i = 0, k = 0, l = 0; i < nfaces; ++i) { // for each  face
            for (uint32_t j = 0; j < faceIndex[i] - 2; ++j) { // for each triangle in the face
                trisIndex[l] = vertsIndex[k];
                trisIndex[l + 1] = vertsIndex[k + j + 1];
                trisIndex[l + 2] = vertsIndex[k + j + 2];
                l += 3;
            }
            k += faceIndex[i];
        }
    }
    // Test if the ray interesests this triangle mesh
    bool intersect(const Vec3d& orig, const Vec3d& dir, double& tNear, uint32_t& triIndex, Vec2f& uv) const
    {
        uint32_t j = 0;
        bool isect = false;
        for (uint32_t i = 0; i < numTris; ++i) {
            const Vec3d& v0 = P[trisIndex[j]];
            const Vec3d& v1 = P[trisIndex[j + 1]];
            const Vec3d& v2 = P[trisIndex[j + 2]];
            double t = kInfinity, u, v;
            if (rayTriangleIntersect(orig, dir, v0, v1, v2, t, u, v) && t < tNear) {
                tNear = t;
                uv.x = u;
                uv.y = v;
                triIndex = i;
                isect = true;
            }
            j += 3;
        }

        return isect;
    }
    void getSurfaceProperties(
        const Vec3d& hitPoint,
        const Vec3d& viewDirection,
        const uint32_t& triIndex,
        const Vec2f& uv,
        Vec3d& hitNormal,
        Vec2f& hitTextureCoordinates) const
    {
        uint32_t vai[3]; // vertex attr index
        if (isSingleVertAttr) {
            vai[0] = trisIndex[triIndex * 3];
            vai[1] = trisIndex[triIndex * 3 + 1];
            vai[2] = trisIndex[triIndex * 3 + 2];
        }
        else {
            vai[0] = triIndex * 3;
            vai[1] = triIndex * 3 + 1;
            vai[2] = triIndex * 3 + 2;
        }
        if (smoothShading) {
            // vertex normal
            const Vec3d& n0 = N[vai[0]];
            const Vec3d& n1 = N[vai[1]];
            const Vec3d& n2 = N[vai[2]];
            hitNormal = (1 - uv.x - uv.y) * n0 + uv.x * n1 + uv.y * n2;
        }
        else {
            // face normal
            const Vec3d& v0 = P[trisIndex[triIndex * 3]];
            const Vec3d& v1 = P[trisIndex[triIndex * 3 + 1]];
            const Vec3d& v2 = P[trisIndex[triIndex * 3 + 2]];
            hitNormal = (v1 - v0).crossProduct(v2 - v0);
        }

        // doesn't need to be normalized as the N's are normalized but just for safety
        hitNormal.normalize();

        // texture coordinates
        const Vec2f& st0 = texCoordinates[vai[0]];
        const Vec2f& st1 = texCoordinates[vai[1]];
        const Vec2f& st2 = texCoordinates[vai[2]];
        hitTextureCoordinates = (1 - uv.x - uv.y) * st0 + uv.x * st1 + uv.y * st2;
    }
    void displayInfo() const
    {
        std::cerr << "Number of triangles in this mesh: " << numTris << std::endl;
        std::cerr << BBox[0] << ", " << BBox[1] << std::endl;
    }
    // member variables
    uint32_t numTris;                         // number of triangles
    std::vector<Vec3d> P;              // triangles vertex position
    std::vector<uint32_t> trisIndex;   // vertex index array
    std::vector<Vec3d> N;              // triangles vertex normals
    std::vector<Vec2f> texCoordinates; // triangles texture coordinates
    bool smoothShading = true;                // smooth shading by default
    bool isSingleVertAttr = true;
};

class Light
{
public:
    Light(const Matrix44d& l2w, const Vec3d& c = 1, const double& i = 1) : lightToWorld(l2w), color(c), intensity(i) {}
    virtual ~Light() {}
    virtual void illuminate(const Vec3d& P, Vec3d&, Vec3d&, double&) const {};
    Vec3d color;
    double intensity;
    Matrix44d lightToWorld;
};

struct IsectInfo
{
    const Object* hitObject = nullptr;
    double tNear = kInfinity;
    Vec2f uv;
    uint32_t index = 0;
};


bool trace(Vec3d& orig, const Vec3d& dir, std::vector<Object*>& objects, IsectInfo& isect);

Vec3d castRay(
    Vec3d& orig, const Vec3d& dir,
    std::vector<Object*>& objects,
    std::vector<Light>& lights,
    Options& options,
    uint32_t depth = 0)
{
    if (depth > options.maxDepth) return 0;
    Vec3d hitColor = 0;
    IsectInfo isect;
    if (trace(orig, dir, objects, isect)) {
        Vec3d hitPoint = orig + dir * isect.tNear;
        Vec3d hitNormal;
        Vec2f hitTexCoordinates;
        isect.hitObject->getSurfaceProperties(hitPoint, dir, isect.index, isect.uv, hitNormal, hitTexCoordinates);

        hitColor = std::max<double>(0.f, -hitNormal.dotProduct(dir));//* Vec3d(hitTexCoordinates.x, hitTexCoordinates.y, 1);
    }
    else {
        hitColor = 0.3;
    }

    return hitColor;
}

void render(
    Options& options,
    std::vector<Object*>& objects,
    std::vector<Light>& lights, tZBufferPtr pDestination)
{
    double scale = tan(deg2rad(options.fov * 0.5));
    double imageAspectRatio = options.width / (double)options.height;
    Vec3d orig;
    options.cameraToWorld.multVecMatrix(Vec3d(0), orig);
    auto timeStart = std::chrono::high_resolution_clock::now();
    double gamma = 1;

    ThreadPool pool(std::thread::hardware_concurrency());

    vector<shared_future<Vec3d> > pixelResults;



    for (uint32_t j = 0; j < options.height; ++j) {
        for (uint32_t i = 0; i < options.width; ++i) {
            // generate primary ray direction
            double x = (2 * (i + 0.5) / (double)options.width - 1) * imageAspectRatio * scale;
            double y = (1 - 2 * (j + 0.5) / (double)options.height) * scale;
            Vec3d dir;
            options.cameraToWorld.multDirMatrix(Vec3d(x, y, -1), dir);
            dir.normalize();

//            Vec3d pixel = castRay(orig, dir, objects, lights, options);

            pixelResults.emplace_back(pool.enqueue(&castRay, orig, dir, objects, lights, options, 0));
        }
    }

    vector<shared_future<Vec3d> >::iterator it = pixelResults.begin();
    for (uint32_t j = 0; j < options.height; ++j) {
        for (uint32_t i = 0; i < options.width; ++i) {

            Vec3d pixel = (*it).get();

            char r = (char)(255 * clamp(0, 1, powf(pixel.x, 1 / gamma)));
            char g = (char)(255 * clamp(0, 1, powf(pixel.y, 1 / gamma)));
            char b = (char)(255 * clamp(0, 1, powf(pixel.z, 1 / gamma)));
            pDestination.get()->SetPixel(i, j, ARGB(0xff, r, g, b));

            it++;
        }
    }


    auto timeEnd = std::chrono::high_resolution_clock::now();
    auto passedTime = std::chrono::duration<double, std::milli>(timeEnd - timeStart).count();
}

// [comment]
// Compute the position of a point along a Bezier curve at t [0:1]
// [/comment]
Vec3d evalBezierCurve(const Vec3d* P, const double& t)
{
    double b0 = (1 - t) * (1 - t) * (1 - t);
    double b1 = 3 * t * (1 - t) * (1 - t);
    double b2 = 3 * t * t * (1 - t);
    double b3 = t * t * t;

    return P[0] * b0 + P[1] * b1 + P[2] * b2 + P[3] * b3;
}

Vec3d evalBezierPatch(const Vec3d* controlPoints, const double& u, const double& v)
{
    Vec3d uCurve[4];
    for (int i = 0; i < 4; ++i)
        uCurve[i] = evalBezierCurve(controlPoints + 4 * i, u);

    return evalBezierCurve(uCurve, v);
}

Vec3d derivBezier(const Vec3d* P, const double& t)
{
    return -3 * (1 - t) * (1 - t) * P[0] +
        (3 * (1 - t) * (1 - t) - 6 * t * (1 - t)) * P[1] +
        (6 * t * (1 - t) - 3 * t * t) * P[2] +
        3 * t * t * P[3];
}

// [comment]
// Compute the derivative of a point on Bezier patch along the u parametric direction
// [/comment]
Vec3d dUBezier(const Vec3d* controlPoints, const double& u, const double& v)
{
    Vec3d P[4];
    Vec3d vCurve[4];
    for (int i = 0; i < 4; ++i) {
        P[0] = controlPoints[i];
        P[1] = controlPoints[4 + i];
        P[2] = controlPoints[8 + i];
        P[3] = controlPoints[12 + i];
        vCurve[i] = evalBezierCurve(P, v);
    }

    return derivBezier(vCurve, u);
}

// [comment]
// Compute the derivative of a point on Bezier patch along the v parametric direction
// [/comment]
Vec3d dVBezier(const Vec3d* controlPoints, const double& u, const double& v)
{
    Vec3d uCurve[4];
    for (int i = 0; i < 4; ++i) {
        uCurve[i] = evalBezierCurve(controlPoints + 4 * i, u);
    }

    return derivBezier(uCurve, v);
}

// [comment]
// Generate a poly-mesh Utah teapot out of Bezier patches
// [/comment]
void createPolyTeapot(Matrix44d& o2w, std::vector<Object*>& objects)
{
    uint32_t divs = 8;
    std::vector<Vec3d> P; P.resize((divs + 1) * (divs + 1));
    std::vector<uint32_t> nvertices; nvertices.resize(divs * divs);
    std::vector<uint32_t> vertices;vertices.resize(divs * divs * 4);
    std::vector<Vec3d> N; N.resize((divs + 1) * (divs + 1));
    std::vector<Vec2f> st; st.resize((divs + 1) * (divs + 1));

    // face connectivity - all patches are subdivided the same way so there
    // share the same topology and uvs
    for (uint16_t j = 0, k = 0; j < divs; ++j) {
        for (uint16_t i = 0; i < divs; ++i, ++k) {
            nvertices[k] = 4;
            vertices[k * 4] = (divs + 1) * j + i;
            vertices[k * 4 + 1] = (divs + 1) * j + i + 1;
            vertices[k * 4 + 2] = (divs + 1) * (j + 1) + i + 1;
            vertices[k * 4 + 3] = (divs + 1) * (j + 1) + i;
        }
    }

    Vec3d controlPoints[16];
    for (int np = 0; np < kTeapotNumPatches; ++np) { // kTeapotNumPatches
        // set the control points for the current patch
        for (uint32_t i = 0; i < 16; ++i)
            controlPoints[i][0] = teapotVertices[teapotPatches[np][i] - 1][0],
            controlPoints[i][1] = teapotVertices[teapotPatches[np][i] - 1][1],
            controlPoints[i][2] = teapotVertices[teapotPatches[np][i] - 1][2];

        // generate grid
        for (uint16_t j = 0, k = 0; j <= divs; ++j) {
            double v = j / (double)divs;
            for (uint16_t i = 0; i <= divs; ++i, ++k) {
                double u = i / (double)divs;
                P[k] = evalBezierPatch(controlPoints, u, v);
                Vec3d dU = dUBezier(controlPoints, u, v);
                Vec3d dV = dVBezier(controlPoints, u, v);
                N[k] = dU.crossProduct(dV).normalize();
                st[k].x = u;
                st[k].y = v;
            }
        }

        objects.emplace_back(new TriangleMesh(o2w, divs * divs, nvertices, vertices, P, N, st));
    }
}

// [comment]
// Bezier curve control points
// [/comment]
constexpr uint32_t curveNumPts = 22;
Vec3d curveData[curveNumPts] = {
    {-0.0029370324, 0.0297554422, 0},
    {-0.1556627219, 0.3293327560, 0},
    {-0.2613958914, 0.9578577085, 0},
    {-0.2555218265, 1.3044275420, 0},
    {-0.2496477615, 1.6509973760, 0},
    {-0.1262923970, 2.0445597290, 0},
    { 0.1791589818, 2.2853963930, 0},
    { 0.4846103605, 2.5262330570, 0},
    { 0.9427874287, 2.2560260680, 0},
    { 1.0132762080, 1.9212043650, 0},
    { 1.0837649880, 1.5863826610, 0},
    { 0.9369133637, 1.2750572170, 0},
    { 0.6667063748, 1.2691831520, 0},
    { 0.3964993859, 1.2633090870, 0},
    { 0.2320255666, 1.3514200620, 0},
    { 0.1850330468, 1.5276420110, 0},
    { 0.1380405269, 1.7038639600, 0},
    { 0.2026552417, 1.8918340400, 0},
    { 0.4082475158, 1.9564487540, 0},
    { 0.6138397900, 2.0210634690, 0},
    { 0.7606914144, 1.8800859100, 0},
    { 0.7606914144, 1.7038639600, 0}
};

// [comment]
// Generate a thin cylinder centred around a Bezier curve
// [/comment]
void createCurveGeometry(std::vector<Object*>& objects)
{
    uint32_t ndivs = 16;
    uint32_t ncurves = 1 + (curveNumPts - 4) / 3;
    Vec3d pts[4];
    std::vector<Vec3d> P; P.resize((ndivs + 1) * ndivs * ncurves + 1);
    std::vector<Vec3d> N; N.resize((ndivs + 1) * ndivs * ncurves + 1);
    std::vector<Vec2f> st; st.resize((ndivs + 1) * ndivs * ncurves + 1);
    for (uint32_t i = 0; i < ncurves; ++i) {
        for (uint32_t j = 0; j < ndivs; ++j) {
            pts[0] = curveData[i * 3];
            pts[1] = curveData[i * 3 + 1];
            pts[2] = curveData[i * 3 + 2];
            pts[3] = curveData[i * 3 + 3];
            double s = j / (double)ndivs;
            Vec3d pt = evalBezierCurve(pts, s);
            Vec3d tangent = derivBezier(pts, s).normalize();
            bool swap = false;

            uint8_t maxAxis;
            if (std::abs(tangent.x) > std::abs(tangent.y))
                if (std::abs(tangent.x) > std::abs(tangent.z))
                    maxAxis = 0;
                else
                    maxAxis = 2;
            else if (std::abs(tangent.y) > std::abs(tangent.z))
                maxAxis = 1;
            else
                maxAxis = 2;

            Vec3d up, forward, right;

            switch (maxAxis) {
            case 0:
            case 1:
                up = tangent;
                forward = Vec3d(0, 0, 1);
                right = up.crossProduct(forward);
                forward = right.crossProduct(up);
                break;
            case 2:
                up = tangent;
                right = Vec3d(0, 0, 1);
                forward = right.crossProduct(up);
                right = up.crossProduct(forward);
                break;
            default:
                break;
            };

            double sNormalized = (i * ndivs + j) / double(ndivs * ncurves);
            double rad = 0.1 * (1 - sNormalized);
            for (uint32_t k = 0; k <= ndivs; ++k) {
                double t = k / (double)ndivs;
                double theta = t * 2 * M_PI;
                Vec3d pc(cos(theta) * rad, 0, sin(theta) * rad);
                double x = pc.x * right.x + pc.y * up.x + pc.z * forward.x;
                double y = pc.x * right.y + pc.y * up.y + pc.z * forward.y;
                double z = pc.x * right.z + pc.y * up.z + pc.z * forward.z;
                P[i * (ndivs + 1) * ndivs + j * (ndivs + 1) + k] = Vec3d(pt.x + x, pt.y + y, pt.z + z);
                N[i * (ndivs + 1) * ndivs + j * (ndivs + 1) + k] = Vec3d(x, y, z).normalize();
                st[i * (ndivs + 1) * ndivs + j * (ndivs + 1) + k] = Vec2f(sNormalized, t);
            }
        }
    }
    P[(ndivs + 1) * ndivs * ncurves] = curveData[curveNumPts - 1];
    N[(ndivs + 1) * ndivs * ncurves] = (curveData[curveNumPts - 2] - curveData[curveNumPts - 1]).normalize();
    st[(ndivs + 1) * ndivs * ncurves] = Vec2f(1, 0.5);
    uint32_t numFaces = ndivs * ndivs * ncurves;
    std::vector<uint32_t> verts; verts.resize(numFaces);
    for (uint32_t i = 0; i < numFaces; ++i)
        verts[i] = (i < (numFaces - ndivs)) ? 4 : 3;
    std::vector<uint32_t> vertIndices; vertIndices.resize(ndivs * ndivs * ncurves * 4 + ndivs * 3);
    uint32_t nf = 0, ix = 0;
    for (uint32_t k = 0; k < ncurves; ++k) {
        for (uint32_t j = 0; j < ndivs; ++j) {
            if (k == (ncurves - 1) && j == (ndivs - 1)) { break; }
            for (uint32_t i = 0; i < ndivs; ++i) {
                vertIndices[ix] = nf;
                vertIndices[ix + 1] = nf + (ndivs + 1);
                vertIndices[ix + 2] = nf + (ndivs + 1) + 1;
                vertIndices[ix + 3] = nf + 1;
                ix += 4;
                ++nf;
            }
            nf++;
        }
    }

    for (uint32_t i = 0; i < ndivs; ++i) {
        vertIndices[ix] = nf;
        vertIndices[ix + 1] = (ndivs + 1) * ndivs * ncurves;
        vertIndices[ix + 2] = nf + 1;
        ix += 3;
        nf++;
    }

    objects.emplace_back(new TriangleMesh(Matrix44d::kIdentity, numFaces, verts, vertIndices, P, N, st));
}


void Z3DTestWin::RenderTeapot()
{
    // loading gemetry
    std::vector<Object*> objects;

    Matrix44d teapotO2W;
    //    Z3D::LookAt(Vec3d(4, 5, -10), Vec3d(0, 0, 0), Vec3d(0, 10, 0), teapotO2W);
    setOrientationMatrix((double)gTimer.GetElapsedTime() / 1000.0, (double)gTimer.GetElapsedTime() / 2500.0, 0.0, teapotO2W);
    createPolyTeapot(teapotO2W, objects);
    //createCurveGeometry(objects);

    // lights
    std::vector<Light> lights;
    Options options;

    // aliasing example
    options.fov = 39.89;
    options.width = 128;
    options.height = 128;
    options.maxDepth = 1;


    mpTeapotRender.get()->Init(options.width, options.height);

    // to render the teapot
    //options.cameraToWorld = Matrix44d(0.897258, 0, -0.441506, 0, -0.288129, 0.757698, -0.585556, 0, 0.334528, 0.652606, 0.679851, 0, 5.439442, 11.080794, 10.381341, 1);
    Z3D::LookAt(Vec3d(4, 5, -10), Vec3d(0, 0, 0), Vec3d(0, 10, 0), options.cameraToWorld);


    // to render the curve as geometry
    //options.cameraToWorld = Matrix44d(0.707107, 0, -0.707107, 0, -0.369866, 0.85229, -0.369866, 0, 0.60266, 0.523069, 0.60266, 0, 2.634, 3.178036, 2.262122, 1);

    // finally, render
    render(options, objects, lights, mpTeapotRender);
    for (auto o : objects)
        delete o;
}


bool trace(
    Vec3d& orig, const Vec3d& dir,
    std::vector<Object*>& objects,
    IsectInfo& isect)
{
    isect.hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        double tNearTriangle = kInfinity;
        uint32_t indexTriangle;
        Vec2f uvTriangle;
        if (objects[k]->intersect(orig, dir, tNearTriangle, indexTriangle, uvTriangle) && tNearTriangle < isect.tNear)
        {
            isect.hitObject = objects[k];
            isect.tNear = tNearTriangle;
            isect.index = indexTriangle;
            isect.uv = uvTriangle;
        }
    }

    return (isect.hitObject != nullptr);
}

#endif

#ifdef RENDER_SPHERES


class Sphere
{
public:
    Sphere() : mRadius(0), mRadius2(0), mTransparency(0), mReflection(0) {}

    Sphere(
        const Vec3d& center,
        const double& radius,
        const Vec3d& surfaceColor,
        const double& reflection = 0,
        const double& transparency = 0,
        const Vec3d& emmisionColor = 0) :
        mCenter(center), mRadius(radius), mRadius2(radius* radius), mSurfaceColor(surfaceColor), mEmissionColor(emmisionColor),
        mTransparency(transparency), mReflection(reflection)
    {
    }

    //[comment]
    // Compute a ray-sphere intersection using the geometric solution
    //[/comment]
    bool intersect(const Vec3d& rayorig, const Vec3d& raydir, double& t0, double& t1) const
    {
        Vec3d l = mCenter - rayorig;
        double tca = l.dotProduct(raydir);
        if (tca < 0) return false;
        double d2 = l.dotProduct(l) - tca * tca;
        if (d2 > mRadius2) return false;
        double thc = sqrt(mRadius2 - d2);
        t0 = tca - thc;
        t1 = tca + thc;

        return true;
    }


    Vec3d mCenter;                           /// position of the sphere
    double mRadius, mRadius2;                  /// sphere radius and radius^2
    Vec3d mSurfaceColor, mEmissionColor;      /// surface color and emission (light)
    double mTransparency, mReflection;         /// surface transparency and reflectivity

};

//#define MAX_RAY_DEPTH 5

double mix(const double& a, const double& b, const double& mix)
{
    return b * mix + a * (1 - mix);
}

Vec3d TraceSpheres(
    const Vec3d& rayorig,
    const Vec3d& raydir,
    const int& depthRemaining, const std::vector<class Sphere>& spheres)
{
    //if (raydir.length() != 1) std::cerr << "Error " << raydir << std::endl;
    double tnear = INFINITY;
    const Sphere* sphere = NULL;
    // find intersection of this ray with the sphere in the scene
    for (unsigned i = 0; i < spheres.size(); ++i) {
        double t0 = INFINITY, t1 = INFINITY;
        if (spheres[i].intersect(rayorig, raydir, t0, t1)) {
            if (t0 < 0) t0 = t1;
            if (t0 < tnear) {
                tnear = t0;
                sphere = &spheres[i];
            }
        }
    }
    // if there's no intersection return black or background color
    if (!sphere) return Vec3d(2);
    Vec3d surfaceColor = 0; // color of the ray/surfaceof the object intersected by the ray
    Vec3d phit = rayorig + raydir * tnear; // point of intersection
    Vec3d nhit = phit - sphere->mCenter; // normal at the intersection point
    nhit.normalize(); // normalize normal direction
    // If the normal and the view direction are not opposite to each other
    // reverse the normal direction. That also means we are inside the sphere so set
    // the inside bool to true. Finally reverse the sign of IdotN which we want
    // positive.
    double bias = (double)1e-4; // add some bias to the point from which we will be tracing
    bool inside = false;
    if (raydir.dotProduct(nhit) > 0) nhit = -nhit, inside = true;
    if ((sphere->mTransparency > 0 || sphere->mReflection > 0) && depthRemaining > 0) {
        double facingratio = -raydir.dotProduct(nhit);
        // change the mix value to tweak the effect
        double fresneleffect = mix((const double) pow(1 - facingratio, 3), 1, 0.1f);
        // compute reflection direction (not need to normalize because all vectors
        // are already normalized)
        Vec3d refldir = raydir - nhit * 2 * raydir.dotProduct(nhit);
        refldir.normalize();
        Vec3d reflection = TraceSpheres(phit + nhit * bias, refldir, depthRemaining-1, spheres);
        Vec3d refraction = 0;
        // if the sphere is also transparent compute refraction ray (transmission)
        if (sphere->mTransparency) {
            double ior = 1.1f, eta = (inside) ? ior : 1 / ior; // are we inside or outside the surface?
            double cosi = -nhit.dotProduct(raydir);
            double k = 1 - eta * eta * (1 - cosi * cosi);
            Vec3d refrdir = raydir * eta + nhit * (eta * cosi - sqrt(k));
            refrdir.normalize();
            refraction = TraceSpheres(phit - nhit * bias, refrdir, depthRemaining - 1, spheres);
        }
        // the result is a mix of reflection and refraction (if the sphere is transparent)
        surfaceColor = (
            reflection * fresneleffect +
            refraction * (1 - fresneleffect) * sphere->mTransparency) * sphere->mSurfaceColor;
    }
    else {
        // it's a diffuse object, no need to raytrace any further
        for (unsigned i = 0; i < spheres.size(); ++i) {
            if (spheres[i].mEmissionColor.x > 0) {
                // this is a light
                Vec3d transmission = 1;
                Vec3d lightDirection = spheres[i].mCenter - phit;
                lightDirection.normalize();
                for (unsigned j = 0; j < spheres.size(); ++j) {
                    if (i != j) {
                        double t0, t1;
                        if (spheres[j].intersect(phit + nhit * bias, lightDirection, t0, t1)) {
                            transmission = 0;
                            break;
                        }
                    }
                }
                surfaceColor += sphere->mSurfaceColor * transmission *
                    std::max<double>(0.0, nhit.dotProduct(lightDirection)) * spheres[i].mEmissionColor;
            }
        }
    }

    return surfaceColor + sphere->mEmissionColor;
}



void ThreadTrace(ZRect rArea, std::vector<Sphere>& spheres, ZBuffer* pDest, int64_t nDepth, double fov)
{
    ZRect rFullArea(pDest->GetArea());
    double invWidth = 1 / double(rFullArea.Width()), invHeight = 1 / double(rFullArea.Height());
    double /*fov = 30,*/ aspectratio = rFullArea.Width() / double(rFullArea.Height());

    double angle = (double)tan( M_PI * 0.5 * fov / 180.);

    for (int64_t y = rArea.top; y < rArea.bottom; ++y) {
        for (int64_t x = rArea.left; x < rArea.right; ++x) {
            double xx = ( 2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
            double yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;
            Vec3d raydir(xx, yy, -1);
            raydir.normalize();
            Vec3d pixel = TraceSpheres(Vec3d(0), raydir, (const int)nDepth, spheres);

            char r = (char)(255 * clamp(0, 1, (const double)pixel.x));
            char g = (char)(255 * clamp(0, 1, (const double)pixel.y));
            char b = (char)(255 * clamp(0, 1, (const double)pixel.z));
            pDest->SetPixel(x, y, ARGB(0xff, r, g, b));
        }
    }
}

//[comment]
// Main rendering function. We compute a camera ray for each pixel of the image
// trace it and return a color. If the ray hits a sphere, we return the color of the
// sphere at the intersection point, else we return the background color.
//[/comment]
void Z3DTestWin::RenderSpheres(tZBufferPtr pSurface)
{
    int64_t width = mnRenderSize;
    int64_t height = mnRenderSize;

    if (pSurface->GetArea().Width() != width || pSurface->GetArea().Height() != height)
        pSurface->Init(width, height);

    size_t nThreadCount = std::thread::hardware_concurrency();

    std::vector<std::thread> workers;

    int64_t nTop = 0;
    int64_t nLines = height / nThreadCount;

    for (uint32_t i = 0; i < nThreadCount; i++)
    {
        int64_t nBottom = nTop + nLines;

        // last thread? take the rest of the scanlines
        if (i == nThreadCount - 1)
            nBottom = height;

        ZRect r(0, nTop, width, nBottom);
        workers.emplace_back(std::thread(ThreadTrace, (ZRect)r, std::ref(mSpheres), (ZBuffer*)pSurface.get(), mnRayDepth, (mnFOVTime100 / 100.0)));

        nTop += nLines;
    }

    for (uint32_t i = 0; i < nThreadCount; i++)
    {
        workers[i].join();
    }


/*

    // Trace rays
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            float xx = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
            float yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;
            Vec3d raydir(xx, yy, -1);
            raydir.normalize();
            Vec3d pixel = TraceSpheres(Vec3d(0), raydir, 0);

            char r = (char)(255 * clamp(0, 1, pixel.x));
            char g = (char)(255 * clamp(0, 1, pixel.y));
            char b = (char)(255 * clamp(0, 1, pixel.z));
            mpSurface.get()->SetPixel(x, y, ARGB(0xff, r, g, b));

        }
    }*/
}


#endif


const int64_t kDefaultMinSphereSize = 10;
const int64_t kDefaultMaxSphereSize = 400;

Z3DTestWin::Z3DTestWin()
{
    mnTargetSphereCount = 15;
    mnMinSphereSizeTimes100 = kDefaultMinSphereSize;
    mnMaxSphereSizeTimes100 = kDefaultMaxSphereSize/2;
    mnFOVTime100 = 30 * 100;
    mnRotateSpeed = 10;
    mnRayDepth = 3;
    mfBaseAngle = 0.0;

    mnCameraX = 0;
    mnCameraY = 200;
    mnCameraZ = 1310;


    mbRenderCube = false;
    mbRenderSpheres = true;
    mbOuterSphere = true;
    mbCenterSphere = true;
    mnRenderSize = 256;
    mbControlPanelEnabled = true;
//    pLightBuffer = nullptr;

    mIdleSleepMS = 16;

    mLastTimeStamp = gTimer.GetMSSinceEpoch();
    msWinName = "3dtestwin";
}
   
bool Z3DTestWin::Init()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mbInvalidateParentWhenInvalid = true;


    if (!gRegistry.Get("3dtestwin", "render_size", mnRenderSize))
    {
        mnRenderSize = 256;
        gRegistry.SetDefault("3dtestwin", "render_size", mnRenderSize);
    }

    if (!gRegistry.Get("3dtestwin", "render_cubes", mbRenderCube))
    {
        mbRenderCube = false;
        gRegistry.SetDefault("3dtestwin", "render_cubes", mbRenderCube);
    }

    if (!gRegistry.Get("3dtestwin", "render_spheres", mbRenderSpheres))
    {
        mbRenderSpheres = true;
        gRegistry.SetDefault("3dtestwin", "render_spheres", mbRenderSpheres);
    }


    if (!gRegistry.Get("3dtestwin", "outersphere", mbOuterSphere))
    {
        mbOuterSphere = true;
        gRegistry.SetDefault("3dtestwin", "outersphere", mbOuterSphere);
    }

    if (!gRegistry.Get("3dtestwin", "centersphere", mbCenterSphere))
    {
        mbCenterSphere = true;
        gRegistry.SetDefault("3dtestwin", "centersphere", mbCenterSphere);
    }

    if (!gRegistry.Get("3dtestwin", "rotatespeed", mnRotateSpeed))
    {
        mnRotateSpeed = true;
        gRegistry.SetDefault("3dtestwin", "rotatespeed", mnRotateSpeed);
    }

    if (!gRegistry.Get("3dtestwin", "raydepth", mnRayDepth))
    {
        mnRayDepth = true;
        gRegistry.SetDefault("3dtestwin", "raydepth", mnRayDepth);
    }

    
    mLight.color = { 1.0f, 0.5f, 0.2f };
    mLight.direction = { 1.0f, 0.0f, -1.0f };
    mLight.intensity = 0.8;


    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(mLight);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    HRESULT hr = ZD3D::mD3DDevice->CreateBuffer(&bufferDesc, nullptr, &ZD3D::pD3DLightBuffer);
    if (FAILED(hr))
    {
        assert(false);
    }
//    mLight.pD3DBuffer = pLightBuffer;

    


    mCubeVertices.resize(8);

    const float f = 1.0;

    mCubeVertices[0] = Vec3d(-f,  f, -f);
    mCubeVertices[1] = Vec3d( f,  f, -f);
    mCubeVertices[2] = Vec3d( f, -f, -f);
    mCubeVertices[3] = Vec3d(-f, -f, -f);

    mCubeVertices[4] = Vec3d(-f,  f,  f);
    mCubeVertices[5] = Vec3d( f,  f,  f);
    mCubeVertices[6] = Vec3d( f, -f,  f);
    mCubeVertices[7] = Vec3d(-f, -f,  f);

    mSides.resize(6);

    mSides[0].mSide[0] = 0;
    mSides[0].mSide[1] = 1;
    mSides[0].mSide[2] = 2;
    mSides[0].mSide[3] = 3;

    mSides[1].mSide[0] = 1;
    mSides[1].mSide[1] = 5;
    mSides[1].mSide[2] = 6;
    mSides[1].mSide[3] = 2;

    mSides[2].mSide[0] = 5;
    mSides[2].mSide[1] = 4;
    mSides[2].mSide[2] = 7;
    mSides[2].mSide[3] = 6;

    mSides[3].mSide[0] = 4;
    mSides[3].mSide[1] = 0;
    mSides[3].mSide[2] = 3;
    mSides[3].mSide[3] = 7;

    mSides[4].mSide[0] = 4;
    mSides[4].mSide[1] = 5;
    mSides[4].mSide[2] = 1;
    mSides[4].mSide[3] = 0;

    mSides[5].mSide[0] = 6;
    mSides[5].mSide[1] = 7;
    mSides[5].mSide[2] = 3;
    mSides[5].mSide[3] = 2;

    mSides[0].mColor = 0xff0000ff;
    mSides[1].mColor = 0xffff0000;
    mSides[2].mColor = 0xff00ff00;
    mSides[3].mColor = 0xffff00ff;

    mSides[4].mColor = 0xffffffff;
    mSides[5].mColor = 0xff000000;


    mpTexture.reset(new ZBuffer());
    mpTexture.get()->LoadBuffer("res/brick-texture.jpg");

    mpDynTexture.reset(new ZD3D::DynamicTexture);
    mpDynTexture->Init(mpTexture->GetArea().BR());
    mpDynTexture->UpdateTexture(mpTexture.get());

    mpGrassTexture.reset(new ZBuffer());
    mpGrassTexture.get()->LoadBuffer("res/grass-texture.jpg");
    mpGrassDynTexture.reset(new ZD3D::DynamicTexture);
    mpGrassDynTexture->Init(mpGrassTexture->GetArea().BR());
    mpGrassDynTexture->UpdateTexture(mpGrassTexture.get());






    SetFocus();



#ifdef RENDER_TEAPOT
    mpTeapotRender.reset(new ZBuffer());
    RenderTeapot();
#endif

#ifdef RENDER_SPHERES
    mpSpheresRender.reset(new ZBuffer());
    UpdateSphereCount();
#endif

    if (mbControlPanelEnabled)
    {

        int64_t panelW = grFullArea.Width() / 10;
        int64_t panelH = grFullArea.Height() / 2;
        ZRect rControlPanel(grFullArea.right - panelW, grFullArea.bottom - panelH, grFullArea.right, grFullArea.bottom);     // upper right for now

        ZWinControlPanel* pCP = new ZWinControlPanel();
        pCP->SetArea(rControlPanel);

//        tZFontPtr pBtnFont(gpFontSystem->GetFont(gDefaultButtonFont));


        pCP->Init();

        string sUpdateSphereCountMsg(ZMessage("updatespherecount", this));

        pCP->Caption("Sphere Count", "Sphere Count");
        pCP->Slider("spherecount", &mnTargetSphereCount, 1, 50, 1, 0.25, sUpdateSphereCountMsg, true, false);
        //    pCP->AddSpace(16);

        pCP->Caption("minspheresize", "Min Sphere Size");
        pCP->Slider("minspheresize", &mnMinSphereSizeTimes100, kDefaultMinSphereSize, kDefaultMaxSphereSize, 1, 0.25, sUpdateSphereCountMsg, true, false);

        pCP->Caption("maxspheresize", "Max Sphere Size");
        pCP->Slider("maxspheresize", &mnMaxSphereSizeTimes100, kDefaultMinSphereSize, kDefaultMaxSphereSize, 1, 0.25, sUpdateSphereCountMsg, true, false);

        pCP->Caption("speed", "Speed");
        pCP->Slider("rotatespeed", &mnRotateSpeed, 0, 500, 1, 0.25, ZMessage("updatesettings", this), true, false);

        pCP->Caption("fov", "FOV");
        pCP->Slider("fov", &mnFOVTime100, 100, 18000, 1, 0.25, ZMessage("updatesettings", this), false, false);


        pCP->Caption("raydepth", "Ray Depth");
        pCP->Slider("raydepth", &mnRayDepth, 0, 10, 1, 0.25, ZMessage("updatesettings", this), true, false);



        pCP->AddSpace(16);
        pCP->Caption("rendersize", "Render Size");
        pCP->Slider("rendersize", &mnRenderSize, 1, 128, 16, 0.25, ZMessage("updatesettings", this), true);

        pCP->AddSpace(16);
        pCP->Caption("camera", "camera");
        pCP->Slider("camerax", &mnCameraX, -1000, 1000, 10, 0.25, {}, true);
        pCP->Slider("cameray", &mnCameraY, -1000, 1000, 10, 0.25, {}, true);
        pCP->Slider("cameraz", &mnCameraZ, -1000, 1000, 10, 0.25, {}, true);






        ZGUI::ZTextLook toggleLook(ZGUI::ZTextLook::kEmbossed, 0xff737373, 0xff737373);

        pCP->AddSpace(16);

        ZWinCheck* pToggle;

        pToggle = pCP->Toggle("rendercubes", &mbRenderCube, "Render Cubes", ZMessage("updatesettings", this), ZMessage("updatesettings", this));
        pToggle->msWinGroup = "rendermode";

        pToggle = pCP->Toggle("renderspheres", &mbRenderSpheres, "Render Spheres", ZMessage("updatesettings", this), ZMessage("updatesettings", this));
        pToggle->msWinGroup = "rendermode";

        pCP->Toggle("outersphere", &mbOuterSphere, "Outer Sphere", sUpdateSphereCountMsg, sUpdateSphereCountMsg);
        pCP->Toggle("centersphere", &mbCenterSphere, "Center Sphere", sUpdateSphereCountMsg, sUpdateSphereCountMsg);

        ChildAdd(pCP);
    }
    else
    {
        mnRenderSize = mAreaLocal.Height() / 2;
        mnTargetSphereCount = 1 + rand() % 10;
        mbCenterSphere = RANDBOOL;
        mbOuterSphere = RANDBOOL;
        mnRayDepth = RANDI64(0, 4);
        if (RANDI64(0, 10) == 1)
        {
            mbRenderCube = RANDBOOL;
            mbRenderSpheres = RANDBOOL;
        }
        mnRotateSpeed = RANDI64(0, 10);
        msWinName = "";
    }


    return ZWin::Init();
}


bool Z3DTestWin::Shutdown()
{
    for (auto& p : mReservedPrims)
        p->clear();
    mReservedPrims.clear();

//    if (pLightBuffer)
//        pLightBuffer->Release();
    //pLightBuffer = nullptr;

    return ZWin::Shutdown();
}

void Z3DTestWin::UpdateSphereCount()
{
    ZASSERT(mnTargetSphereCount > 0 && mnTargetSphereCount < 10000);

    int64_t nCount = mnTargetSphereCount + mbOuterSphere + mbCenterSphere;
    mSpheres.resize(nCount);
    int i = 0;
    if (mbOuterSphere)
    {
        mSpheres[i] = Sphere(Vec3d(0, 0, -50), 500, Vec3d(0, 0, 0), 0.0, 0.0, Vec3d(0,0,0));
        i++;
    }

    if (mbCenterSphere)
    {
        mSpheres[i] = Sphere(Vec3d(0, 0, -50), 5, Vec3d(0.5, 0.5, 0.5), 0.5, 0.5, Vec3d(0, 0, 0));
        i++;
    }

    for (;i < mnTargetSphereCount; i++)
    {
        double fMaxDist = 100.0;
        double fRadius = RANDDOUBLE( (mnMinSphereSizeTimes100 / 100.0), (mnMaxSphereSizeTimes100 / 100.0));

        Vec3d center(fMaxDist * sin(RANDDOUBLE(0.0, (2.0 * M_PI))),
                     fMaxDist * sin(RANDDOUBLE(0.0, (2.0 * M_PI))), 
                     fMaxDist * sin(RANDDOUBLE(0.0, (2.0 * M_PI))));

        Vec3d color(RANDDOUBLE(0.0, 1.0), RANDDOUBLE(0.0, 1.0), RANDDOUBLE(0.0, 1.0));
        double fReflective(RANDDOUBLE(0.0, 1.0));
        double fTransparent(RANDDOUBLE(0.0, 1.0));
        Vec3d emmisionColor(0);

        if (RANDPERCENT(10))    // 10% of spheres will be emitters
            emmisionColor = Vec3d(RANDDOUBLE(0.0, 1.0), RANDDOUBLE(0.0, 1.0), RANDDOUBLE(0.0, 1.0));

        mSpheres[i] = Sphere(center, fRadius, color, fReflective, fTransparent, emmisionColor);
    }

//    RenderSpheres(mpSpheresRender);
}

bool Z3DTestWin::OnChar(char key)
{
#ifdef _WIN64
    switch (key)
    {
    case VK_ESCAPE:
        gMessageSystem.Post("{quit_app_confirmed}");
        break;
    }
#endif
    return ZWin::OnChar(key);
}



void multPointMatrix(const Vec3d& in, Vec3d& out, const Matrix44d& M)
{
    //out = in * M;
    out.x = in.x * M[0][0] + in.y * M[1][0] + in.z * M[2][0] + /* in.z = 1 */ M[3][0];
    out.y = in.x * M[0][1] + in.y * M[1][1] + in.z * M[2][1] + /* in.z = 1 */ M[3][1];
    out.z = in.x * M[0][2] + in.y * M[1][2] + in.z * M[2][2] + /* in.z = 1 */ M[3][2];
    double w = in.x * M[0][3] + in.y * M[1][3] + in.z * M[2][3] + /* in.z = 1 */ M[3][3];

    if (w != 0.0) 
    {
        out.x /= w;
        out.y /= w;
        out.z /= w;
    }
}




bool Z3DTestWin::Process()
{
    return true;
}

void Z3DTestWin::RenderPoly(vector<Vec3d>& worldVerts, Matrix44d& mtxProjection, Matrix44d& mtxWorldToCamera, uint32_t nCol)
{
    tColorVertexArray screenVerts;
    screenVerts.resize(worldVerts.size());




    for (int i = 0; i < worldVerts.size(); i++)
    {
        Vec3d v = worldVerts[i];

        Vec3d vertCamera;
        Vec3d vertProjected;

        multPointMatrix(v, vertCamera, mtxWorldToCamera);
        multPointMatrix(vertCamera, vertProjected, mtxProjection);

/*        if (vertProjected.x < -1 || vertProjected.x > 1 || vertProjected.y < -1 || vertProjected.y > 1)
            continue;*/

        screenVerts[i].x = (double)(mAreaLocal.Width()/2 + (int64_t)(vertProjected.x * mnRenderSize *10));
        screenVerts[i].y = (double)(mAreaLocal.Height()/2 + (int64_t)(vertProjected.y * mnRenderSize *10));

        screenVerts[i].mColor = nCol;
    }

    Vec3d planeX(screenVerts[1].x- screenVerts[0].x, screenVerts[1].y - screenVerts[0].y, 1);
    Vec3d planeY(screenVerts[0].x - screenVerts[3].x, screenVerts[0].y - screenVerts[3].y, 1);
    Vec3d normal = planeX.crossProduct(planeY);
    if (normal.z > 0)
        return;

    gRasterizer.Rasterize(mpSurface.get(), screenVerts);
}

void Z3DTestWin::RenderPoly(vector<Vec3d>& worldVerts, Matrix44d& mtxProjection, Matrix44d& mtxWorldToCamera, ZD3D::tDynamicTexturePtr pTexture, ZD3D::Light* pLight)
{
//    tUVVertexArray screenVerts;
//    screenVerts.resize(worldVerts.size());


    if (mReservedPrims.size() <= mFramePrimCount)
    {
        mReservedPrims.push_back(ZD3D::ReservePrimitive());
    }

    ZD3D::ScreenSpacePrimitive* pPrim = mReservedPrims[mFramePrimCount];
    mFramePrimCount++;
    

//    std::vector<Vertex> verts(4);

    pPrim->verts.resize(4);

    int mapping[4] = { 0, 1, 3, 2 };
    
    Vec3d planeX(worldVerts[1].x - worldVerts[0].x, worldVerts[1].y - worldVerts[0].y, worldVerts[1].z - worldVerts[0].z);
    Vec3d planeY(worldVerts[0].x - worldVerts[3].x, worldVerts[0].y - worldVerts[3].y, worldVerts[0].z - worldVerts[3].z);
    Vec3d normal = planeX.crossProduct(planeY).normalize();
    
    DirectX::XMFLOAT3 d3dNormal;
    d3dNormal.x = normal.x;
    d3dNormal.y = normal.y;
    d3dNormal.z = normal.z;

/*    d3dNormal.x = (float)(rand()%1000-500);
    d3dNormal.y = (float)(rand()%1000-500);
    d3dNormal.z = (float)(rand()%1000-500);*/


    float f = sqrt(d3dNormal.x * d3dNormal.x + d3dNormal.y * d3dNormal.y + d3dNormal.z * d3dNormal.z);

    d3dNormal.x /= f;
    d3dNormal.y /= f;
    d3dNormal.z /= f;


/*    XMVECTOR lightVec = XMLoadFloat3(&mLight.direction);
    lightVec = XMVector3Normalize(lightVec);
    XMStoreFloat3(&mLight.direction, lightVec);*/
    



    for (int i = 0; i < worldVerts.size(); i++)
    {
        Vec3d v = worldVerts[mapping[i]];

        Vec3d vertCamera;
        Vec3d vertProjected;

        multPointMatrix(v, vertCamera, mtxWorldToCamera);
        multPointMatrix(vertCamera, vertProjected, mtxProjection);
        pPrim->SetVertex(i, (double)(mAreaLocal.Width() / 2 + (int64_t)(vertProjected.x * mnRenderSize * 10)), (double)(mAreaLocal.Height() / 2 + (int64_t)(vertProjected.y * mnRenderSize * 10)), vertProjected.z-1.0, d3dNormal, 0.0, 0.0);
    }


    pPrim->light = pLight;

    pPrim->verts[0].uv.x = 0.0;
    pPrim->verts[0].uv.y = 0.0;

    pPrim->verts[1].uv.x = 1.0;
    pPrim->verts[1].uv.y = 0.0;

    pPrim->verts[2].uv.x = 0.0;
    pPrim->verts[2].uv.y = 1.0;
    
    pPrim->verts[3].uv.x = 1.0;
    pPrim->verts[3].uv.y = 1.0;

    pPrim->texture = pTexture;
    pPrim->state = ZD3D::ScreenSpacePrimitive::eState::kVisible;
    pPrim->ps = ZD3D::GetPixelShader("GrassShader");
//    ZD3D::AddPrim(ZD3D::GetVertexShader("ScreenSpaceShader"), ZD3D::GetPixelShader("ScreenSpaceShader"), &mDynTexture, verts);
}




bool Z3DTestWin::Paint()
{
    if (!PrePaintCheck())
        return false;

#ifdef RENDER_TEAPOT
    RenderTeapot();
#endif

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());
    mpSurface->FillAlpha(0xff000000);

    mfBaseAngle += (mnRotateSpeed / 10000.0) * gTimer.GetElapsedTime() / 10000.0;

    if (mbRenderSpheres)
    {
        double fAngle = mfBaseAngle;

        int nStartSphere = 0;
        if (mbCenterSphere)
            nStartSphere++;
        if (mbOuterSphere)
            nStartSphere++;
        for (int i = nStartSphere; i < mSpheres.size(); i++)
        {
            mSpheres[i].mCenter = Vec3d(5 * cos(fAngle * mSpheres[i].mSurfaceColor.length()), 5.0 * sin(fAngle * mSpheres[i].mTransparency), -50+10.0 * sin(fAngle * mSpheres[i].mReflection));
            fAngle += 1.2;
        }

        RenderSpheres(mpSpheresRender);
    }


    uint64_t nTime = gTimer.GetMSSinceEpoch();
    string sTime;
    Sprintf(sTime, "fps: %f", 1000.0 / (nTime - mLastTimeStamp));
    gpFontSystem->GetDefaultFont()->DrawText(mpSurface.get(), sTime, mAreaLocal);
    mLastTimeStamp = nTime;



    if (mbRenderCube)
    {
        Matrix44d mtxProjection;
        Matrix44d mtxWorldToCamera;

        Z3D::LookAt(Vec3d(0, 0, -20), Vec3d(0, 0, 0), Vec3d(0, 10, 0), mtxWorldToCamera);
//        Z3D::LookAt(Vec3d(mnCameraX / 100.0, mnCameraY / 100.0, mnCameraZ/100.0), Vec3d(0, 0, 0), Vec3d(0, 10, 0), mtxWorldToCamera);

        double fFoV = 90;
        double fNear = 1.0;
        double fFar = 500;

        double fAspect = (double)grFullArea.Width() / (double)grFullArea.Height();

        setProjectionMatrix(fFoV, fAspect, fNear, fFar, mtxProjection);

        vector<Vec3d> worldVerts;
        worldVerts.resize(4);

        mFramePrimCount = 0;
        int cubenum = 0;
        Matrix44d mtxRotation;
        Matrix44d mtxTranslation;

        // plane
        setOrientationMatrix(4.0, 0.0, mfBaseAngle + cubenum, mtxRotation);
        setTranslationMatrix(mnCameraX / 10.0, mnCameraY / 10.0, mnCameraZ / 10.0, mtxTranslation);
        mObjectToWorld = mtxRotation * mtxTranslation;

        std::vector<Vec3d> verts(4);
        verts[0] = Vec3d(-10, 10, 500);
        verts[1] = Vec3d(-10, -10, 500);
        verts[3] = Vec3d(10, 10, 1500);
        verts[2] = Vec3d(10, -10, 1500);

        std::vector<Vec3d> transformedVerts(4);
        for (int i = 0; i < 4; i++)
            multPointMatrix(verts[i], transformedVerts[i], mtxTranslation);

        RenderPoly(transformedVerts, mtxProjection, mtxWorldToCamera, mpGrassDynTexture, &mLight);





        for (int x = 0; x < 10; x++)
        {
            for (int y = 0; y < 10; y++)
            {
                int cubenum = y * 20 + x;
                //        setOrientationMatrix((float)sin(i)*gTimer.GetElapsedTime() / 1050.0, (float)gTimer.GetElapsedTime() / 8000.0, (float)gTimer.GetElapsedTime() / 1000.0, mObjectToWorld);

                setOrientationMatrix(4.0, 0.0, mfBaseAngle + cubenum, mtxRotation);


                //            setTranslationMatrix(0, -cubenum * 2.0, 120 - cubenum * sin(mfBaseAngle), mtxTranslation);
                setTranslationMatrix(-y * 2.0, -x * 2.0, y * 2.0, mtxTranslation);
                mObjectToWorld = mtxRotation * mtxTranslation;



                std::vector<Vec3d> cubeWorldVerts(8);
                for (int i = 0; i < 8; i++)
                    multPointMatrix(mCubeVertices[i], cubeWorldVerts[i], mObjectToWorld);

                for (int i = 0; i < 6; i++)
                {
                    worldVerts[0] = cubeWorldVerts[mSides[i].mSide[0]];
                    worldVerts[1] = cubeWorldVerts[mSides[i].mSide[1]];
                    worldVerts[2] = cubeWorldVerts[mSides[i].mSide[2]];
                    worldVerts[3] = cubeWorldVerts[mSides[i].mSide[3]];

#define RENDER_TEXTURE

#ifdef RENDER_TEXTURE
                    RenderPoly(worldVerts, mtxProjection, mtxWorldToCamera, mpDynTexture, &mLight);
#else
                    RenderPoly(worldVerts, mtxProjection, mtxWorldToCamera, mSides[i].mColor);
#endif
                }
            }
        }
    }

#ifdef RENDER_TEAPOT
    if (mpTeapotRender)
    {
        ZRect rArea(mpTeapotRender.get()->GetArea());
        mpSurface->Blt(mpTeapotRender.get(), rArea, rArea);
    }
#endif

    if (mbRenderSpheres)
    {
        if (mpSpheresRender)
        {
            ZRect rArea(mpSpheresRender.get()->GetArea());

            ZRect rDest(rArea.CenterInRect(mAreaLocal));

            mpSurface->Blt(mpSpheresRender.get(), rArea, rDest);
        }
    }

    ZWin::Paint();

    mbInvalid = true;
    return true;
}
   
bool Z3DTestWin::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();

    if (sType == "updatespherecount")
    {
        UpdateSphereCount();
        return true;
    }
    else if (sType == "updatesettings")
    {
        gRegistry["3dtestwin"]["render_size"] = mnRenderSize;
        gRegistry["3dtestwin"]["render_cubes"] = mbRenderCube;
        gRegistry["3dtestwin"]["render_spheres"] = mbRenderSpheres;

        gRegistry["3dtestwin"]["outersphere"] = mbOuterSphere;
        gRegistry["3dtestwin"]["centersphere"] = mbCenterSphere;
        gRegistry["3dtestwin"]["rotatespeed"] = mnRotateSpeed;
        gRegistry["3dtestwin"]["raydepth"] = mnRayDepth;

        
        

        return true;
    }

    return ZWin::HandleMessage(message);
}
