/** 2D Computer Graphics OpenGL/freeGLUT game
 * Built for macOS + C++ + GLUT
 *
 * Goal: Collect all energy crystals, avoid meteors, and land safely.
 * Landing pad repairs hull damage and refuels the ship.
 * Safe landing requires low speed, upright angle, and landing on the pad.
 *
 * Controls:
 *   ENTER  - Start game / play again
 *   A/D or Left/Right arrows - Rotate lander
 *   W or Up arrow - Thrust
 *   P - Pause / resume
 *   R - Restart
 *   ESC - Exit
 */

#include <cmath>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

const int WIN_W = 800;
const int WIN_H = 600;
const float PI = 3.1415926535f;

struct Vec2 {
    float x;
    float y;
};

struct Star {
    float x, y, speed, size;
};

struct Particle {
    float x, y;
    float vx, vy;
    float life;
    float r, g, b;
};

struct Crystal {
    float x, y;
    bool collected;
};

struct Meteor {
    float x, y;
    float vx, vy;
    float radius;
    float angle;
    float spin;
};

struct FloatingText {
    float x, y;
    float vy;
    float life;
    std::string text;
};

struct Lander {
    float x, y;
    float vx, vy;
    float angle;      // 0 = facing up. Positive value means rotate clockwise.
    float fuel;
    int hull;
};

enum GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER,
    GAME_WON
};

GameState state = MENU;
Lander lander;

bool keys[256] = {false};
bool specialKeys[256] = {false};

std::vector<Star> stars;
std::vector<Particle> particles;
std::vector<Crystal> crystals;
std::vector<Meteor> meteors;
std::vector<FloatingText> floatingTexts;
std::vector<Vec2> terrain;

int collectedCrystals = 0;
int score = 0;
float screenShake = 0.0f;
float repairTimer = 0.0f;
bool isLandedOnPad = false;
std::string endMessage = "";

const float LANDER_RADIUS = 18.0f;
const float PAD_X1 = 340.0f;
const float PAD_X2 = 460.0f;
const float PAD_Y  = 105.0f;

float randFloat(float minV, float maxV) {
    return minV + (maxV - minV) * ((float)rand() / (float)RAND_MAX);
}

float degToRad(float deg) {
    return deg * PI / 180.0f;
}

float distance2D(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    return std::sqrt(dx * dx + dy * dy);
}

void drawText(float x, float y, const std::string& text, void* font = GLUT_BITMAP_8_BY_13) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}

void drawFilledCircle(float cx, float cy, float r, int segments = 32) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; ++i) {
        float a = 2.0f * PI * (float)i / (float)segments;
        glVertex2f(cx + std::cos(a) * r, cy + std::sin(a) * r);
    }
    glEnd();
}

void drawOutlinedCircle(float cx, float cy, float r, int segments = 32) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float a = 2.0f * PI * (float)i / (float)segments;
        glVertex2f(cx + std::cos(a) * r, cy + std::sin(a) * r);
    }
    glEnd();
}

void drawRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void addFloatingText(float x, float y, const std::string& text) {
    floatingTexts.push_back({x, y, 1.0f, 1.0f, text});
}

// ==========================================
// CONCEPT 4: Rotation Vectors
// Using trigonometric calculations (cos/sin) to disperse radial explosion debris.
// ==========================================
void addExplosion(float x, float y, float r, float g, float b, int count) {
    for (int i = 0; i < count; ++i) {
        float a = randFloat(0.0f, 2.0f * PI);
        float spd = randFloat(1.0f, 4.5f);
        particles.push_back({x, y, std::cos(a) * spd, std::sin(a) * spd, randFloat(0.5f, 1.0f), r, g, b});
    }
}

void initStars() {
    stars.clear();
    for (int i = 0; i < 120; ++i) {
        Star s;
        s.x = randFloat(0.0f, (float)WIN_W);
        s.y = randFloat(0.0f, (float)WIN_H);
        s.speed = randFloat(0.2f, 1.5f);
        s.size = randFloat(1.0f, 3.0f);
        stars.push_back(s);
    }
}

