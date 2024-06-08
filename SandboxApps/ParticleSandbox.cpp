#include "ParticleSandbox.h"

const int64_t kMaxParticles = 5;

ParticleSandbox::ParticleSandbox()
{
    mbAcceptsFocus = true;
    mbAcceptsCursorMessages = true;

}
   
bool ParticleSandbox::Init()
{
    particles.resize(kMaxParticles);
    prevFrameRects.resize(kMaxParticles);

    return ZWin::Init();
}

void ParticleSandbox::SpawnParticle(int64_t index)
{
    particles[index].mass = RANDEXP_DOUBLE(10.0, 200.0);
    particles[index].dir.x = RANDDOUBLE((double)-2.0, (double)2.0);
    particles[index].dir.y = RANDDOUBLE((double)-2.0, (double)2.0);
//    particles[index].dir.x = 0;
//    particles[index].dir.y = 0;
    particles[index].pos.x = RANDU64(mAreaLocal.Width()/4, mAreaLocal.Width()*3/4);
    particles[index].pos.y = RANDU64(mAreaLocal.Height()/4, mAreaLocal.Height()*3/4);
    particles[index].col = RANDU64(0xff000000, 0xffffffff);
    particles[index].active = true;
}

ZFPoint ParticleSandbox::GetForceVector(Particle* p)
{
    ZFPoint force;
    for (const auto& p2 : particles)
    {
        if (p2.active && &p2 != p)
        {
            double fDist = sqrt((p2.pos.x - p->pos.x) * (p2.pos.x - p->pos.x) + (p2.pos.y - p->pos.y) * (p2.pos.y - p->pos.y));

            if (fDist > 0)
            {
                double fForce = p2.mass / (fDist*fDist);

                force.x += ((p2.pos.x - p->pos.x) / 10.0) * fForce;
                force.y += ((p2.pos.y - p->pos.y) / 10.0) * fForce;
            }
        }
    }

    return force;
}


bool ParticleSandbox::Process()
{
    const double kFriction = 0.00;
    for (auto& p : particles)
    {
        ZFPoint force = GetForceVector(&p);
        p.dir.x += force.x * (1.0 - kFriction);
        p.dir.y += force.y * (1.0 - kFriction);
    }

    for (int64_t i = 0; i < particles.size(); i++)
    {
        Particle* p = &particles[i];
        if (p->active /*&& !(p->dir.x == 0 && p->dir.y == 0)*/ )
        {
            p->pos += p->dir;
            if (p->pos.x < -mAreaLocal.Width()*1.5 || p->pos.x > mAreaLocal.Width()*1.5 ||
                p->pos.y < -mAreaLocal.Height()*1.5 || p->pos.y > mAreaLocal.Height()*1.5)
                p->active = false;
        }
        else
        {
            SpawnParticle(i);
        }
    }

    Invalidate();
    return ZWin::Process();
}

bool ParticleSandbox::Paint()
{
    if (!PrePaintCheck())
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

    // erase drawn particles
    for (int64_t i = 0; i < particles.size(); i++)
    {
        mpSurface->FillAlpha(0xee000000, &prevFrameRects[i]);
    }

    for (int64_t i = 0; i < particles.size(); i++)
    {
        DrawParticle(mpSurface.get(), i);
    }

    Invalidate();
    return ZWin::Paint();
}

void ParticleSandbox::DrawParticle(ZBuffer* pDest, int64_t index)
{
    Particle& p = particles[index];
    int64_t nSize = p.mass / 10;
    ZRect r(p.pos.x - nSize, p.pos.y - nSize, p.pos.x + nSize, p.pos.y + nSize);
    pDest->FillAlpha(p.col, &r);
    prevFrameRects[index].SetRect(r);
}


bool ParticleSandbox::OnKeyDown(uint32_t key)
{
        switch (key)
        {
        case VK_ESCAPE:
            gMessageSystem.Post("{quit_app_confirmed}");
            break;
        case ' ':
            for (auto& p : particles)
                p.active = false;
            break;
        }
    return ZWin::OnKeyDown(key);
}

bool ParticleSandbox::OnParentAreaChange()
{
    return ZWin::OnParentAreaChange();
}
 
