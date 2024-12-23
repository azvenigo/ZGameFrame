#include "ParticleSandbox.h"
#include "ZTypes.h"
#include <utility>
#include "ZWinControlPanel.h"

//const int64_t kMaxParticles = 1000;
//const int64_t kTerminalVelocity = 10.0;

using namespace std;

const int kParticleMaxSize = 1024;
float fShininess = 132.0f;
Z3D::Vec3f specular(0.0, 1.0, 1.0);

vector<tZBufferPtr> particleBuffers;

ParticleSandbox::ParticleSandbox()
{
    mbAcceptsFocus = true;
    mbAcceptsCursorMessages = true;
    msWinName = "particlesandbox";
}
   
bool ParticleSandbox::Init()
{
    particleBuffers.resize(kParticleMaxSize);


    //gravFieldX = RANDI64(0, 10);
    //gravFieldY = RANDI64(0, 10);

    int64_t panelW = grFullArea.Width() / 8;
    int64_t panelH = grFullArea.Height() / 2;
    ZRect rControlPanel(panelW, panelH);
    rControlPanel = ZGUI::Arrange(rControlPanel, grFullArea, ZGUI::RB);

    mpControlPanel = new ZWinControlPanel();
    mpControlPanel->SetArea(rControlPanel);
    mpControlPanel->mrTrigger = rControlPanel;
    mpControlPanel->mbHideOnMouseExit = true;

    mpControlPanel->Init();

    mpControlPanel->Button("quit_app_confirmed", "Quit", "{quit_app_confirmed}");
    mpControlPanel->AddSpace(gM / 2);

    mpControlPanel->Toggle("frame_clear", &frameClear, "Frame Clear");


    mpControlPanel->Caption("particles_caption", "Particles");
    mpControlPanel->Slider("particles", &mMaxParticles, 0, 1000, 1, 0.2, "{reinit;target=particlesandbox}", true, false);

    mpControlPanel->Caption("particles_min", "Particle Size Min");
    mpControlPanel->Slider("particleMassMin", &particleMassMin, 0, 1000, 10, 0.2, "{reinit;target=particlesandbox}", true, false);

    mpControlPanel->Caption("particles_max", "Particle Size Max");
    mpControlPanel->Slider("particleMassMax", &particleMassMax, 0, 1000, 10, 0.2, "{reinit;target=particlesandbox}", true, false);

    mpControlPanel->AddSpace(gM / 2);


    mpControlPanel->Caption("center_caption", "Center Mass");
    mpControlPanel->Slider("centerMass", &centerMass, 0, 4000, 10, 0.2, "{reinit;target=particlesandbox}", true, false);

    mpControlPanel->AddSpace(gM / 2);

    mpControlPanel->Caption("grav_caption", "Grav Field Grid X & Y");
    mpControlPanel->Toggle("gravField_toggle", &shiftingGravField, "Shifting Grav Field", "{reinit;target=particlesandbox}", "{reinit;target=particlesandbox}");
    

    mpControlPanel->Slider("gravFieldX", &gravFieldX, 0, 100, 1, 0.2, "{reinit;target=particlesandbox}", true, false);
    mpControlPanel->Slider("gravFieldY", &gravFieldY, 0, 100, 1, 0.2, "{reinit;target=particlesandbox}", true, false);
    mpControlPanel->Toggle("draw_field_toggle", &drawGravField, "Draw Grav Field", "{reinit;target=particlesandbox}", "{reinit;target=particlesandbox}");

    mpControlPanel->Caption("term_caption", "Terminal Velocity");
    mpControlPanel->Slider("termVel", &mTerminalVelocity, 0, 1000, 1, 0.2, "", true, false);
    
    mpControlPanel->AddSpace(gM / 2);
    mpControlPanel->Toggle("collisions_toggle", &enableCollisions, "Enable Collisions", "{reinit;target=particlesandbox}", "{reinit;target=particlesandbox}");

    mpControlPanel->AddSpace(gM / 2);
    mpControlPanel->Toggle("absorption_toggle", &enableAbsorption, "Enable Absorption", "{reinit;target=particlesandbox}", "{reinit;target=particlesandbox}");
    

    mpControlPanel->Toggle("arrows_toggle", &drawArrows, "Draw Arrows", "{reinit;target=particlesandbox}", "{reinit;target=particlesandbox}");

    mpControlPanel->AddSpace(gM / 2);
    mpControlPanel->Caption("atten_caption", "Attenuation");
    mpControlPanel->Slider("atten_slider", &Attenuation, 0, 200, 1, 0.2, "{reinit;target=particlesandbox}", true, false);

    

    mpControlPanel->FitToControls();

    mpControlPanel->mTransformIn = ZWin::kToOrFrom;
    mpControlPanel->mTransformOut = ZWin::kToOrFrom;
    mpControlPanel->mToOrFrom = ZTransformation(ZPoint(grFullArea.right, grFullArea.top + gM), 0.0, 0.0);

    ChildAdd(mpControlPanel);


    lightPos = Z3D::Vec3f((float)RANDDOUBLE(-5.0, 5.0), (float)RANDDOUBLE(-5.0, 5.0), (float)5);
    viewPos = Z3D::Vec3f((float)RANDDOUBLE(-5.0, 5.0), (float)RANDDOUBLE(-5.0, 5.0), (float)1);
    ambient = Z3D::Vec3f(0.05f, 0.05f, 0.05f);

    lightPos.normalize();
    viewPos.normalize();



    InitFromParams();

    return ZWin::Init();
}

