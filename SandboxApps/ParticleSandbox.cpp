#include "ParticleSandbox.h"
#include "ZTypes.h"
#include <utility>

const int64_t kMaxParticles = 604;

ParticleSandbox::ParticleSandbox()
{
    mbAcceptsFocus = true;
    mbAcceptsCursorMessages = true;

}
   
bool ParticleSandbox::Init()
{
    particles.resize(kMaxParticles);
    prevFrameRects.resize(kMaxParticles);

    for (int i = 0; i < kMaxParticles; i++)
        SpawnParticle(i);

    CreateGravField();

    mLastTimeStamp = gTimer.GetUSSinceEpoch();
    return ZWin::Init();
}

void ParticleSandbox::DrawArrow(ZFPoint start, ZFPoint end, uint32_t col)
{
    double x0 = start.x;
    double y0 = start.y;
    double x1 = end.x;
    double y1 = end.y;
    double dx = end.x - start.x;
    double dy = end.y - start.y;

    double length = sqrt(dx * dx + dy * dy);
    double ux = dx / length;
    double uy = dy / length;



    double shaft_width = std::min<double>(length / 10.0, 8);
    double head_length = length / 4;

    double shaft_length = length - head_length;
    double shaftratio = shaft_length / length;

    double shaftx1 = start.x + dx * shaftratio;
    double shafty1 = start.y + dy * shaftratio;

    double head_width = head_length*0.66 ;


    tColorVertexArray shaft;
    shaft.resize(4);

    // Compute the rectangle points for the shaft
    shaft[0].x = x0 - uy * shaft_width / 2;
    shaft[0].y = y0 + ux * shaft_width / 2;
    shaft[1].x = x0 + uy * shaft_width / 2;
    shaft[1].y = y0 - ux * shaft_width / 2;
    shaft[2].x = shaftx1 + uy * shaft_width / 2;
    shaft[2].y = shafty1 - ux * shaft_width / 2;
    shaft[3].x = shaftx1 - uy * shaft_width / 2;
    shaft[3].y = shafty1 + ux * shaft_width / 2;

    for (int i = 0; i < shaft.size(); i++)
        shaft[i].mColor = col;

    tColorVertexArray head;
    head.resize(3);

    // Compute the triangle points for the head
    head[0].x = x1 - ux * head_length - uy * head_width / 2;
    head[0].y = y1 - uy * head_length + ux * head_width / 2;
    head[1].x = x1 - ux * head_length + uy * head_width / 2;
    head[1].y = y1 - uy * head_length - ux * head_width / 2;
    head[2].x = x1;
    head[2].y = y1;

    for (int i = 0; i < head.size(); i++)
        head[i].mColor = col;


    gRasterizer.Rasterize(mpSurface.get(), shaft);
    gRasterizer.Rasterize(mpSurface.get(), head);
}

/*void ParticleSandbox::DrawArrow(ZFPoint start, ZFPoint end, uint32_t col)
{
    double x0 = start.x;
    double y0 = start.y;
    double x1 = end.x;
    double y1 = end.y;
    double dx = end.x - start.x;
    double dy = end.y - start.y;

    double length = sqrt(dx * dx + dy * dy);
    double ux = dx / length;
    double uy = dy / length;

    const double head_length = length / 2;
    const double head_width = length / 4;
    const double thickness = length / 16;

    double left_base_x = x1 - ux * head_length + uy * head_width / 2;
    double left_base_y = y1 - uy * head_length - ux * head_width / 2;
    double right_base_x = x1 - ux * head_length - uy * head_width / 2;
    double right_base_y = y1 - uy * head_length + ux * head_width / 2;

    mpSurface.get()->DrawAlphaLine(ZColorVertex(x0, y0, col), ZColorVertex(x1, y1, col), thickness);
    mpSurface.get()->DrawAlphaLine(ZColorVertex(x0, y0, col), ZColorVertex(x1, y1, col), thickness);
    mpSurface.get()->DrawAlphaLine(ZColorVertex(left_base_x, left_base_y, col), ZColorVertex(x1, y1, col), thickness);
    mpSurface.get()->DrawAlphaLine(ZColorVertex(right_base_x, right_base_y, col), ZColorVertex(x1, y1, col), thickness);
}
*/
void ParticleSandbox::CreateGravField()
{
    gravField.clear();

    const int kCountY = 6;
    const int kCountX = 16;

    int starty = mAreaLocal.Height() / (kCountY + 1);    // start offset a bit
    int startx = mAreaLocal.Width() / (kCountX + 1);    // start offset a bit

    int spacingy = mAreaLocal.Height() / kCountY;
    int spacingx = mAreaLocal.Height() / kCountY;
    
    for (int iy = 0; iy < kCountY; iy++)
    {
        int y = starty + spacingy * iy;
        for (int ix = 0; ix < kCountX; ix++)
        {
            int x = startx + spacingx * ix;

            Particle gf;

            gf.pos = ZFPoint(x, y);
            gf.dir.x = RANDDOUBLE((double)-10.0, (double)10.0);
            gf.dir.y = RANDDOUBLE((double)-10.0, (double)10.0);
            gf.mass = RANDEXP_DOUBLE(100.0, 600.0);

            gravField.push_back(gf);
        }
    }    
}

