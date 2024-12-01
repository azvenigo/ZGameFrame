#pragma once

#include "ZTypes.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZTimer.h"
#include "ZTypes.h"
#include "ZRandom.h"

class ZWinControlPanel;

struct Particle
{
    Particle(double _mass = 0, ZFPoint _pos = {}, ZFPoint _dir = {}, uint32_t _col = (uint32_t)RANDU64(0xff000000, 0xffffffff)) : mass(_mass), pos(_pos), dir(_dir), col(_col) 
    {
        rendering.reset(new ZBuffer);
    }

    bool Draw(ZBuffer* pDest, Z3D::Vec3f lightPos, Z3D::Vec3f viewPos, Z3D::Vec3f ambient);

    bool active = true;

    uint32_t col;
    double mass = 0;
    ZFPoint pos;
    ZFPoint dir;

    tZBufferPtr rendering;
};

typedef std::vector< Particle> tParticleArray;
typedef std::vector< ZRect > tRectArray;

class ParticleSandbox : public ZWin
{
public:
    ParticleSandbox();
   
   bool		Init();
   bool     Process();
   bool		Paint();
   virtual bool	OnKeyDown(uint32_t key);
   virtual bool OnMouseDownR(int64_t x, int64_t y);

   bool	HandleMessage(const ZMessage& message);

   bool     OnParentAreaChange();
 
   void     InitFromParams();

protected:
    void    DrawArrow(ZFPoint start, ZFPoint end, uint32_t col = 0xffffffff);

    void    SpawnParticle(int64_t index);
    void    CreateGravField();
    void    DrawParticle(ZBuffer* pDest, int64_t index);
//    ZFPoint GetForceVector(Particle* p);  
//    ZFPoint LimitMagnitude(ZFPoint v);

   tParticleArray particles;

   tParticleArray gravField;

   tRectArray prevFrameRects;

   int64_t   mLastTimeStamp;
   ZWinControlPanel* mpControlPanel;



   int64_t mMaxParticles = 200;
   int64_t mTerminalVelocity = 25;

   bool    shiftingGravField = false;
   bool    frameClear = true;
   int64_t gravFieldX = 0;
   int64_t gravFieldY = 0;

   int64_t centerMass = 0;

   int64_t particleMassMin = 1;
   int64_t particleMassMax = 100;


   int64_t Attenuation = 90;

   Z3D::Vec3f lightPos;
   Z3D::Vec3f viewPos;

   Z3D::Vec3f ambient;


   bool paused = false;

   bool enableCollisions = true;
   bool enableAbsorption = false;
   bool drawArrows = false;
   bool drawGravField = false;

   std::vector<std::thread>  workers;
};