void ParticleSandbox::InitFromParams()
{
    particles.resize(mMaxParticles);
    prevFrameRects.resize(mMaxParticles);


    for (int i = 0; i < particles.size(); i++)
        SpawnParticle(i);

    CreateGravField();

    mLastTimeStamp = gTimer.GetUSSinceEpoch();

}

bool ParticleSandbox::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();

    if (sType == "reinit")
    {
        InitFromParams();
        return true;
    }

    return ZWin::HandleMessage(message);
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
    if (gravField.size() == gravFieldX * gravFieldY)    // no need to change gravfield if it's already the right size
        return;

    gravField.clear();

    if (gravFieldX == 0 || gravFieldY == 0)
        return;

    int64_t starty = mAreaLocal.Height() / (gravFieldY + 1);    // start offset a bit
    int64_t startx = mAreaLocal.Width() / (gravFieldX + 1);    // start offset a bit

    int64_t spacingy = mAreaLocal.Height() / gravFieldY;
    int64_t spacingx = mAreaLocal.Width() / gravFieldX;
    
    for (int64_t iy = 0; iy < gravFieldY; iy++)
    {
        int64_t y = starty + spacingy * iy;
        for (int64_t ix = 0; ix < gravFieldX; ix++)
        {
            int64_t x = startx + spacingx * ix;

            Particle gf;

            gf.pos = ZFPoint((float)x, (float)y);
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
        particles[index].mass = (double)centerMass;
        particles[index].dir.x = 0;
        particles[index].dir.y = 0;
        particles[index].pos.x = mAreaLocal.Width() / 2.0;
        particles[index].pos.y = mAreaLocal.Height() / 2.0;
        particles[index].col = 0xffffffff;
        particles[index].active = centerMass > 0;
    }
    else
    {
        if (particles[index].mass < 10.0 || particles[index].mass > particleMassMax)
            particles[index].mass = RANDEXP_DOUBLE((double)particleMassMin, (double)particleMassMax);
        particles[index].dir.x = RANDDOUBLE((double)-mTerminalVelocity/2, (double)mTerminalVelocity/2);
        particles[index].dir.y = RANDDOUBLE((double)-mTerminalVelocity/2, (double)mTerminalVelocity/2);
        //    particles[index].dir.x = 0;
        //    particles[index].dir.y = 0;

        double randdist = RANDDOUBLE(0, mAreaLocal.Width());
        double angle = RANDDOUBLE(0.0, 360.0 / (2 * 3.1415));

        particles[index].pos.x = mAreaLocal.Width() / 2 + randdist * sin(angle);
        particles[index].pos.y = mAreaLocal.Height() / 2 + randdist * cos(angle);
        particles[index].col = RANDU64(0xff000000, 0xffffffff);

        particles[index].active = true;
    }
}
ZFPoint GetForceVector(Particle* p, tParticleArray& particles, const tParticleArray& gravField, double atten, Particle** pOutCollision)
{
    ZFPoint force;

    for (auto& p2 : particles)
    {
        if (p2.active && &p2 != p)
        {
            // Calculate displacement vector
            double dx = p2.pos.x - p->pos.x;
            double dy = p2.pos.y - p->pos.y;

            // Calculate distance
            double distanceSquared = dx * dx + dy * dy;
            double distance = sqrt(distanceSquared);

            if (distance < sqrt(p->mass + p2.mass)) // Collision check
                *pOutCollision = &p2;
            else
            //if (distance > 10.0) // Avoid division by zero
            {
                // Gravitational force magnitude
                double fMassSquared = p->mass * p2.mass;
/*                if (atten > 0.0)
                    fMassSquared /= atten;*/
                double fForce = (fMassSquared) / distanceSquared;

                // Apply attenuation
//                fForce /= (10000.0 * atten);
                fForce *= 0.002;
                fForce *= (p2.mass / p->mass);

                // Normalize displacement vector and scale by force magnitude
                force.x += fForce * (dx / distance);
                force.y += fForce * (dy / distance);
            }
        }
    }

    for (const auto& p2 : gravField)
    {
        // Calculate displacement vector
        double dx = p2.pos.x - p->pos.x;
        double dy = p2.pos.y - p->pos.y;

        // Calculate distance
        double distanceSquared = dx * dx + dy * dy;
        double distance = sqrt(distanceSquared);

        if (distance > 0) // Avoid division by zero
        {
            // Gravitational force magnitude (using p2.dir for directional influence)
            double fForce = (p->mass + p2.mass) / distanceSquared;

            // Normalize displacement vector and scale by force magnitude
            force.x += (fForce * p2.dir.x) / 10.0;
            force.y += (fForce * p2.dir.y) / 10.0;
        }
    }

    return force;
}