void ParticleSandbox::SpawnParticle(int64_t index)
{
    if (index == 0)
    {
        particles[index].mass = RANDDOUBLE(2000.0, 40000.0);
        particles[index].dir.x = 0;
        particles[index].dir.y = 0;
        particles[index].pos.x = mAreaLocal.Width() / 2;
        particles[index].pos.y = mAreaLocal.Height() / 2;
        particles[index].col = 0xffffffff;
        particles[index].active = false;
    }
    else
    {
        particles[index].mass = RANDEXP_DOUBLE(10.0, 200.0);
        particles[index].dir.x = RANDDOUBLE((double)-10.0, (double)10.0);
        particles[index].dir.y = RANDDOUBLE((double)-10.0, (double)10.0);
        //    particles[index].dir.x = 0;
        //    particles[index].dir.y = 0;
        particles[index].pos.x = RANDU64(mAreaLocal.Width() / 4, mAreaLocal.Width() * 2 / 4);
        particles[index].pos.y = RANDU64(mAreaLocal.Height() / 4, mAreaLocal.Height() * 3 / 4);
        particles[index].col = RANDU64(0xff000000, 0xffffffff);
        particles[index].active = true;
    }
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
                double fForce = (p2.mass*p->mass) / (fDist*fDist);

                double dX = ((p2.pos.x - p->pos.x) * fForce) / 50000.0;
                double dY = ((p2.pos.y - p->pos.y) * fForce) / 50000.0;

                force.x += dX;
                force.y += dY;
            }
        }
    }

    for (const auto& p2 : gravField)
    {
        double fDist = sqrt((p2.pos.x - p->pos.x) * (p2.pos.x - p->pos.x) + (p2.pos.y - p->pos.y) * (p2.pos.y - p->pos.y));

        if (fDist > 0)
        {
            double fForce = (p2.mass + p->mass) / (fDist * fDist);

            double dX = ((p2.dir.x) * fForce) / 10.0;
            double dY = ((p2.dir.y) * fForce) / 10.0;

            force.x += dX;
            force.y += dY;
        }
    }



    return force;
}


bool ParticleSandbox::Process()
{
    const double kFriction = 0.50;
    for (auto& p : particles)
    {
        ZFPoint force = GetForceVector(&p);
        p.dir.x += force.x * (1.0 - kFriction);
        p.dir.y += force.y * (1.0 - kFriction);
    }

    int64_t time = gTimer.GetUSSinceEpoch();
    double timeScale = (time - mLastTimeStamp) / 16000.0;

    for (int64_t i = 1; i < particles.size(); i++)  // 1 because particle 0 is the center big particle
    {
        Particle* p = &particles[i];
        if (p->active /*&& !(p->dir.x == 0 && p->dir.y == 0)*/ )
        {
            p->pos.x += p->dir.x * timeScale;
            p->pos.y += p->dir.y * timeScale;

            if (p->pos.x < -mAreaLocal.Width()*1.5 || p->pos.x > mAreaLocal.Width()*1.5 ||
                p->pos.y < -mAreaLocal.Height()*1.5 || p->pos.y > mAreaLocal.Height()*1.5)
                p->active = false;
        }
        else
        {
            SpawnParticle(i);
        }
    }


    mLastTimeStamp = time;

    Invalidate();
    return true;
}

bool ParticleSandbox::Paint()
{
    if (!PrePaintCheck())
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());




/*
    // test
    int64_t start = gTimer.GetUSSinceEpoch();
    for (int i = 0; i < 1000; i++)
    {
        mpSurface->Fill(0xff000000 | i);
    }
    int64_t end = gTimer.GetUSSinceEpoch();

    int64_t delta = end - start;
    */








    // erase drawn particles
/*    for (int64_t i = 0; i < particles.size(); i++)
    {
        mpSurface->FillAlpha(0xee000000, &prevFrameRects[i]);
    }*/
    mpSurface->Fill(0);

    for (int64_t i = 0; i < particles.size(); i++)
    {
        DrawParticle(mpSurface.get(), i);
    }

    for (int64_t i = 0; i < gravField.size(); i++)
    {
        ZFPoint end(gravField[i].pos);
        end.x += gravField[i].dir.x * (gravField[i].mass/100);
        end.y += gravField[i].dir.y * (gravField[i].mass / 100);
        
        DrawArrow(gravField[i].pos, end, 0xffff00ff);
    }

//    DrawArrow(ZFPoint(50,500), ZFPoint(250,190));

    return ZWin::Paint();
}

void ParticleSandbox::DrawParticle(ZBuffer* pDest, int64_t index)
{
    Particle& p = particles[index];
    if (!p.active)
        return;

    int64_t nSize = sqrt(p.mass);
    ZRect r(p.pos.x - nSize, p.pos.y - nSize, p.pos.x + nSize, p.pos.y + nSize);
    pDest->FillAlpha(p.col, &r);
    prevFrameRects[index].SetRect(r);

    ZFPoint pEnd(p.pos);
    pEnd.x += p.dir.x * 5;
    pEnd.y += p.dir.y * 5;

    if (index > 0)
        DrawArrow(p.pos, pEnd, p.col);

}


bool ParticleSandbox::OnKeyDown(uint32_t key)
{
        switch (key)
        {
        case VK_ESCAPE:
            gMessageSystem.Post("{quit_app_confirmed}");
            break;
        case ' ':
            for (int64_t i = 1; i < particles.size(); i++)
                particles[i].active = false;
            break;
        }
    return ZWin::OnKeyDown(key);
}

bool ParticleSandbox::OnParentAreaChange()
{
    return ZWin::OnParentAreaChange();
}
 
