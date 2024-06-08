#pragma once

#include "ZTypes.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZTimer.h"
#include "ZTypes.h"
#include "ZRandom.h"

struct Particle
{
    Particle(double _mass = 0, ZFPoint _pos = {}, ZFPoint _dir = {}, uint32_t _col = (uint32_t)RANDU64(0xff000000, 0xffffffff)) : mass(_mass), pos(_pos), dir(_dir), col(_col) {}

    bool active = true;

    uint32_t col;
    double mass = 0;
    ZFPoint pos;
    ZFPoint dir;
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

   bool     OnParentAreaChange();
 

protected:
    void    SpawnParticle(int64_t index);
    void    DrawParticle(ZBuffer* pDest, int64_t index);
    ZFPoint GetForceVector(Particle* p);  
   tParticleArray particles;

   tRectArray prevFrameRects;

};