bool Collision(Particle* p1, Particle* p2)
{
    double dist = sqrt((p1->pos.x - p2->pos.x) * (p1->pos.x - p2->pos.x) + (p1->pos.y - p2->pos.y) * (p1->pos.y - p2->pos.y));
    bool bCollision = sqrt(p1->mass + p2->mass) > dist;
    return bCollision;
}

double Magnitude(ZFPoint v)
{
    return sqrt(v.x * v.x + v.y * v.y);
}

ZFPoint LimitMagnitude(ZFPoint v, double terminal)
{
    if (terminal > 0)
    {
        double mag = Magnitude(v);
        if (mag > terminal / 10.0)
        {
            return ZFPoint(v.x * (terminal / mag), v.y * (terminal / mag));
        }
    }
    return v;
}


void UpdateParticleDirs(tParticleArray& particles, tParticleArray& gravField, size_t i, size_t last, int64_t terminal, int64_t atten, bool bEnableCollide, bool bEnableAbsorb)
{
    while (i != last)
    {
        auto& p = particles[i];

        if (p.active)
        {
            Particle* pCollision = nullptr;

            ZFPoint force = GetForceVector(&p, particles, gravField, atten / 50.0, &pCollision);

            if (bEnableCollide && pCollision && pCollision->mass < p.mass)
            {
                pCollision->active = false;
                continue;
            }
            else if (bEnableAbsorb && pCollision)
            {
                if (i != 0)
                {
                    p.mass += pCollision->mass;
                }
                pCollision->active = false;
            }

/*            double attenuation = 1.0;
            if (atten > 0)
            {
                attenuation = 100.0 / (double)atten;
            }*/

            if (terminal > 0)
            {
                force.x *= (double)terminal / 100.0;
                force.y *= (double)terminal / 100.0;
            }

            p.dir.x += force.x/* * attenuation*/;
            p.dir.y += force.y/* * attenuation*/;

/*            if (terminal > 0)
            {
                p.dir = LimitMagnitude(p.dir, terminal / 10.0);
            }*/
        }

        i++;
    }
}