void initTerrain() {
    terrain.clear();
    terrain.push_back({0, 60});
    terrain.push_back({80, 90});
    terrain.push_back({160, 65});
    terrain.push_back({250, 130});
    terrain.push_back({330, 95});
    terrain.push_back({PAD_X1, PAD_Y});
    terrain.push_back({PAD_X2, PAD_Y});
    terrain.push_back({470, 95});
    terrain.push_back({560, 145});
    terrain.push_back({640, 85});
    terrain.push_back({720, 115});
    terrain.push_back({800, 80});
}

float getTerrainY(float x) {
    if (terrain.empty()) return 0.0f;
    if (x <= terrain.front().x) return terrain.front().y;
    if (x >= terrain.back().x) return terrain.back().y;

    for (size_t i = 0; i < terrain.size() - 1; ++i) {
        Vec2 a = terrain[i];
        Vec2 b = terrain[i + 1];
        if (x >= a.x && x <= b.x) {
            float t = (x - a.x) / (b.x - a.x);
            return a.y + t * (b.y - a.y);
        }
    }
    return 0.0f;
}

void resetMeteors() {
    meteors.clear();
    for (int i = 0; i < 5; ++i) {
        Meteor m;
        m.x = randFloat(40.0f, WIN_W - 40.0f);
        m.y = randFloat(WIN_H + 40.0f, WIN_H + 380.0f);
        m.vx = randFloat(-0.8f, 0.8f);
        m.vy = randFloat(-2.4f, -1.0f);
        m.radius = randFloat(12.0f, 24.0f);
        m.angle = randFloat(0.0f, 360.0f);
        m.spin = randFloat(-3.0f, 3.0f);
        meteors.push_back(m);
    }
}

void resetCrystals() {
    crystals.clear();
    crystals.push_back({115, 230, false});
    crystals.push_back({245, 390, false});
    crystals.push_back({520, 205, false});
    crystals.push_back({640, 315, false});
    crystals.push_back({690, 455, false});
    collectedCrystals = 0;
}

void resetGame() {
    lander.x = 130.0f;
    lander.y = 520.0f;
    lander.vx = 0.0f;
    lander.vy = 0.0f;
    lander.angle = 0.0f;
    lander.fuel = 100.0f;
    lander.hull = 3;

    score = 0;
    screenShake = 0.0f;
    repairTimer = 0.0f;
    isLandedOnPad = false;
    endMessage = "";
    particles.clear();
    floatingTexts.clear();
    resetCrystals();
    resetMeteors();
}

void drawStars() {
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (const Star& s : stars) {
        float brightness = 0.35f + s.speed / 2.0f;
        glColor3f(brightness, brightness, brightness);
        glVertex2f(s.x, s.y);
    }
    glEnd();
}

void drawTerrain() {
    // Moon ground fill
    glBegin(GL_POLYGON);
    glColor3f(0.12f, 0.12f, 0.16f);
    glVertex2f(0, 0);
    for (const Vec2& p : terrain) {
        glVertex2f(p.x, p.y);
    }
    glVertex2f(WIN_W, 0);
    glEnd();

    // Moon ground outline
    glLineWidth(2.0f);
    glColor3f(0.55f, 0.55f, 0.62f);
    glBegin(GL_LINE_STRIP);
    for (const Vec2& p : terrain) {
        glVertex2f(p.x, p.y);
    }
    glEnd();
    glLineWidth(1.0f);

    // Landing pad
    glColor3f(0.1f, 0.8f, 0.55f);
    drawRect(PAD_X1, PAD_Y - 4.0f, PAD_X2 - PAD_X1, 8.0f);

    glColor3f(0.9f, 1.0f, 0.55f);
    drawText(PAD_X1 + 30.0f, PAD_Y + 12.0f, "LANDING PAD");

    // Small pad lights
    glColor3f(0.0f, 1.0f, 0.5f);
    drawFilledCircle(PAD_X1 + 12.0f, PAD_Y + 8.0f, 4.0f);
    drawFilledCircle(PAD_X2 - 12.0f, PAD_Y + 8.0f, 4.0f);
}

