#include "ParticleSandbox.h"
#include "ZTypes.h"
#include <utility>
#include "ZWinControlPanel.h"

//const int64_t kMaxParticles = 1000;
//const int64_t kTerminalVelocity = 10.0;

using namespace std;

ParticleSandbox::ParticleSandbox()
{
    mbAcceptsFocus = true;
    mbAcceptsCursorMessages = true;
    msWinName = "particlesandbox";
}
   
bool ParticleSandbox::Init()
{

    enableCollisions = RANDBOOL();
    gravFieldX = RANDI64(0, 4);
    gravFieldY = RANDI64(0, 4);

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


    mpControlPanel->Caption("particles_caption", "Particles");
    mpControlPanel->Slider("particles", &mMaxParticles, 0, 500, 2, 0.2, "{reinit;target=particlesandbox}", true, false);

    mpControlPanel->Caption("particles_min", "Particle Size Min");
    mpControlPanel->Slider("particleMassMin", &particleMassMin, 0, 400, 10, 0.2, "{reinit;target=particlesandbox}", true, false);

    mpControlPanel->Caption("particles_max", "Particle Size Max");
    mpControlPanel->Slider("particleMassMax", &particleMassMax, 0, 400, 10, 0.2, "{reinit;target=particlesandbox}", true, false);

    mpControlPanel->AddSpace(gM / 2);


    mpControlPanel->Caption("center_caption", "Center Mass");
    mpControlPanel->Slider("centerMass", &centerMass, 0, 4000, 10, 0.2, "{reinit;target=particlesandbox}", true, false);

    mpControlPanel->AddSpace(gM / 2);

    mpControlPanel->Caption("grav_caption", "Grav Field Grid X & Y");
    mpControlPanel->Slider("gravFieldX", &gravFieldX, 0, 10, 1, 0.2, "{reinit;target=particlesandbox}", true, false);
    mpControlPanel->Slider("gravFieldY", &gravFieldY, 0, 10, 1, 0.2, "{reinit;target=particlesandbox}", true, false);
    mpControlPanel->Toggle("draw_field_toggle", &drawGravField, "Draw Grav Field", "{reinit;target=particlesandbox}", "{reinit;target=particlesandbox}");

    mpControlPanel->Caption("term_caption", "Terminal Velocity");
    mpControlPanel->Slider("termVel", &mTerminalVelocity, 0, 100, 1, 0.2, "{reinit;target=particlesandbox}", true, false);
    
    mpControlPanel->AddSpace(gM / 2);
    mpControlPanel->Toggle("collisions_toggle", &enableCollisions, "Enable Collisions", "{reinit;target=particlesandbox}", "{reinit;target=particlesandbox}");

    mpControlPanel->AddSpace(gM / 2);
    mpControlPanel->Toggle("absorption_toggle", &enableAbsorption, "Enable Absorption", "{reinit;target=particlesandbox}", "{reinit;target=particlesandbox}");
    

    mpControlPanel->Toggle("arrows_toggle", &drawArrows, "Draw Arrows", "{reinit;target=particlesandbox}", "{reinit;target=particlesandbox}");

    mpControlPanel->AddSpace(gM / 2);
    mpControlPanel->Caption("friction_caption", "Friction");
    mpControlPanel->Slider("friction_slider", &mFriction, 0, 100, 1, 0.2, "{reinit;target=particlesandbox}", true, false);

    

    mpControlPanel->FitToControls();

    mpControlPanel->mTransformIn = ZWin::kToOrFrom;
    mpControlPanel->mTransformOut = ZWin::kToOrFrom;
    mpControlPanel->mToOrFrom = ZTransformation(ZPoint(grFullArea.right, grFullArea.top + gM), 0.0, 0.0);

    ChildAdd(mpControlPanel);


    InitFromParams();

    return ZWin::Init();
}

