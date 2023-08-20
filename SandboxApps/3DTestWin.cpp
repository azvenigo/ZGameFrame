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

using namespace std;
using namespace Z3D;



//#define RENDER_TEAPOT

#ifdef RENDER_TEAPOT
#include "teapotdata.h"
#endif


inline
float clamp(const float& lo, const float& hi, const float& v)
{
    return std::max<float>(lo, std::min<float>(hi, v));
}


#ifdef RENDER_TEAPOT

static const float kInfinity = FLT_MAX;
static const float kEpsilon = 1e-8;
static const Vec3f kDefaultBackgroundColor = Vec3f(0.235294, 0.67451, 0.843137);
template <> const Matrix44f Matrix44f::kIdentity = Matrix44f();

inline
float deg2rad(const float& deg)
{
    return deg * M_PI / 180;
}

inline
Vec3f mix(const Vec3f& a, const Vec3f& b, const float& mixValue)
{
    return a * (1 - mixValue) + b * mixValue;
}

struct Options
{
    uint32_t width = 640;
    uint32_t height = 480;
    float fov = 90;
    Vec3f backgroundColor = kDefaultBackgroundColor;
    Matrix44f cameraToWorld;
    float bias = 0.0001;
    uint32_t maxDepth = 2;
};

enum MaterialType { kDiffuse, kNothing };

class Object
{
public:
    // [comment]
    // Setting up the object-to-world and world-to-object matrix
    // [/comment]
    Object(const Matrix44f& o2w) : objectToWorld(o2w), worldToObject(o2w.inverse()) {}
    virtual ~Object() {}
    virtual bool intersect(const Vec3f&, const Vec3f&, float&, uint32_t&, Vec2f&) const = 0;
    virtual void getSurfaceProperties(const Vec3f&, const Vec3f&, const uint32_t&, const Vec2f&, Vec3f&, Vec2f&) const = 0;
    virtual void displayInfo() const = 0;
    Matrix44f objectToWorld, worldToObject;
    MaterialType type = kDiffuse;
    Vec3f albedo = 0.18;
    float Kd = 0.8; // phong model diffuse weight
    float Ks = 0.2; // phong model specular weight
    float n = 10;   // phong specular exponent
    Vec3f BBox[2] = { kInfinity, -kInfinity };
};