void drawLander() {
    glPushMatrix();
    glTranslatef(lander.x, lander.y, 0.0f);
    glRotatef(-lander.angle, 0.0f, 0.0f, 1.0f);

    // Body triangle
    if (lander.hull == 3) glColor3f(0.25f, 0.75f, 1.0f);
    else if (lander.hull == 2) glColor3f(1.0f, 0.75f, 0.15f);
    else glColor3f(1.0f, 0.25f, 0.2f);

    glBegin(GL_TRIANGLES);
    glVertex2f(0, 22);
    glVertex2f(-16, -15);
    glVertex2f(16, -15);
    glEnd();

    // Window
    glColor3f(0.05f, 0.08f, 0.15f);
    drawFilledCircle(0, 3, 5, 18);

    // Landing legs
    glColor3f(0.8f, 0.8f, 0.85f);
    glBegin(GL_LINES);
    glVertex2f(-10, -12); glVertex2f(-22, -24);
    glVertex2f(10, -12);  glVertex2f(22, -24);
    glVertex2f(-22, -24); glVertex2f(-10, -24);
    glVertex2f(22, -24);  glVertex2f(10, -24);
    glEnd();

    // Shield outline
    if (lander.hull > 0) {
        glColor3f(0.35f, 0.9f, 1.0f);
        drawOutlinedCircle(0, 0, LANDER_RADIUS + 4.0f, 40);
    }

    glPopMatrix();
}

void drawThrustPreview() {
    if (!keys['w'] && !keys['W'] && !specialKeys[GLUT_KEY_UP]) return;
    if (lander.fuel <= 0.0f || state != PLAYING) return;

    glPushMatrix();
    glTranslatef(lander.x, lander.y, 0.0f);
    glRotatef(-lander.angle, 0.0f, 0.0f, 1.0f);
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.8f, 0.1f);
    glVertex2f(-8, -16);
    glVertex2f(8, -16);
    glColor3f(1.0f, 0.2f, 0.0f);
    glVertex2f(0, -38);
    glEnd();
    glPopMatrix();
}

void drawCrystals() {
    float pulse = std::sin((float)glutGet(GLUT_ELAPSED_TIME) * 0.005f) * 2.0f;
    for (const Crystal& c : crystals) {
        if (c.collected) continue;
        glPushMatrix();
        glTranslatef(c.x, c.y, 0.0f);
        glRotatef((float)glutGet(GLUT_ELAPSED_TIME) * 0.07f, 0.0f, 0.0f, 1.0f);
        glColor3f(0.2f, 1.0f, 0.9f);
        glBegin(GL_POLYGON);
        glVertex2f(0, 14 + pulse);
        glVertex2f(10 + pulse, 0);
        glVertex2f(0, -14 - pulse);
        glVertex2f(-10 - pulse, 0);
        glEnd();
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(0, 14 + pulse);
        glVertex2f(10 + pulse, 0);
        glVertex2f(0, -14 - pulse);
        glVertex2f(-10 - pulse, 0);
        glEnd();
        glPopMatrix();
    }
}

void drawMeteor(const Meteor& m) {
    glPushMatrix();
    glTranslatef(m.x, m.y, 0.0f);
    glRotatef(m.angle, 0.0f, 0.0f, 1.0f);

    glColor3f(0.55f, 0.35f, 0.18f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 9; ++i) {
        float a = 2.0f * PI * (float)i / 9.0f;
        float rr = m.radius * (0.75f + 0.25f * std::sin(i * 2.1f));
        glVertex2f(std::cos(a) * rr, std::sin(a) * rr);
    }
    glEnd();

    glColor3f(0.9f, 0.6f, 0.3f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 9; ++i) {
        float a = 2.0f * PI * (float)i / 9.0f;
        float rr = m.radius * (0.75f + 0.25f * std::sin(i * 2.1f));
        glVertex2f(std::cos(a) * rr, std::sin(a) * rr);
    }
    glEnd();

    glPopMatrix();
}