void ParticleSandbox::InitFromParams()
{
    particles.resize(mMaxParticles);
    prevFrameRects.resize(mMaxParticles);



    lightPos = Z3D::Vec3f(RANDDOUBLE(-5.0,5.0), RANDDOUBLE(-5.0, 5.0), 5);
    viewPos = Z3D::Vec3f(RANDDOUBLE(-5.0, 5.0), RANDDOUBLE(-5.0, 5.0), 1);
    ambient = Z3D::Vec3f(0.05, 0.05, 0.05);

    lightPos.normalize();
    viewPos.normalize();





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

    int starty = mAreaLocal.Height() / (gravFieldY + 1);    // start offset a bit
    int startx = mAreaLocal.Width() / (gravFieldX + 1);    // start offset a bit

    int spacingy = mAreaLocal.Height() / gravFieldY;
    int spacingx = mAreaLocal.Width() / gravFieldX;
    
    for (int iy = 0; iy < gravFieldY; iy++)
    {
        int y = starty + spacingy * iy;
        for (int ix = 0; ix < gravFieldX; ix++)
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
        particles[index].mass = centerMass;
        particles[index].dir.x = 0;
        particles[index].dir.y = 0;
        particles[index].pos.x = mAreaLocal.Width() / 2;
        particles[index].pos.y = mAreaLocal.Height() / 2;
        particles[index].col = 0xffffffff;
        particles[index].active = centerMass > 0;
    }
    else
    {
        particles[index].mass = RANDEXP_DOUBLE((double)particleMassMin, (double)particleMassMax);
        particles[index].dir.x = RANDDOUBLE((double)-10.0, (double)10.0);
        particles[index].dir.y = RANDDOUBLE((double)-10.0, (double)10.0);
        //    particles[index].dir.x = 0;
        //    particles[index].dir.y = 0;
        particles[index].pos.x = RANDU64(0, mAreaLocal.Width());
        particles[index].pos.y = RANDU64(0, mAreaLocal.Height());
        particles[index].col = RANDU64(0xff000000, 0xffffffff);
        particles[index].fShininess = RANDDOUBLE(0, 255);
        particles[index].specular = Z3D::Vec3f(RANDDOUBLE(0, 1.0), RANDDOUBLE(0, 1.0), RANDDOUBLE(0, 1.0));
        particles[index].diffuse = Z3D::Vec3f(RANDDOUBLE(0, 1.0), RANDDOUBLE(0, 1.0), RANDDOUBLE(0, 1.0));

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

ZFPoint ParticleSandbox::LimitMagnitude(ZFPoint v)
{
    if (mTerminalVelocity > 0)
    {
        double mag = Magnitude(v);
        if (mag > mTerminalVelocity / 10.0)
        {
            return ZFPoint(v.x * (mTerminalVelocity / mag), v.y * (mTerminalVelocity / mag));
        }
    }
    return v;
}


bool ParticleSandbox::Process()
{
    if (!paused)
    {
        // check for collisions
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


        for (auto& p : particles)
        {
            ZFPoint force = GetForceVector(&p);

            double friction = (double)mFriction / 100.0;

            p.dir.x += force.x * (1.0 - friction);
            p.dir.y += force.y * (1.0 - friction);

            p.dir = LimitMagnitude(p.dir);
        }

        int64_t time = gTimer.GetUSSinceEpoch();
        double timeScale = (time - mLastTimeStamp) / 16000.0;

        for (int64_t i = 1; i < particles.size(); i++)  // 1 because particle 0 is the center big particle
        {
            Particle* p = &particles[i];
            if (p->active /*&& !(p->dir.x == 0 && p->dir.y == 0)*/)
            {
                p->pos.x += p->dir.x * timeScale;
                p->pos.y += p->dir.y * timeScale;

                if (p->pos.x < -mAreaLocal.Width() * 1.5 || p->pos.x > mAreaLocal.Width() * 1.5 ||
                    p->pos.y < -mAreaLocal.Height() * 1.5 || p->pos.y > mAreaLocal.Height() * 1.5)
                    p->active = false;
            }
            else
            {
                SpawnParticle(i);
            }
        }

        // shift grav field
        for (int64_t i = 0; i < gravField.size(); i++)
        {
            Particle* p = &gravField[i];
            ZFPoint force = GetForceVector(p);
            double friction = (double)mFriction / 100.0;
            p->dir.x += force.x * (1.0 - friction);
            p->dir.y += force.y * (1.0 - friction);

            p->dir = LimitMagnitude(p->dir);
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
    mpSurface->Fill(0);

    for (int64_t i = 0; i < particles.size(); i++)
    {
        DrawParticle(mpSurface.get(), i);
    }

    if (drawGravField)
    {
        for (int64_t i = 0; i < gravField.size(); i++)
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
    int64_t size = 2*sqrt(mass);

    if (rendering->GetArea().right != size)
    {    
        rendering->Init(size, size);
        rendering->Fill(0);

        ZPoint center(size/2, size/2);
        rendering->DrawSphere(center, size/2, lightPos, viewPos, ambient, diffuse, specular, fShininess);
    }

    ZRect rDest(pos.x, pos.y, pos.x + size, pos.y + size);
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
            InitFromParams();
            break;
        }
    return ZWin::OnKeyDown(key);
}

bool ParticleSandbox::OnParentAreaChange()
{
    return ZWin::OnParentAreaChange();
}
 