bool rayTriangleIntersect(
    const Vec3f& orig, const Vec3f& dir,
    const Vec3f& v0, const Vec3f& v1, const Vec3f& v2,
    float& t, float& u, float& v)
{
    Vec3f v0v1 = v1 - v0;
    Vec3f v0v2 = v2 - v0;
    Vec3f pvec = dir.crossProduct(v0v2);
    float det = v0v1.dotProduct(pvec);

    // ray and triangle are parallel if det is close to 0
    if (fabs(det) < kEpsilon) return false;

    float invDet = 1 / det;

    Vec3f tvec = orig - v0;
    u = tvec.dotProduct(pvec) * invDet;
    if (u < 0 || u > 1) return false;

    Vec3f qvec = tvec.crossProduct(v0v1);
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
        const Matrix44f& o2w,
        uint32_t nfaces,
        std::vector<uint32_t>& faceIndex,
        std::vector<uint32_t>& vertsIndex,
        std::vector<Vec3f>& verts,
        std::vector<Vec3f>& normals,
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
        Matrix44f transformNormals = worldToObject.transpose();
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
    bool intersect(const Vec3f& orig, const Vec3f& dir, float& tNear, uint32_t& triIndex, Vec2f& uv) const
    {
        uint32_t j = 0;
        bool isect = false;
        for (uint32_t i = 0; i < numTris; ++i) {
            const Vec3f& v0 = P[trisIndex[j]];
            const Vec3f& v1 = P[trisIndex[j + 1]];
            const Vec3f& v2 = P[trisIndex[j + 2]];
            float t = kInfinity, u, v;
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
        const Vec3f& hitPoint,
        const Vec3f& viewDirection,
        const uint32_t& triIndex,
        const Vec2f& uv,
        Vec3f& hitNormal,
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
            const Vec3f& n0 = N[vai[0]];
            const Vec3f& n1 = N[vai[1]];
            const Vec3f& n2 = N[vai[2]];
            hitNormal = (1 - uv.x - uv.y) * n0 + uv.x * n1 + uv.y * n2;
        }
        else {
            // face normal
            const Vec3f& v0 = P[trisIndex[triIndex * 3]];
            const Vec3f& v1 = P[trisIndex[triIndex * 3 + 1]];
            const Vec3f& v2 = P[trisIndex[triIndex * 3 + 2]];
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
    std::vector<Vec3f> P;              // triangles vertex position
    std::vector<uint32_t> trisIndex;   // vertex index array
    std::vector<Vec3f> N;              // triangles vertex normals
    std::vector<Vec2f> texCoordinates; // triangles texture coordinates
    bool smoothShading = true;                // smooth shading by default
    bool isSingleVertAttr = true;
};

class Light
{
public:
    Light(const Matrix44f& l2w, const Vec3f& c = 1, const float& i = 1) : lightToWorld(l2w), color(c), intensity(i) {}
    virtual ~Light() {}
    virtual void illuminate(const Vec3f& P, Vec3f&, Vec3f&, float&) const {};
    Vec3f color;
    float intensity;
    Matrix44f lightToWorld;
};

struct IsectInfo
{
    const Object* hitObject = nullptr;
    float tNear = kInfinity;
    Vec2f uv;
    uint32_t index = 0;
};

Vec3f castRay(
    Vec3f& orig, const Vec3f& dir,
    std::vector<Object*>& objects,
    std::vector<Light>& lights,
    Options& options,
    uint32_t depth = 0)
{
    if (depth > options.maxDepth) return 0;
    Vec3f hitColor = 0;
    IsectInfo isect;
    if (trace(orig, dir, objects, isect)) {
        Vec3f hitPoint = orig + dir * isect.tNear;
        Vec3f hitNormal;
        Vec2f hitTexCoordinates;
        isect.hitObject->getSurfaceProperties(hitPoint, dir, isect.index, isect.uv, hitNormal, hitTexCoordinates);

        hitColor = std::max<float>(0.f, -hitNormal.dotProduct(dir));//* Vec3f(hitTexCoordinates.x, hitTexCoordinates.y, 1);
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
    float scale = tan(deg2rad(options.fov * 0.5));
    float imageAspectRatio = options.width / (float)options.height;
    Vec3f orig;
    options.cameraToWorld.multVecMatrix(Vec3f(0), orig);
    auto timeStart = std::chrono::high_resolution_clock::now();
    float gamma = 1;

    ThreadPool pool(std::thread::hardware_concurrency());

    vector<shared_future<Vec3f> > pixelResults;



    for (uint32_t j = 0; j < options.height; ++j) {
        for (uint32_t i = 0; i < options.width; ++i) {
            // generate primary ray direction
            float x = (2 * (i + 0.5) / (float)options.width - 1) * imageAspectRatio * scale;
            float y = (1 - 2 * (j + 0.5) / (float)options.height) * scale;
            Vec3f dir;
            options.cameraToWorld.multDirMatrix(Vec3f(x, y, -1), dir);
            dir.normalize();

//            Vec3f pixel = castRay(orig, dir, objects, lights, options);

            pixelResults.emplace_back(pool.enqueue(&castRay, orig, dir, objects, lights, options, 0));
        }
    }

    vector<shared_future<Vec3f> >::iterator it = pixelResults.begin();
    for (uint32_t j = 0; j < options.height; ++j) {
        for (uint32_t i = 0; i < options.width; ++i) {

            Vec3f pixel = (*it).get();

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
Vec3f evalBezierCurve(const Vec3f* P, const float& t)
{
    float b0 = (1 - t) * (1 - t) * (1 - t);
    float b1 = 3 * t * (1 - t) * (1 - t);
    float b2 = 3 * t * t * (1 - t);
    float b3 = t * t * t;

    return P[0] * b0 + P[1] * b1 + P[2] * b2 + P[3] * b3;
}

Vec3f evalBezierPatch(const Vec3f* controlPoints, const float& u, const float& v)
{
    Vec3f uCurve[4];
    for (int i = 0; i < 4; ++i)
        uCurve[i] = evalBezierCurve(controlPoints + 4 * i, u);

    return evalBezierCurve(uCurve, v);
}

Vec3f derivBezier(const Vec3f* P, const float& t)
{
    return -3 * (1 - t) * (1 - t) * P[0] +
        (3 * (1 - t) * (1 - t) - 6 * t * (1 - t)) * P[1] +
        (6 * t * (1 - t) - 3 * t * t) * P[2] +
        3 * t * t * P[3];
}

// [comment]
// Compute the derivative of a point on Bezier patch along the u parametric direction
// [/comment]
Vec3f dUBezier(const Vec3f* controlPoints, const float& u, const float& v)
{
    Vec3f P[4];
    Vec3f vCurve[4];
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
Vec3f dVBezier(const Vec3f* controlPoints, const float& u, const float& v)
{
    Vec3f uCurve[4];
    for (int i = 0; i < 4; ++i) {
        uCurve[i] = evalBezierCurve(controlPoints + 4 * i, u);
    }

    return derivBezier(uCurve, v);
}

// [comment]
// Generate a poly-mesh Utah teapot out of Bezier patches
// [/comment]
void createPolyTeapot(Matrix44f& o2w, std::vector<Object*>& objects)
{
    uint32_t divs = 8;
    std::vector<Vec3f> P; P.resize((divs + 1) * (divs + 1));
    std::vector<uint32_t> nvertices; nvertices.resize(divs * divs);
    std::vector<uint32_t> vertices;vertices.resize(divs * divs * 4);
    std::vector<Vec3f> N; N.resize((divs + 1) * (divs + 1));
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

    Vec3f controlPoints[16];
    for (int np = 0; np < kTeapotNumPatches; ++np) { // kTeapotNumPatches
        // set the control points for the current patch
        for (uint32_t i = 0; i < 16; ++i)
            controlPoints[i][0] = teapotVertices[teapotPatches[np][i] - 1][0],
            controlPoints[i][1] = teapotVertices[teapotPatches[np][i] - 1][1],
            controlPoints[i][2] = teapotVertices[teapotPatches[np][i] - 1][2];

        // generate grid
        for (uint16_t j = 0, k = 0; j <= divs; ++j) {
            float v = j / (float)divs;
            for (uint16_t i = 0; i <= divs; ++i, ++k) {
                float u = i / (float)divs;
                P[k] = evalBezierPatch(controlPoints, u, v);
                Vec3f dU = dUBezier(controlPoints, u, v);
                Vec3f dV = dVBezier(controlPoints, u, v);
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
Vec3f curveData[curveNumPts] = {
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
    Vec3f pts[4];
    std::vector<Vec3f> P; P.resize((ndivs + 1) * ndivs * ncurves + 1);
    std::vector<Vec3f> N; N.resize((ndivs + 1) * ndivs * ncurves + 1);
    std::vector<Vec2f> st; st.resize((ndivs + 1) * ndivs * ncurves + 1);
    for (uint32_t i = 0; i < ncurves; ++i) {
        for (uint32_t j = 0; j < ndivs; ++j) {
            pts[0] = curveData[i * 3];
            pts[1] = curveData[i * 3 + 1];
            pts[2] = curveData[i * 3 + 2];
            pts[3] = curveData[i * 3 + 3];
            float s = j / (float)ndivs;
            Vec3f pt = evalBezierCurve(pts, s);
            Vec3f tangent = derivBezier(pts, s).normalize();
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

            Vec3f up, forward, right;

            switch (maxAxis) {
            case 0:
            case 1:
                up = tangent;
                forward = Vec3f(0, 0, 1);
                right = up.crossProduct(forward);
                forward = right.crossProduct(up);
                break;
            case 2:
                up = tangent;
                right = Vec3f(0, 0, 1);
                forward = right.crossProduct(up);
                right = up.crossProduct(forward);
                break;
            default:
                break;
            };

            float sNormalized = (i * ndivs + j) / float(ndivs * ncurves);
            float rad = 0.1 * (1 - sNormalized);
            for (uint32_t k = 0; k <= ndivs; ++k) {
                float t = k / (float)ndivs;
                float theta = t * 2 * M_PI;
                Vec3f pc(cos(theta) * rad, 0, sin(theta) * rad);
                float x = pc.x * right.x + pc.y * up.x + pc.z * forward.x;
                float y = pc.x * right.y + pc.y * up.y + pc.z * forward.y;
                float z = pc.x * right.z + pc.y * up.z + pc.z * forward.z;
                P[i * (ndivs + 1) * ndivs + j * (ndivs + 1) + k] = Vec3f(pt.x + x, pt.y + y, pt.z + z);
                N[i * (ndivs + 1) * ndivs + j * (ndivs + 1) + k] = Vec3f(x, y, z).normalize();
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

    objects.emplace_back(new TriangleMesh(Matrix44f::kIdentity, numFaces, verts, vertIndices, P, N, st));
}


void Z3DTestWin::RenderTeapot()
{
    // loading gemetry
    std::vector<Object*> objects;

    Matrix44f teapotO2W;
    //    Z3D::LookAt(Vec3f(4, 5, -10), Vec3f(0, 0, 0), Vec3f(0, 10, 0), teapotO2W);
    setOrientationMatrix((float)gTimer.GetElapsedTime() / 1000.0, (float)gTimer.GetElapsedTime() / 2500.0, 0.0, teapotO2W);
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
    //options.cameraToWorld = Matrix44f(0.897258, 0, -0.441506, 0, -0.288129, 0.757698, -0.585556, 0, 0.334528, 0.652606, 0.679851, 0, 5.439442, 11.080794, 10.381341, 1);
    Z3D::LookAt(Vec3f(4, 5, -10), Vec3f(0, 0, 0), Vec3f(0, 10, 0), options.cameraToWorld);


    // to render the curve as geometry
    //options.cameraToWorld = Matrix44f(0.707107, 0, -0.707107, 0, -0.369866, 0.85229, -0.369866, 0, 0.60266, 0.523069, 0.60266, 0, 2.634, 3.178036, 2.262122, 1);

    // finally, render
    render(options, objects, lights, mpTeapotRender);
    for (auto o : objects)
        delete o;
}


bool trace(
    Vec3f& orig, const Vec3f& dir,
    std::vector<Object*>& objects,
    IsectInfo& isect)
{
    isect.hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearTriangle = kInfinity;
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
        const Vec3f& center,
        const float& radius,
        const Vec3f& surfaceColor,
        const float& reflection = 0,
        const float& transparency = 0,
        const Vec3f& emmisionColor = 0) :
        mCenter(center), mRadius(radius), mRadius2(radius* radius), mSurfaceColor(surfaceColor), mEmissionColor(emmisionColor),
        mTransparency(transparency), mReflection(reflection)
    {
    }

    //[comment]
    // Compute a ray-sphere intersection using the geometric solution
    //[/comment]
    bool intersect(const Vec3f& rayorig, const Vec3f& raydir, float& t0, float& t1) const
    {
        Vec3f l = mCenter - rayorig;
        float tca = l.dotProduct(raydir);
        if (tca < 0) return false;
        float d2 = l.dotProduct(l) - tca * tca;
        if (d2 > mRadius2) return false;
        float thc = sqrt(mRadius2 - d2);
        t0 = tca - thc;
        t1 = tca + thc;

        return true;
    }


    Vec3f mCenter;                           /// position of the sphere
    float mRadius, mRadius2;                  /// sphere radius and radius^2
    Vec3f mSurfaceColor, mEmissionColor;      /// surface color and emission (light)
    float mTransparency, mReflection;         /// surface transparency and reflectivity

};

//#define MAX_RAY_DEPTH 5

float mix(const float& a, const float& b, const float& mix)
{
    return b * mix + a * (1 - mix);
}

Vec3f TraceSpheres(
    const Vec3f& rayorig,
    const Vec3f& raydir,
    const int& depthRemaining, const std::vector<class Sphere>& spheres)
{
    //if (raydir.length() != 1) std::cerr << "Error " << raydir << std::endl;
    float tnear = INFINITY;
    const Sphere* sphere = NULL;
    // find intersection of this ray with the sphere in the scene
    for (unsigned i = 0; i < spheres.size(); ++i) {
        float t0 = INFINITY, t1 = INFINITY;
        if (spheres[i].intersect(rayorig, raydir, t0, t1)) {
            if (t0 < 0) t0 = t1;
            if (t0 < tnear) {
                tnear = t0;
                sphere = &spheres[i];
            }
        }
    }
    // if there's no intersection return black or background color
    if (!sphere) return Vec3f(2);
    Vec3f surfaceColor = 0; // color of the ray/surfaceof the object intersected by the ray
    Vec3f phit = rayorig + raydir * tnear; // point of intersection
    Vec3f nhit = phit - sphere->mCenter; // normal at the intersection point
    nhit.normalize(); // normalize normal direction
    // If the normal and the view direction are not opposite to each other
    // reverse the normal direction. That also means we are inside the sphere so set
    // the inside bool to true. Finally reverse the sign of IdotN which we want
    // positive.
    float bias = (float)1e-4; // add some bias to the point from which we will be tracing
    bool inside = false;
    if (raydir.dotProduct(nhit) > 0) nhit = -nhit, inside = true;
    if ((sphere->mTransparency > 0 || sphere->mReflection > 0) && depthRemaining > 0) {
        float facingratio = -raydir.dotProduct(nhit);
        // change the mix value to tweak the effect
        float fresneleffect = mix(pow(1 - facingratio, 3), 1, 0.1f);
        // compute reflection direction (not need to normalize because all vectors
        // are already normalized)
        Vec3f refldir = raydir - nhit * 2 * raydir.dotProduct(nhit);
        refldir.normalize();
        Vec3f reflection = TraceSpheres(phit + nhit * bias, refldir, depthRemaining-1, spheres);
        Vec3f refraction = 0;
        // if the sphere is also transparent compute refraction ray (transmission)
        if (sphere->mTransparency) {
            float ior = 1.1, eta = (inside) ? ior : 1 / ior; // are we inside or outside the surface?
            float cosi = -nhit.dotProduct(raydir);
            float k = 1 - eta * eta * (1 - cosi * cosi);
            Vec3f refrdir = raydir * eta + nhit * (eta * cosi - sqrt(k));
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
                Vec3f transmission = 1;
                Vec3f lightDirection = spheres[i].mCenter - phit;
                lightDirection.normalize();
                for (unsigned j = 0; j < spheres.size(); ++j) {
                    if (i != j) {
                        float t0, t1;
                        if (spheres[j].intersect(phit + nhit * bias, lightDirection, t0, t1)) {
                            transmission = 0;
                            break;
                        }
                    }
                }
                surfaceColor += sphere->mSurfaceColor * transmission *
                    std::max<float>(float(0), nhit.dotProduct(lightDirection)) * spheres[i].mEmissionColor;
            }
        }
    }

    return surfaceColor + sphere->mEmissionColor;
}



void ThreadTrace(ZRect rArea, std::vector<class Sphere>& spheres, ZBuffer* pDest, int64_t nDepth)
{
    ZRect rFullArea(pDest->GetArea());
    float invWidth = 1 / float(rFullArea.Width()), invHeight = 1 / float(rFullArea.Height());
    float fov = 30, aspectratio = rFullArea.Width() / float(rFullArea.Height());
    float angle = tan( M_PI * 0.5 * fov / 180.);

    for (unsigned y = rArea.top; y < rArea.bottom; ++y) {
        for (unsigned x = rArea.left; x < rArea.right; ++x) {
            float xx = ( 2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
            float yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;
            Vec3f raydir(xx, yy, -1);
            raydir.normalize();
            Vec3f pixel = TraceSpheres(Vec3f(0), raydir, nDepth, spheres);

            char r = (char)(255 * clamp(0, 1, pixel.x));
            char g = (char)(255 * clamp(0, 1, pixel.y));
            char b = (char)(255 * clamp(0, 1, pixel.z));
            pDest->SetPixel(x, y, ARGB(0xff, r, g, b));
        }
    }
}

//[comment]
// Main rendering function. We compute a camera ray for each pixel of the image
// trace it and return a color. If the ray hits a sphere, we return the color of the
// sphere at the intersection point, else we return the background color.
//[/comment]
void Z3DTestWin::RenderSpheres(tZBufferPtr mpSurface)
{
    uint32_t width = mnRenderSize;
    uint32_t height = mnRenderSize;
    mpSurface->Init(width, height);


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

        workers.emplace_back(std::thread(ThreadTrace, ZRect(0, nTop, width, nBottom), mSpheres, mpSurface.get(), mnRayDepth));

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
            Vec3f raydir(xx, yy, -1);
            raydir.normalize();
            Vec3f pixel = TraceSpheres(Vec3f(0), raydir, 0);

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
    mnRotateSpeed = 10;
    mnRayDepth = 3;
    mfBaseAngle = 0.0;

    mbRenderCube = false;
    mbRenderSpheres = true;
    mbOuterSphere = true;
    mbCenterSphere = true;
    mnRenderSize = 256;
    mbControlPanelEnabled = true;

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


    mCubeVertices.resize(8);

    const float f = 1.0;

    mCubeVertices[0] = Vec3f(-f,  f, -f);
    mCubeVertices[1] = Vec3f( f,  f, -f);
    mCubeVertices[2] = Vec3f( f, -f, -f);
    mCubeVertices[3] = Vec3f(-f, -f, -f);

    mCubeVertices[4] = Vec3f(-f,  f,  f);
    mCubeVertices[5] = Vec3f( f,  f,  f);
    mCubeVertices[6] = Vec3f( f, -f,  f);
    mCubeVertices[7] = Vec3f(-f, -f,  f);

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

        pCP->AddCaption("Sphere Count");
        pCP->AddSlider(&mnTargetSphereCount, 1, 50, 1, 0.25, sUpdateSphereCountMsg, true, false, gStyleButton.Font());
        //    pCP->AddSpace(16);

        pCP->AddCaption("Min Sphere Size");
        pCP->AddSlider(&mnMinSphereSizeTimes100, kDefaultMinSphereSize, kDefaultMaxSphereSize, 1, 0.25, sUpdateSphereCountMsg, true, false, gStyleButton.Font());

        pCP->AddCaption("Max Sphere Size");
        pCP->AddSlider(&mnMaxSphereSizeTimes100, kDefaultMinSphereSize, kDefaultMaxSphereSize, 1, 0.25, sUpdateSphereCountMsg, true, false, gStyleButton.Font());

        pCP->AddCaption("Speed");
        pCP->AddSlider(&mnRotateSpeed, 0, 100, 1, 0.25, "", true, false, gStyleButton.Font());

        pCP->AddCaption("Ray Depth");
        pCP->AddSlider(&mnRayDepth, 0, 10, 1, 0.25, "", true, false, gStyleButton.Font());



        pCP->AddSpace(16);
        pCP->AddCaption("Render Size");
        pCP->AddSlider(&mnRenderSize, 1, 128, 16, 0.25, ZMessage("updaterendersize", this), true);

        ZGUI::ZTextLook toggleLook(ZGUI::ZTextLook::kEmbossed, 0xff737373, 0xff737373);

        pCP->AddSpace(16);

        ZWinCheck* pToggle;

        pToggle = pCP->AddToggle(&mbRenderCube, "Render Cube");
        pToggle->msWinGroup = "rendermode";

        pToggle = pCP->AddToggle(&mbRenderSpheres, "Render Spheres");
        pToggle->msWinGroup = "rendermode";

        pCP->AddToggle(&mbOuterSphere, "Outer Sphere", sUpdateSphereCountMsg, sUpdateSphereCountMsg);
        pCP->AddToggle(&mbCenterSphere, "Center Sphere", sUpdateSphereCountMsg, sUpdateSphereCountMsg);

        ChildAdd(pCP);
    }
    else
    {
        mnRenderSize = mAreaToDrawTo.Height() / 2;
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

void Z3DTestWin::UpdateSphereCount()
{
    ZASSERT(mnTargetSphereCount > 0 && mnTargetSphereCount < 10000);

    int64_t nCount = mnTargetSphereCount + mbOuterSphere + mbCenterSphere;
    mSpheres.resize(nCount);
    int i = 0;
    if (mbOuterSphere)
    {
        mSpheres[i] = Sphere(Vec3f(0, 0, -50), 500, Vec3f(0, 0, 0), 0.0, 0.0, Vec3f(0,0,0));   
        i++;
    }

    if (mbCenterSphere)
    {
        mSpheres[i] = Sphere(Vec3f(0, 0, -50), 5, Vec3f(0.5, 0.5, 0.5), 0.5, 0.5, Vec3f(0, 0, 0)); 
        i++;
    }

    for (;i < mnTargetSphereCount; i++)
    {
        float fMaxDist = 100.0;
        float fRadius = RANDDOUBLE( (mnMinSphereSizeTimes100 / 100.0), (mnMaxSphereSizeTimes100 / 100.0));

        Vec3f center(fMaxDist * sin(RANDDOUBLE(0.0, (2.0 * M_PI))),
                     fMaxDist * sin(RANDDOUBLE(0.0, (2.0 * M_PI))), 
                     fMaxDist * sin(RANDDOUBLE(0.0, (2.0 * M_PI))));

        Vec3f color(RANDDOUBLE(0.0, 1.0), RANDDOUBLE(0.0, 1.0), RANDDOUBLE(0.0, 1.0));
        float fReflective(RANDDOUBLE(0.0, 1.0));
        float fTransparent(RANDDOUBLE(0.0, 1.0));
        Vec3f emmisionColor(0);

        if (RANDPERCENT(10))    // 10% of spheres will be emitters
            emmisionColor = Vec3f(RANDDOUBLE(0.0, 1.0), RANDDOUBLE(0.0, 1.0), RANDDOUBLE(0.0, 1.0));

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
        gMessageSystem.Post("quit_app_confirmed");
        break;
    }
#endif
    return ZWin::OnChar(key);
}



void multPointMatrix(const Vec3f& in, Vec3f& out, const Matrix44f& M)
{
    //out = in * M;
    out.x = in.x * M[0][0] + in.y * M[1][0] + in.z * M[2][0] + /* in.z = 1 */ M[3][0];
    out.y = in.x * M[0][1] + in.y * M[1][1] + in.z * M[2][1] + /* in.z = 1 */ M[3][1];
    out.z = in.x * M[0][2] + in.y * M[1][2] + in.z * M[2][2] + /* in.z = 1 */ M[3][2];
    float w = in.x * M[0][3] + in.y * M[1][3] + in.z * M[2][3] + /* in.z = 1 */ M[3][3];

    // normalize if w is different than 1 (convert from homogeneous to Cartesian coordinates)
    if (w != 1) {
        out.x /= w;
        out.y /= w;
        out.z /= w;
    }
}




bool Z3DTestWin::Process()
{
    return true;
}

void Z3DTestWin::RenderPoly(vector<Vec3f>& worldVerts, Matrix44f& mtxProjection, Matrix44f& mtxWorldToCamera, uint32_t nCol)
{
    tColorVertexArray screenVerts;
    screenVerts.resize(worldVerts.size());


/*    Vec3f faceX(worldVerts[0].x - worldVerts[1].x, worldVerts[0].y - worldVerts[1].y, 1);
    Vec3f faceY(worldVerts[0].x - worldVerts[3].x, worldVerts[0].y - worldVerts[3].y, 1);
    Vec3f faceNormal = faceX.cross(faceY);
    faceNormal.normalize();
    Vec3f lightDirection(5, 5, 1);
    lightDirection.normalize();

    double fScale = lightDirection.dot(-faceNormal);
    if (fScale < 0)
        fScale = 0;
    if (fScale > 1.0)
        fScale = 1.0;

    double fR = ARGB_R(nCol) * fScale;
    double fG = ARGB_G(nCol) * fScale;
    double fB = ARGB_B(nCol) * fScale;

    nCol = ARGB(0xff, (uint8_t)fR, (uint8_t)fG, (uint8_t)fB);
    */


    for (int i = 0; i < worldVerts.size(); i++)
    {
        Vec3f v = worldVerts[i];

        Vec3f vertCamera;
        Vec3f vertProjected;

        multPointMatrix(v, vertCamera, mtxWorldToCamera);
        multPointMatrix(vertCamera, vertProjected, mtxProjection);

/*        if (vertProjected.x < -1 || vertProjected.x > 1 || vertProjected.y < -1 || vertProjected.y > 1)
            continue;*/

        screenVerts[i].x = mAreaToDrawTo.Width()/2 + (int64_t)(vertProjected.x * mnRenderSize *10);
        screenVerts[i].y = mAreaToDrawTo.Height()/2 + (int64_t)(vertProjected.y * mnRenderSize *10);

        screenVerts[i].mColor = nCol;
    }

    Vec3f planeX(screenVerts[1].x- screenVerts[0].x, screenVerts[1].y - screenVerts[0].y, 1);
    Vec3f planeY(screenVerts[0].x - screenVerts[3].x, screenVerts[0].y - screenVerts[3].y, 1);
    Vec3f normal = planeX.crossProduct(planeY);
    if (normal.z > 0)
        return;

    gRasterizer.Rasterize(mpTransformTexture.get(), screenVerts);
}

void Z3DTestWin::RenderPoly(vector<Vec3f>& worldVerts, Matrix44f& mtxProjection, Matrix44f& mtxWorldToCamera, tZBufferPtr pTexture)
{
    tUVVertexArray screenVerts;
    screenVerts.resize(worldVerts.size());

    for (int i = 0; i < worldVerts.size(); i++)
    {
        Vec3f v = worldVerts[i];

        Vec3f vertCamera;
        Vec3f vertProjected;

        multPointMatrix(v, vertCamera, mtxWorldToCamera);
        multPointMatrix(vertCamera, vertProjected, mtxProjection);

        /*        if (vertProjected.x < -1 || vertProjected.x > 1 || vertProjected.y < -1 || vertProjected.y > 1)
                    continue;*/

        screenVerts[i].x = mAreaToDrawTo.Width() / 2 + (int64_t)(vertProjected.x * mnRenderSize * 10);
        screenVerts[i].y = mAreaToDrawTo.Height() / 2 + (int64_t)(vertProjected.y * mnRenderSize * 10);
    }

    Vec3f planeX(screenVerts[1].x - screenVerts[0].x, screenVerts[1].y - screenVerts[0].y, 1);
    Vec3f planeY(screenVerts[0].x - screenVerts[3].x, screenVerts[0].y - screenVerts[3].y, 1);
    Vec3f normal = planeX.crossProduct(planeY);
    if (normal.z > 0)
        return;


    screenVerts[0].u = 0.0;
    screenVerts[0].v = 0.0;

    screenVerts[1].u = 1.0;
    screenVerts[1].v = 0.0;

    screenVerts[2].u = 1.0;
    screenVerts[2].v = 1.0;

    screenVerts[3].u = 0.0;
    screenVerts[3].v = 1.0;

    gRasterizer.Rasterize(mpTransformTexture.get(), pTexture.get(), screenVerts);
}




bool Z3DTestWin::Paint()
{
    if (!mbInvalid)
        return false;

#ifdef RENDER_TEAPOT
    RenderTeapot();
#endif

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    mpTransformTexture->FillAlpha(mAreaToDrawTo, 0xff000000);

    mfBaseAngle += (mnRotateSpeed / 1000.0) * gTimer.GetElapsedTime() / 10000.0;

    if (mbRenderSpheres)
    {
        float fAngle = mfBaseAngle;

        int nStartSphere = 0;
        if (mbCenterSphere)
            nStartSphere++;
        if (mbOuterSphere)
            nStartSphere++;
        for (int i = nStartSphere; i < mSpheres.size(); i++)
        {
            mSpheres[i].mCenter = Vec3f(5 * cos(fAngle * mSpheres[i].mSurfaceColor.length()), 5.0 * sin(fAngle * mSpheres[i].mTransparency), -50+10.0 * sin(fAngle * mSpheres[i].mReflection));
            fAngle += 1.2;
        }

        RenderSpheres(mpSpheresRender);
    }


    uint64_t nTime = gTimer.GetMSSinceEpoch();
    string sTime;
    Sprintf(sTime, "fps: %f", 1000.0 / (nTime - mLastTimeStamp));
    gpFontSystem->GetDefaultFont()->DrawText(mpTransformTexture.get(), sTime, mAreaToDrawTo);
    mLastTimeStamp = nTime;

    if (mbRenderCube)
    {
        Matrix44f mtxProjection;
        Matrix44f mtxWorldToCamera;

        //    Z3D::LookAt(Vec3f(10*sin(gTimer.GetMSSinceEpoch() / 1000.0), 0, -10-10*cos(gTimer.GetMSSinceEpoch()/1000.0)), Vec3f(0, 0, 0), Vec3f(0, 1, 0), mtxWorldToCamera);
        //    Z3D::LookAt(Vec3f(0, 1, -20 - 10 * cos(gTimer.GetMSSinceEpoch() / 1000.0)), Vec3f(0, 0, 0), Vec3f(0, 10, 0), mtxWorldToCamera);
        Z3D::LookAt(Vec3f(0, 1, -20), Vec3f(0, 0, 0), Vec3f(0, 10, 0), mtxWorldToCamera);

        double fFoV = 90;
        double fNear = 0.1;
        double fFar = 100;

        setProjectionMatrix(fFoV, fNear, fFar, mtxProjection);


        vector<Vec3f> worldVerts;
        worldVerts.resize(4);

        int i = 1;
        //    for (; i < 100; i++)
        {
            //        setOrientationMatrix((float)sin(i)*gTimer.GetElapsedTime() / 1050.0, (float)gTimer.GetElapsedTime() / 8000.0, (float)gTimer.GetElapsedTime() / 1000.0, mObjectToWorld);
            setOrientationMatrix(4.0, 0.0, mfBaseAngle, mObjectToWorld);

            std::vector<Vec3f> cubeWorldVerts;
            cubeWorldVerts.resize(8);
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
                RenderPoly(worldVerts, mtxProjection, mtxWorldToCamera, mpTexture);
#else
                RenderPoly(worldVerts, mtxProjection, mtxWorldToCamera, mSides[i].mColor);
#endif
            }
        }

    }

#ifdef RENDER_TEAPOT
    if (mpTeapotRender)
    {
        ZRect rArea(mpTeapotRender.get()->GetArea());
        mpTransformTexture->Blt(mpTeapotRender.get(), rArea, rArea);
    }
#endif

    if (mbRenderSpheres)
    {
        if (mpSpheresRender)
        {
            ZRect rArea(mpSpheresRender.get()->GetArea());

            ZRect rDest(rArea.CenterInRect(mAreaToDrawTo));

            mpTransformTexture->Blt(mpSpheresRender.get(), rArea, rDest);
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
    else if (sType == "updaterendersize")
    {
        gRegistry["3dtestwin"]["render_size"] = mnRenderSize;
        return true;
    }

    return ZWin::HandleMessage(message);
}