void drawParticles() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (const Particle& p : particles) {
        glColor4f(p.r, p.g, p.b, p.life);
        drawFilledCircle(p.x, p.y, 2.0f + p.life * 3.0f, 12);
    }
    glDisable(GL_BLEND);
}

void drawFloatingTexts() {
    for (const FloatingText& ft : floatingTexts) {
        glColor3f(1.0f, 0.95f, 0.4f);
        drawText(ft.x, ft.y, ft.text, GLUT_BITMAP_HELVETICA_18);
    }
}

// ==========================================
// CONCEPT 2: Polygon Filling & Primitive Construction
// Utilizing drawRect/drawTriangle for dynamic bars (Fuel/Hull) and object construction.
// ==========================================
void drawHUD() {
    std::stringstream ss;
    glColor3f(1.0f, 1.0f, 1.0f);

    ss << "Crystals: " << collectedCrystals << "/" << crystals.size();
    drawText(18, WIN_H - 25, ss.str(), GLUT_BITMAP_HELVETICA_18);

    ss.str(""); ss.clear();
    ss << "Hull: " << lander.hull;
    drawText(18, WIN_H - 50, ss.str(), GLUT_BITMAP_HELVETICA_18);

    ss.str(""); ss.clear();
    ss << "Velocity X: " << (int)(lander.vx * 10) / 10.0f << "  Y: " << (int)(lander.vy * 10) / 10.0f;
    drawText(18, WIN_H - 75, ss.str());

    ss.str(""); ss.clear();
    ss << "Angle: " << (int)lander.angle << " deg";
    drawText(18, WIN_H - 95, ss.str());

    // Fuel bar
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(WIN_W - 220, WIN_H - 25, "Fuel");
    glColor3f(0.25f, 0.25f, 0.3f);
    drawRect(WIN_W - 170, WIN_H - 33, 140, 14);
    if (lander.fuel > 50.0f) glColor3f(0.2f, 0.9f, 0.4f);
    else if (lander.fuel > 20.0f) glColor3f(1.0f, 0.8f, 0.1f);
    else glColor3f(1.0f, 0.2f, 0.1f);
    drawRect(WIN_W - 170, WIN_H - 33, 140.0f * (lander.fuel / 100.0f), 14);

    glColor3f(0.7f, 0.9f, 1.0f);
    drawText(WIN_W - 285, 18, "Safe landing: slow speed + upright + on pad");
}