bool ParticleSandbox::Process()
{
    if (!paused)
    {
/*        // check for collisions
        if (enableAbsorption)
        {
            for (int i = 1; i < particles.size(); i++)  // 0 is center so don't consider it for absorption
            {
                auto& p = particles[i];
                for (int j = 1; j < particles.size(); j++)
                {
                    auto& p2 = particles[j];
                    if (&p != &p2)
                    {
                        if (Collision(&p, &p2) && p2.mass > p.mass)
                        {
                            p2.mass += p.mass;
                            p.active = false;

                            if (p2.mass > 40000)
                                p2.active = false;

                            break;
                        }
                    }
                }
            }
        }
        else if (enableCollisions)
        {
            for (int i = 1; i < particles.size(); i++) // 0 is center so don't consider it for collision
            {
                auto& p = particles[i];
                for (int j = 0; j < particles.size(); j++)
                {
                    auto& p2 = particles[j];
                    if (&p != &p2)
                    {
                        if (Collision(&p, &p2) && p2.mass > p.mass)
                        {
                            p.active = false;
                            break;
                        }
                    }
                }
            }
        }
        */

        size_t threads = std::thread::hardware_concurrency();
        workers.resize(threads);

        size_t jobs_per_thread = std::max<size_t>(particles.size() / threads, 1);
        size_t batches = (particles.size() + jobs_per_thread - 1) / jobs_per_thread;

        if (batches > particles.size())
            batches = particles.size();
        if (batches > workers.size())
            batches = workers.size();


        for (int i = 0; i < batches; i++)
        {
            size_t first = i * jobs_per_thread;
            size_t last = first + jobs_per_thread;
            if (last > particles.size())
                last = particles.size();
            if (i == threads - 1)
                last = particles.size();

            assert(first < particles.size());
            assert(last < particles.size()+1);
            workers[i] = std::thread(UpdateParticleDirs, std::ref(particles), std::ref(gravField), first, last, mTerminalVelocity, Attenuation, enableCollisions, enableAbsorption);
        }

        for (int i = 0; i < batches; i++)
        {
            workers[i].join();
        }


        int64_t time = gTimer.GetUSSinceEpoch();
        double timeScale = (time - mLastTimeStamp) / 160000.0;

        for (int64_t i = 1; i < (int64_t)particles.size(); i++)  // 1 because particle 0 is the center big particle
        {
            Particle* p = &particles[i];
            if (p->active /*&& !(p->dir.x == 0 && p->dir.y == 0)*/)
            {
                if (p->mass > particleMassMax)
                    p->active = false;

                p->pos.x += p->dir.x * timeScale;
                p->pos.y += p->dir.y * timeScale;

                if (p->pos.x < -mAreaLocal.Width() * 2 || p->pos.x > mAreaLocal.Width() * 2 ||
                    p->pos.y < -mAreaLocal.Height() * 2 || p->pos.y > mAreaLocal.Height() * 2)
                    p->active = false;
            }
            else
            {
                SpawnParticle(i);
            }
        }

        // shift grav field
        if (shiftingGravField)
        {
            for (int64_t i = 0; i < (int64_t)gravField.size(); i++)
            {
                Particle* p = &gravField[i];
                Particle* pCollision = nullptr;
                ZFPoint force = GetForceVector(p, particles, gravField, Attenuation / 50.0, &pCollision);
                double attenuation = (double)Attenuation / 50.0;
                p->dir.x += force.x/* * attenuation*/;
                p->dir.y += force.y /* * attenuation*/;

                p->dir = LimitMagnitude(p->dir, mTerminalVelocity/10.0);
            }
        }

        mLastTimeStamp = time;

        Invalidate();
    }
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

    if (frameClear)
        mpSurface->Fill(0xff000000);

    for (int64_t i = 0; i < (int64_t)particles.size(); i++)
    {
        DrawParticle(mpSurface.get(), i);
    }

    if (drawGravField)
    {
        for (int64_t i = 0; i < (int64_t)gravField.size(); i++)
        {
            ZFPoint end(gravField[i].pos);
            end.x += gravField[i].dir.x * (gravField[i].mass / 100);
            end.y += gravField[i].dir.y * (gravField[i].mass / 100);

            DrawArrow(gravField[i].pos, end, 0xffff00ff);
        }
    }
//    DrawArrow(ZFPoint(50,500), ZFPoint(250,190));

    return ZWin::Paint();
}