void drawMenu() {
    drawStars();
    glColor3f(0.2f, 0.9f, 1.0f);
    drawText(230, 435, "LUNAR RESCUE MISSION", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(160, 380, "Collect all crystals, avoid meteors, and land safely.", GLUT_BITMAP_HELVETICA_18);
    drawText(160, 360, "The landing pad refuels your ship and repairs hull damage.", GLUT_BITMAP_HELVETICA_18);
    drawText(230, 325, "Controls", GLUT_BITMAP_HELVETICA_18);
    drawText(230, 300, "A/D or Left/Right arrows : Rotate ship");
    drawText(230, 280, "W or Up arrow             : Thrust");
    drawText(230, 260, "P                         : Pause");
    drawText(230, 240, "R                         : Restart");
    drawText(230, 220, "ESC                       : Exit");

    glColor3f(0.9f, 1.0f, 0.3f);
    drawText(285, 170, "Press ENTER to Start", GLUT_BITMAP_HELVETICA_18);

    // Decorative rotating lander
    glPushMatrix();
    glTranslatef(400, 110, 0);
    glRotatef((float)glutGet(GLUT_ELAPSED_TIME) * 0.04f, 0, 0, 1);
    glColor3f(0.25f, 0.75f, 1.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(0, 25);
    glVertex2f(-18, -15);
    glVertex2f(18, -15);
    glEnd();
    glColor3f(0.35f, 0.9f, 1.0f);
    drawOutlinedCircle(0, 0, 28, 40);
    glPopMatrix();
}

void drawEndScreen(bool won) {
    drawStars();
    drawTerrain();
    drawParticles();

    glColor3f(won ? 0.2f : 1.0f, won ? 1.0f : 0.2f, won ? 0.4f : 0.1f);
    drawText(won ? 280 : 300, 395, won ? "MISSION SUCCESS" : "MISSION FAILED", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(270, 350, endMessage, GLUT_BITMAP_HELVETICA_18);

    std::stringstream ss;
    ss << "Final Score: " << score;
    drawText(330, 315, ss.str(), GLUT_BITMAP_HELVETICA_18);

    ss.str(""); ss.clear();
    ss << "Crystals Collected: " << collectedCrystals << "/" << crystals.size();
    drawText(300, 290, ss.str());

    ss.str(""); ss.clear();
    ss << "Fuel Remaining: " << (int)lander.fuel << "%";
    drawText(322, 270, ss.str());

    glColor3f(0.9f, 1.0f, 0.3f);
    drawText(270, 210, "Press ENTER or R to play again", GLUT_BITMAP_HELVETICA_18);
    drawText(330, 185, "Press ESC to exit");
}

// ==========================================
// CONCEPT 3: Transformations
// Including object translation, screen shake composite matrices, and dynamic scaling.
// ==========================================
void drawScene() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (screenShake > 0.0f) {
        glTranslatef(randFloat(-screenShake, screenShake), randFloat(-screenShake, screenShake), 0.0f);
    }

    if (state == MENU) {
        drawMenu();
        glutSwapBuffers();
        return;
    }

    if (state == GAME_OVER) {
        drawEndScreen(false);
        glutSwapBuffers();
        return;
    }

    if (state == GAME_WON) {
        drawEndScreen(true);
        glutSwapBuffers();
        return;
    }

    drawStars();
    drawTerrain();
    drawCrystals();

    for (const Meteor& m : meteors) {
        drawMeteor(m);
    }

    drawThrustPreview();
    drawLander();
    drawParticles();
    drawFloatingTexts();
    drawHUD();

    if (state == PAUSED) {
        glColor3f(1.0f, 1.0f, 0.3f);
        drawText(350, 330, "PAUSED", GLUT_BITMAP_TIMES_ROMAN_24);
        drawText(300, 300, "Press P to continue", GLUT_BITMAP_HELVETICA_18);
    }

    glutSwapBuffers();
}

void respawnMeteor(Meteor& m) {
    m.x = randFloat(40.0f, WIN_W - 40.0f);
    m.y = randFloat(WIN_H + 40.0f, WIN_H + 240.0f);
    m.vx = randFloat(-0.9f, 0.9f);
    m.vy = randFloat(-2.6f, -1.1f);
    m.radius = randFloat(12.0f, 24.0f);
    m.angle = randFloat(0.0f, 360.0f);
    m.spin = randFloat(-3.0f, 3.0f);
}

void updateStars() {
    for (Star& s : stars) {
        s.y -= s.speed;
        if (s.y < 0) {
            s.y = WIN_H;
            s.x = randFloat(0.0f, WIN_W);
        }
    }
}

void updateParticles() {
    for (Particle& p : particles) {
        p.x += p.vx;
        p.y += p.vy;
        p.vx *= 0.98f;
        p.vy *= 0.98f;
        p.life -= 0.018f;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) {
        return p.life <= 0.0f;
    }), particles.end());
}

void updateFloatingTexts() {
    for (FloatingText& ft : floatingTexts) {
        ft.y += ft.vy;
        ft.life -= 0.014f;
    }
    floatingTexts.erase(std::remove_if(floatingTexts.begin(), floatingTexts.end(), [](const FloatingText& ft) {
        return ft.life <= 0.0f;
    }), floatingTexts.end());
}

void finishGame(bool won, const std::string& message) {
    if (won) {
        score = collectedCrystals * 100 + (int)lander.fuel * 2 + lander.hull * 50;
        state = GAME_WON;
    } else {
        score = collectedCrystals * 100;
        state = GAME_OVER;
    }
    endMessage = message;
    addExplosion(lander.x, lander.y, won ? 0.2f : 1.0f, won ? 1.0f : 0.3f, won ? 0.5f : 0.1f, won ? 20 : 45);
}

void updateLander() {
    // Rotation
    if (keys['a'] || keys['A'] || specialKeys[GLUT_KEY_LEFT]) {
        lander.angle -= 3.0f;
    }
    if (keys['d'] || keys['D'] || specialKeys[GLUT_KEY_RIGHT]) {
        lander.angle += 3.0f;
    }

    if (lander.angle > 180.0f) lander.angle -= 360.0f;
    if (lander.angle < -180.0f) lander.angle += 360.0f;

    bool thrusting = keys['w'] || keys['W'] || specialKeys[GLUT_KEY_UP];
    if (thrusting && lander.fuel > 0.0f) {
        float a = degToRad(lander.angle);
        float thrust = 0.12f;
        lander.vx += std::sin(a) * thrust;
        lander.vy += std::cos(a) * thrust;
        lander.fuel -= 0.20f;
        if (lander.fuel < 0.0f) lander.fuel = 0.0f;

        // Flame particles behind lander
        float backX = lander.x - std::sin(a) * 20.0f;
        float backY = lander.y - std::cos(a) * 20.0f;
        particles.push_back({backX, backY, randFloat(-0.8f, 0.8f) - std::sin(a) * 1.5f, randFloat(-0.8f, 0.8f) - std::cos(a) * 1.5f, 0.55f, 1.0f, 0.55f, 0.1f});
    }

    // Gravity and movement
    lander.vy -= 0.035f;
    lander.vx *= 0.995f;
    lander.vy *= 0.998f;

    lander.x += lander.vx;
    lander.y += lander.vy;

    if (lander.x < -20.0f || lander.x > WIN_W + 20.0f || lander.y > WIN_H + 80.0f) {
        finishGame(false, "You drifted away from the rescue zone.");
        return;
    }

    // Terrain / landing check
    float terrainY = getTerrainY(lander.x);
    isLandedOnPad = false;
    
    if (lander.y - 24.0f <= terrainY) {
        bool onPad = (lander.x >= PAD_X1 && lander.x <= PAD_X2);
        bool upright = std::fabs(lander.angle) <= 12.0f;
        bool slow = std::fabs(lander.vx) <= 1.3f && std::fabs(lander.vy) <= 1.7f;

        if (onPad && upright && slow) {
            lander.y = terrainY + 24.0f;
            lander.vx = 0.0f;
            lander.vy = 0.0f;
            isLandedOnPad = true;
            
            // Refuel (takes 5s to go 0->100, approx 0.33 per frame)
            if (lander.fuel < 100.0f) {
                lander.fuel += 100.0f / (5.0f * 60.0f);
                if (lander.fuel > 100.0f) lander.fuel = 100.0f;
            }
            
            // Repair
            repairTimer += 16.0f;
            if (repairTimer >= 5000.0f) {
                if (lander.hull < 3) {
                    lander.hull++;
                    addFloatingText(lander.x - 20.0f, lander.y + 40.0f, "REPAIRED!");
                    addExplosion(lander.x, lander.y, 0.2f, 1.0f, 0.4f, 15);
                }
                repairTimer = 0.0f;
            }
        } else {
            finishGame(false, "Crash landing. Too fast, tilted, or outside the pad.");
            return;
        }
    }
    
    if (!isLandedOnPad) {
        repairTimer = 0.0f;
    }
}