bool Particle::Draw(ZBuffer* pDest, Z3D::Vec3f lightPos, Z3D::Vec3f viewPos, Z3D::Vec3f ambient)
{
//    int64_t size = 10;

    int64_t size = (int64_t)(2.0 * sqrt(mass));
    limit<int64_t>(size, 1, kParticleMaxSize);

    if (rendering->GetArea().right != size)
    {    
        if (particleBuffers[size] == nullptr)
        {
            particleBuffers[size].reset(new ZBuffer());
            particleBuffers[size]->Init(size, size);
            particleBuffers[size]->Fill(0);
            particleBuffers[size]->mbHasAlphaPixels = true;

            ZPoint center(size / 2, size / 2);
            particleBuffers[size]->DrawSphere(center, size / 2, lightPos, viewPos, ambient, Z3D::Vec3f(1,1,1), specular, fShininess);
        }

        rendering->Init(size, size);
        rendering->CopyPixels(particleBuffers[size].get());
        rendering->mbHasAlphaPixels = true;

        uint32_t nHSV = COL::ARGB_To_AHSV(col);
        uint32_t nH = AHSV_H(nHSV);
        uint32_t nS = AHSV_S(nHSV);

        rendering->Colorize(nH, nS);
    }
    

    ZRect rDest((int64_t)pos.x, (int64_t)pos.y, (int64_t)pos.x + size, (int64_t)pos.y + size);
    rDest.OffsetRect(-size / 2, -size / 2);

    return pDest->Blt(rendering.get(), rendering->GetArea(), rDest);
}

void ParticleSandbox::DrawParticle(ZBuffer* pDest, int64_t index)
{
    Particle& p = particles[index];
    if (!p.active || p.mass < 0)
        return;

    p.Draw(pDest, lightPos, viewPos, ambient);


    ZFPoint pEnd(p.pos);
    pEnd.x += p.dir.x * 5;
    pEnd.y += p.dir.y * 5;

    if (index > 0 && drawArrows)
        DrawArrow(p.pos, pEnd, p.col);

}



bool ParticleSandbox::OnMouseDownR(int64_t x, int64_t y)
{
    paused = !paused;
    return ZWin::OnMouseDownR(x, y);
}


bool ParticleSandbox::OnKeyDown(uint32_t key)
{
        switch (key)
        {
        case VK_ESCAPE:
            gMessageSystem.Post("{quit_app_confirmed}");
            break;
        case ' ':
            particleBuffers.clear();
            particleBuffers.resize(kParticleMaxSize);
            InitFromParams();
            break;
        }
    return ZWin::OnKeyDown(key);
}

bool ParticleSandbox::OnParentAreaChange()
{
    return ZWin::OnParentAreaChange();
}
 