void updateMeteors() {
    for (Meteor& m : meteors) {
        m.x += m.vx;
        m.y += m.vy;
        m.angle += m.spin;

        if (m.y < -50.0f || m.x < -80.0f || m.x > WIN_W + 80.0f) {
            respawnMeteor(m);
        }

        if (!isLandedOnPad && distance2D(lander.x, lander.y, m.x, m.y) < LANDER_RADIUS + m.radius) {
            lander.hull--;
            screenShake = 7.0f;
            addExplosion(m.x, m.y, 1.0f, 0.45f, 0.1f, 25);
            addFloatingText(lander.x - 20.0f, lander.y + 25.0f, "HIT!");
            respawnMeteor(m);
            if (lander.hull <= 0) {
                finishGame(false, "Your lander was destroyed by meteors.");
                return;
            }
        }
    }
}

void updateCrystals() {
    for (Crystal& c : crystals) {
        if (c.collected) continue;
        if (distance2D(lander.x, lander.y, c.x, c.y) < LANDER_RADIUS + 14.0f) {
            c.collected = true;
            collectedCrystals++;
            addExplosion(c.x, c.y, 0.2f, 1.0f, 0.9f, 18);
            addFloatingText(c.x - 25.0f, c.y + 20.0f, "+100");
            
            if (collectedCrystals == (int)crystals.size()) {
                finishGame(true, "All crystals collected! VICTORY!");
                // Victory Celebration
                addExplosion(lander.x, lander.y, 0.2f, 1.0f, 0.9f, 60);
                addExplosion(lander.x - 30.0f, lander.y + 20.0f, 1.0f, 0.9f, 0.2f, 40);
                addExplosion(lander.x + 30.0f, lander.y + 20.0f, 1.0f, 0.2f, 0.9f, 40);
                addFloatingText(lander.x - 40.0f, lander.y + 40.0f, "VICTORY!");
            }
        }
    }
}

void updateGame() {
    updateStars();
    updateParticles();
    updateFloatingTexts();

    if (screenShake > 0.0f) {
        screenShake *= 0.85f;
        if (screenShake < 0.2f) screenShake = 0.0f;
    }

    if (state != PLAYING) return;

    updateLander();
    if (state != PLAYING) return;

    updateMeteors();
    if (state != PLAYING) return;

    updateCrystals();
}

// ==========================================
// CONCEPT 5: Animation Principles (Timing & Staging)
// Framerate management, background staging (parallax stars), and reaction feedback loops.
// ==========================================
void timer(int) {
    updateGame();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyDown(unsigned char key, int, int) {
    keys[(unsigned char)key] = true;

    if (key == 27) {
        std::exit(0);
    }

    if (key == 13) { // ENTER
        if (state == MENU || state == GAME_OVER || state == GAME_WON) {
            resetGame();
            state = PLAYING;
        }
    }

    if (key == 'r' || key == 'R') {
        resetGame();
        state = PLAYING;
    }

    if (key == 'p' || key == 'P') {
        if (state == PLAYING) state = PAUSED;
        else if (state == PAUSED) state = PLAYING;
    }
}

void keyUp(unsigned char key, int, int) {
    keys[(unsigned char)key] = false;
}

void specialDown(int key, int, int) {
    if (key >= 0 && key < 256) specialKeys[key] = true;
}

void specialUp(int key, int, int) {
    if (key >= 0 && key < 256) specialKeys[key] = false;
}

// ==========================================
// CONCEPT 1: Viewing & Clipping
// Using gluOrtho2D viewport configurations and boundary clamps.
// ==========================================
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
}

void initOpenGL() {
    glClearColor(0.01f, 0.01f, 0.04f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_DEPTH_TEST);
}

int main(int argc, char** argv) {
    srand((unsigned int)time(nullptr));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(120, 80);
    glutCreateWindow("Lunar Rescue Mission - OpenGL CG Project");

    initOpenGL();
    initStars();
    initTerrain();
    resetGame();
    state = MENU;

    glutDisplayFunc(drawScene);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
