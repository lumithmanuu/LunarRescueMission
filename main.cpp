/*
 * Lunar Rescue Mission
 * --------------------
 * A beginner-friendly 2D OpenGL / GLUT game for a Computer Graphics module.
 *
 * Project goals demonstrated in this file:
 * - translation, rotation, and scaling
 * - animation and timer-based updates
 * - collision detection
 * - keyboard interaction
 * - particles and visual feedback
 * - multiple game states and UI screens
 *
 * Controls:
 *   W / Up Arrow           - thrust
 *   A / Left Arrow         - rotate left
 *   D / Right Arrow        - rotate right
 *   Enter                  - select menu item / confirm
 *   P                      - pause
 *   R                      - restart mission
 *   M                      - return to main menu
 *   ESC                    - back / exit
 */

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/glu.h>
#include <GL/freeglut.h>
#endif

const int WIN_W = 800;
const int WIN_H = 600;
const float PI = 3.1415926535f;
const float LANDER_RADIUS = 18.0f;
const float PAD_X1 = 330.0f;
const float PAD_X2 = 470.0f;
const float PAD_Y = 105.0f;
const float SAFE_LANDING_VX = 1.0f;
const float SAFE_LANDING_VY = 1.45f;
const float SAFE_LANDING_ANGLE = 10.0f;

struct Vec2 {
    float x;
    float y;
};

struct Star {
    float x;
    float y;
    float speed;
    float size;
    float brightness;
    int layer;
};

struct Particle {
    float x;
    float y;
    float vx;
    float vy;
    float life;
    float size;
    float r;
    float g;
    float b;
};

struct Crystal {
    float x;
    float y;
    bool collected;
};

struct Meteor {
    float x;
    float y;
    float vx;
    float vy;
    float radius;
    float angle;
    float spin;
    float swayPhase;
};

struct FloatingText {
    float x;
    float y;
    float vy;
    float life;
    float r;
    float g;
    float b;
    std::string text;
};

struct Lander {
    float x;
    float y;
    float vx;
    float vy;
    float angle;
    float fuel;
    int health;
};

enum GameState {
    MAIN_MENU,
    INSTRUCTIONS,
    PLAYING,
    PAUSED,
    GAME_OVER,
    GAME_WON,
    CREDITS
};

GameState state = MAIN_MENU;
Lander lander;

bool keys[256] = {false};
bool specialKeys[256] = {false};

std::vector<Star> stars;
std::vector<Particle> particles;
std::vector<Crystal> crystals;
std::vector<Meteor> meteors;
std::vector<FloatingText> floatingTexts;
std::vector<Vec2> terrain;

int menuSelection = 0;
int score = 0;
int lives = 3;
int collectedCrystals = 0;
float missionTime = 0.0f;
float screenShake = 0.0f;
float statusMessageTimer = 0.0f;
std::string endMessage = "";
std::string statusMessage = "";

const char* MENU_ITEMS[] = {
    "Start Game",
    "Instructions",
    "Credits / Team Contributions",
    "Exit"
};
const int MENU_ITEM_COUNT = 4;

float randFloat(float minV, float maxV) {
    return minV + (maxV - minV) * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
}

float degToRad(float degrees) {
    return degrees * PI / 180.0f;
}

float distance2D(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    return std::sqrt(dx * dx + dy * dy);
}

float clampFloat(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}

// Basic text and shape helpers keep the rest of the code readable for beginners.
void drawText(float x, float y, const std::string& text, void* font = GLUT_BITMAP_8_BY_13) {
    glRasterPos2f(x, y);
    for (size_t i = 0; i < text.size(); ++i) {
        glutBitmapCharacter(font, text[i]);
    }
}

void drawRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawFilledCircle(float cx, float cy, float radius, int segments = 32) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
        glVertex2f(cx + std::cos(angle) * radius, cy + std::sin(angle) * radius);
    }
    glEnd();
}

void drawOutlinedCircle(float cx, float cy, float radius, int segments = 32) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
        glVertex2f(cx + std::cos(angle) * radius, cy + std::sin(angle) * radius);
    }
    glEnd();
}

void addFloatingText(float x, float y, const std::string& text, float r, float g, float b) {
    FloatingText ft;
    ft.x = x;
    ft.y = y;
    ft.vy = 0.6f;
    ft.life = 1.0f;
    ft.r = r;
    ft.g = g;
    ft.b = b;
    ft.text = text;
    floatingTexts.push_back(ft);
}

void addStatusMessage(const std::string& message) {
    statusMessage = message;
    statusMessageTimer = 2.2f;
}

void addExplosion(float x, float y, float r, float g, float b, int count, float speedScale) {
    for (int i = 0; i < count; ++i) {
        float angle = randFloat(0.0f, 2.0f * PI);
        float speed = randFloat(0.8f, speedScale);
        Particle particle;
        particle.x = x;
        particle.y = y;
        particle.vx = std::cos(angle) * speed;
        particle.vy = std::sin(angle) * speed;
        particle.life = randFloat(0.4f, 1.0f);
        particle.size = randFloat(2.0f, 5.5f);
        particle.r = r;
        particle.g = g;
        particle.b = b;
        particles.push_back(particle);
    }
}

void addThrustParticles() {
    float angle = degToRad(lander.angle);
    for (int i = 0; i < 2; ++i) {
        Particle particle;
        particle.x = lander.x - std::sin(angle) * 20.0f + randFloat(-2.5f, 2.5f);
        particle.y = lander.y - std::cos(angle) * 20.0f + randFloat(-2.5f, 2.5f);
        particle.vx = randFloat(-0.5f, 0.5f) - std::sin(angle) * randFloat(1.6f, 2.4f);
        particle.vy = randFloat(-0.5f, 0.5f) - std::cos(angle) * randFloat(1.6f, 2.4f);
        particle.life = randFloat(0.35f, 0.55f);
        particle.size = randFloat(2.0f, 4.0f);
        particle.r = 1.0f;
        particle.g = randFloat(0.45f, 0.8f);
        particle.b = 0.1f;
        particles.push_back(particle);
    }
}

void initStars() {
    stars.clear();
    for (int i = 0; i < 140; ++i) {
        Star star;
        star.x = randFloat(0.0f, static_cast<float>(WIN_W));
        star.y = randFloat(0.0f, static_cast<float>(WIN_H));
        star.layer = 1 + (i % 3);
        star.speed = 0.12f * static_cast<float>(star.layer) + randFloat(0.05f, 0.25f);
        star.size = 1.0f + static_cast<float>(star.layer) * 0.6f;
        star.brightness = randFloat(0.55f, 1.0f);
        stars.push_back(star);
    }
}

void initTerrain() {
    terrain.clear();
    terrain.push_back((Vec2){0.0f, 60.0f});
    terrain.push_back((Vec2){70.0f, 84.0f});
    terrain.push_back((Vec2){160.0f, 70.0f});
    terrain.push_back((Vec2){230.0f, 125.0f});
    terrain.push_back((Vec2){315.0f, 92.0f});
    terrain.push_back((Vec2){PAD_X1, PAD_Y});
    terrain.push_back((Vec2){PAD_X2, PAD_Y});
    terrain.push_back((Vec2){520.0f, 95.0f});
    terrain.push_back((Vec2){585.0f, 150.0f});
    terrain.push_back((Vec2){665.0f, 90.0f});
    terrain.push_back((Vec2){735.0f, 118.0f});
    terrain.push_back((Vec2){800.0f, 78.0f});
}

float getTerrainY(float x) {
    if (terrain.empty()) {
        return 0.0f;
    }

    if (x <= terrain.front().x) {
        return terrain.front().y;
    }
    if (x >= terrain.back().x) {
        return terrain.back().y;
    }

    for (size_t i = 0; i + 1 < terrain.size(); ++i) {
        const Vec2& a = terrain[i];
        const Vec2& b = terrain[i + 1];
        if (x >= a.x && x <= b.x) {
            float t = (x - a.x) / (b.x - a.x);
            return a.y + t * (b.y - a.y);
        }
    }
    return 0.0f;
}

void resetCrystals() {
    crystals.clear();
    crystals.push_back((Crystal){110.0f, 225.0f, false});
    crystals.push_back((Crystal){250.0f, 375.0f, false});
    crystals.push_back((Crystal){390.0f, 285.0f, false});
    crystals.push_back((Crystal){545.0f, 215.0f, false});
    crystals.push_back((Crystal){690.0f, 445.0f, false});
    collectedCrystals = 0;
}

void respawnMeteor(Meteor& meteor, bool fromTopOnly) {
    meteor.x = randFloat(40.0f, static_cast<float>(WIN_W) - 40.0f);
    meteor.y = fromTopOnly ? randFloat(WIN_H + 20.0f, WIN_H + 220.0f)
                           : randFloat(190.0f, WIN_H + 220.0f);
    meteor.vx = randFloat(-0.8f, 0.8f);
    meteor.vy = randFloat(-2.5f, -1.1f);
    meteor.radius = randFloat(13.0f, 24.0f);
    meteor.angle = randFloat(0.0f, 360.0f);
    meteor.spin = randFloat(-2.5f, 2.5f);
    meteor.swayPhase = randFloat(0.0f, 6.28f);
}

void resetMeteors() {
    meteors.clear();
    for (int i = 0; i < 6; ++i) {
        Meteor meteor;
        respawnMeteor(meteor, false);
        meteors.push_back(meteor);
    }
}

void resetLanderAtSpawn() {
    lander.x = 120.0f;
    lander.y = 520.0f;
    lander.vx = 0.0f;
    lander.vy = 0.0f;
    lander.angle = 0.0f;
    lander.health = 100;
}

// A full mission reset is used when the player starts or restarts the game.
void startNewGame() {
    score = 0;
    lives = 3;
    missionTime = 0.0f;
    screenShake = 0.0f;
    endMessage = "";
    statusMessage = "";
    statusMessageTimer = 0.0f;
    particles.clear();
    floatingTexts.clear();
    resetCrystals();
    resetMeteors();
    resetLanderAtSpawn();
    lander.fuel = 100.0f;
    state = PLAYING;
    addStatusMessage("Collect every crystal, then land on the pad.");
}

void restartCurrentMission() {
    startNewGame();
}

void finishGame(bool won, const std::string& message) {
    endMessage = message;
    if (won) {
        int landingBonus = 600;
        int fuelBonus = static_cast<int>(lander.fuel) * 3;
        int healthBonus = lander.health * 2;
        int lifeBonus = lives * 100;
        int timeBonus = std::max(0, 300 - static_cast<int>(missionTime) * 5);
        score += landingBonus + fuelBonus + healthBonus + lifeBonus + timeBonus;
        state = GAME_WON;
        addExplosion(lander.x, lander.y, 0.25f, 1.0f, 0.55f, 35, 4.8f);
    } else {
        state = GAME_OVER;
        addExplosion(lander.x, lander.y, 1.0f, 0.35f, 0.1f, 55, 5.6f);
    }
}

void handleCrash(const std::string& reason) {
    screenShake = 8.0f;
    addExplosion(lander.x, lander.y, 1.0f, 0.4f, 0.08f, 42, 5.0f);
    addFloatingText(lander.x - 24.0f, lander.y + 20.0f, "CRASH!", 1.0f, 0.35f, 0.15f);
    score = std::max(0, score - 80);
    lives--;

    if (lives <= 0) {
        finishGame(false, reason);
        return;
    }

    resetLanderAtSpawn();
    lander.fuel = std::max(lander.fuel - 12.0f, 25.0f);
    addStatusMessage(reason + " Respawning...");
}

void drawGradientSky() {
    glBegin(GL_QUADS);
    glColor3f(0.01f, 0.02f, 0.08f);
    glVertex2f(0.0f, WIN_H);
    glVertex2f(static_cast<float>(WIN_W), WIN_H);
    glColor3f(0.03f, 0.04f, 0.13f);
    glVertex2f(static_cast<float>(WIN_W), 240.0f);
    glVertex2f(0.0f, 240.0f);
    glEnd();

    glBegin(GL_QUADS);
    glColor4f(0.05f, 0.14f, 0.25f, 0.25f);
    glVertex2f(0.0f, 420.0f);
    glVertex2f(static_cast<float>(WIN_W), 420.0f);
    glColor4f(0.02f, 0.08f, 0.16f, 0.0f);
    glVertex2f(static_cast<float>(WIN_W), 250.0f);
    glVertex2f(0.0f, 250.0f);
    glEnd();
}

// The background is split into layers so the presentation can explain parallax-style motion.
void drawStars() {
    for (int layer = 1; layer <= 3; ++layer) {
        glPointSize(1.0f + static_cast<float>(layer));
        glBegin(GL_POINTS);
        for (size_t i = 0; i < stars.size(); ++i) {
            const Star& star = stars[i];
            if (star.layer != layer) {
                continue;
            }
            float tint = star.brightness - 0.1f * static_cast<float>(layer - 1);
            glColor3f(tint, tint, std::min(1.0f, tint + 0.08f));
            glVertex2f(star.x, star.y);
        }
        glEnd();
    }
}

void drawMoonAndNebula() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.9f, 0.92f, 1.0f, 0.12f);
    drawFilledCircle(645.0f, 470.0f, 92.0f, 48);
    glColor4f(0.78f, 0.82f, 0.9f, 0.9f);
    drawFilledCircle(650.0f, 468.0f, 72.0f, 48);

    glColor4f(0.67f, 0.72f, 0.78f, 0.8f);
    drawFilledCircle(625.0f, 492.0f, 12.0f, 20);
    drawFilledCircle(680.0f, 445.0f, 15.0f, 20);
    drawFilledCircle(660.0f, 510.0f, 10.0f, 18);

    glColor4f(0.2f, 0.55f, 0.75f, 0.08f);
    drawFilledCircle(175.0f, 470.0f, 118.0f, 40);
    glColor4f(0.1f, 0.3f, 0.5f, 0.05f);
    drawFilledCircle(230.0f, 405.0f, 94.0f, 40);

    glDisable(GL_BLEND);
}

void drawBackground() {
    drawGradientSky();
    drawMoonAndNebula();
    drawStars();
}

void drawTerrain() {
    glBegin(GL_POLYGON);
    glColor3f(0.10f, 0.11f, 0.14f);
    glVertex2f(0.0f, 0.0f);
    for (size_t i = 0; i < terrain.size(); ++i) {
        glVertex2f(terrain[i].x, terrain[i].y);
    }
    glVertex2f(static_cast<float>(WIN_W), 0.0f);
    glEnd();

    glLineWidth(2.0f);
    glColor3f(0.57f, 0.58f, 0.66f);
    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i < terrain.size(); ++i) {
        glVertex2f(terrain[i].x, terrain[i].y);
    }
    glEnd();
    glLineWidth(1.0f);

    glColor3f(0.09f, 0.12f, 0.16f);
    drawFilledCircle(135.0f, 58.0f, 26.0f, 28);
    drawFilledCircle(235.0f, 74.0f, 18.0f, 24);
    drawFilledCircle(600.0f, 80.0f, 24.0f, 24);

    glColor3f(0.12f, 0.78f, 0.56f);
    drawRect(PAD_X1, PAD_Y - 4.0f, PAD_X2 - PAD_X1, 8.0f);
    glColor3f(0.88f, 1.0f, 0.65f);
    drawText(PAD_X1 + 28.0f, PAD_Y + 14.0f, "LANDING PAD");

    float blink = 0.65f + 0.35f * std::sin(static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.01f);
    glColor3f(0.0f, blink, 0.45f);
    drawFilledCircle(PAD_X1 + 10.0f, PAD_Y + 8.0f, 4.0f, 16);
    drawFilledCircle(PAD_X2 - 10.0f, PAD_Y + 8.0f, 4.0f, 16);
}

// Crystals are the main collectibles and also reward a small fuel refill.
void drawCrystal(const Crystal& crystal) {
    float pulse = 1.0f + 0.16f * std::sin(static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.006f + crystal.x * 0.03f);
    glPushMatrix();
    glTranslatef(crystal.x, crystal.y, 0.0f);
    glRotatef(static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.08f, 0.0f, 0.0f, 1.0f);
    glScalef(pulse, pulse, 1.0f);

    glColor3f(0.15f, 1.0f, 0.88f);
    glBegin(GL_POLYGON);
    glVertex2f(0.0f, 14.0f);
    glVertex2f(10.0f, 0.0f);
    glVertex2f(0.0f, -14.0f);
    glVertex2f(-10.0f, 0.0f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.0f, 14.0f);
    glVertex2f(10.0f, 0.0f);
    glVertex2f(0.0f, -14.0f);
    glVertex2f(-10.0f, 0.0f);
    glEnd();
    glPopMatrix();
}

void drawCrystals() {
    for (size_t i = 0; i < crystals.size(); ++i) {
        if (!crystals[i].collected) {
            drawCrystal(crystals[i]);
        }
    }
}

void drawMeteor(const Meteor& meteor) {
    glPushMatrix();
    glTranslatef(meteor.x, meteor.y, 0.0f);
    glRotatef(meteor.angle, 0.0f, 0.0f, 1.0f);

    glColor3f(0.52f, 0.34f, 0.16f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 10; ++i) {
        float angle = 2.0f * PI * static_cast<float>(i) / 10.0f;
        float radius = meteor.radius * (0.78f + 0.18f * std::sin(static_cast<float>(i) * 2.3f));
        glVertex2f(std::cos(angle) * radius, std::sin(angle) * radius);
    }
    glEnd();

    glColor3f(0.88f, 0.61f, 0.33f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 10; ++i) {
        float angle = 2.0f * PI * static_cast<float>(i) / 10.0f;
        float radius = meteor.radius * (0.78f + 0.18f * std::sin(static_cast<float>(i) * 2.3f));
        glVertex2f(std::cos(angle) * radius, std::sin(angle) * radius);
    }
    glEnd();

    glColor3f(0.28f, 0.18f, 0.08f);
    drawFilledCircle(-meteor.radius * 0.18f, meteor.radius * 0.12f, meteor.radius * 0.18f, 14);
    drawFilledCircle(meteor.radius * 0.22f, -meteor.radius * 0.16f, meteor.radius * 0.12f, 14);
    glPopMatrix();
}

void drawMeteors() {
    for (size_t i = 0; i < meteors.size(); ++i) {
        drawMeteor(meteors[i]);
    }
}

void drawParticles() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (size_t i = 0; i < particles.size(); ++i) {
        const Particle& particle = particles[i];
        glColor4f(particle.r, particle.g, particle.b, particle.life);
        drawFilledCircle(particle.x, particle.y, particle.size, 12);
    }
    glDisable(GL_BLEND);
}

void drawFloatingTexts() {
    for (size_t i = 0; i < floatingTexts.size(); ++i) {
        const FloatingText& text = floatingTexts[i];
        glColor3f(text.r, text.g, text.b);
        drawText(text.x, text.y, text.text, GLUT_BITMAP_HELVETICA_18);
    }
}

void drawLanderBody() {
    if (lander.health > 60) {
        glColor3f(0.28f, 0.78f, 1.0f);
    } else if (lander.health > 25) {
        glColor3f(1.0f, 0.78f, 0.18f);
    } else {
        glColor3f(1.0f, 0.30f, 0.22f);
    }

    glBegin(GL_TRIANGLES);
    glVertex2f(0.0f, 22.0f);
    glVertex2f(-16.0f, -13.0f);
    glVertex2f(16.0f, -13.0f);
    glEnd();

    glColor3f(0.12f, 0.13f, 0.18f);
    drawFilledCircle(0.0f, 4.0f, 5.0f, 18);

    glColor3f(0.85f, 0.85f, 0.9f);
    glBegin(GL_LINES);
    glVertex2f(-9.0f, -12.0f); glVertex2f(-21.0f, -24.0f);
    glVertex2f(9.0f, -12.0f);  glVertex2f(21.0f, -24.0f);
    glVertex2f(-21.0f, -24.0f); glVertex2f(-9.0f, -24.0f);
    glVertex2f(21.0f, -24.0f);  glVertex2f(9.0f, -24.0f);
    glEnd();

    glColor3f(0.35f, 0.9f, 1.0f);
    drawOutlinedCircle(0.0f, 0.0f, LANDER_RADIUS + 4.0f, 40);
}

void drawThrustFlame() {
    bool thrusting = (keys['w'] || keys['W'] || specialKeys[GLUT_KEY_UP]) && state == PLAYING && lander.fuel > 0.0f;
    if (!thrusting) {
        return;
    }

    float flicker = 28.0f + 7.0f * std::sin(static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.04f);
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.95f, 0.55f);
    glVertex2f(-7.0f, -15.0f);
    glVertex2f(7.0f, -15.0f);
    glColor3f(1.0f, 0.25f, 0.05f);
    glVertex2f(0.0f, -flicker);
    glEnd();

    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.85f, 0.2f);
    glVertex2f(-4.0f, -15.0f);
    glVertex2f(4.0f, -15.0f);
    glColor3f(1.0f, 0.55f, 0.1f);
    glVertex2f(0.0f, -flicker + 7.0f);
    glEnd();
}

void drawLander() {
    glPushMatrix();
    glTranslatef(lander.x, lander.y, 0.0f);
    glRotatef(-lander.angle, 0.0f, 0.0f, 1.0f);
    drawThrustFlame();
    drawLanderBody();
    glPopMatrix();
}

void drawBar(float x, float y, float width, float value, float maxValue, float r, float g, float b) {
    glColor3f(0.20f, 0.22f, 0.28f);
    drawRect(x, y, width, 14.0f);
    float normalized = clampFloat(value / maxValue, 0.0f, 1.0f);
    glColor3f(r, g, b);
    drawRect(x, y, width * normalized, 14.0f);
}

// The HUD keeps the important gameplay values visible at all times.
void drawHUD() {
    std::stringstream ss;

    glColor3f(1.0f, 1.0f, 1.0f);
    ss << "Score: " << score;
    drawText(18.0f, WIN_H - 24.0f, ss.str(), GLUT_BITMAP_HELVETICA_18);

    ss.str("");
    ss.clear();
    ss << "Crystals: " << collectedCrystals << "/" << crystals.size();
    drawText(18.0f, WIN_H - 48.0f, ss.str(), GLUT_BITMAP_HELVETICA_18);

    ss.str("");
    ss.clear();
    ss << "Lives: " << lives;
    drawText(18.0f, WIN_H - 72.0f, ss.str());

    ss.str("");
    ss.clear();
    ss << "Speed X: " << static_cast<int>(lander.vx * 10.0f) / 10.0f
       << "   Y: " << static_cast<int>(lander.vy * 10.0f) / 10.0f;
    drawText(18.0f, WIN_H - 92.0f, ss.str());

    ss.str("");
    ss.clear();
    ss << "Angle: " << static_cast<int>(lander.angle) << " deg";
    drawText(18.0f, WIN_H - 112.0f, ss.str());

    ss.str("");
    ss.clear();
    ss << "Time: " << static_cast<int>(missionTime) << " s";
    drawText(18.0f, WIN_H - 132.0f, ss.str());

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(WIN_W - 238.0f, WIN_H - 24.0f, "Fuel");
    drawBar(WIN_W - 180.0f, WIN_H - 34.0f, 150.0f, lander.fuel, 100.0f, 0.24f, 0.90f, 0.46f);

    drawText(WIN_W - 238.0f, WIN_H - 58.0f, "Health");
    drawBar(WIN_W - 180.0f, WIN_H - 68.0f, 150.0f, static_cast<float>(lander.health), 100.0f, 1.0f, 0.55f, 0.18f);

    glColor3f(0.75f, 0.95f, 1.0f);
    drawText(WIN_W - 300.0f, 40.0f, "Win condition: collect all crystals and land slowly on the pad.");
    drawText(WIN_W - 300.0f, 22.0f, "Safe landing: |vx| <= 1.0, |vy| <= 1.45, angle <= 10 deg.");

    if (statusMessageTimer > 0.0f) {
        glColor3f(1.0f, 0.94f, 0.55f);
        drawText(220.0f, WIN_H - 24.0f, statusMessage, GLUT_BITMAP_HELVETICA_18);
    }
}

void drawPanel(float x, float y, float w, float h) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.03f, 0.05f, 0.10f, 0.82f);
    drawRect(x, y, w, h);
    glDisable(GL_BLEND);

    glColor3f(0.35f, 0.85f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawMainMenu() {
    drawBackground();
    drawPanel(160.0f, 95.0f, 480.0f, 405.0f);

    glColor3f(0.23f, 0.92f, 1.0f);
    drawText(248.0f, 450.0f, "LUNAR RESCUE MISSION", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(0.94f, 0.97f, 1.0f);
    drawText(212.0f, 415.0f, "Navigate the lander, collect rescue crystals, and touch down safely.", GLUT_BITMAP_HELVETICA_18);
    drawText(228.0f, 393.0f, "A Computer Graphics project using only OpenGL / GLUT primitives.", GLUT_BITMAP_HELVETICA_18);

    for (int i = 0; i < MENU_ITEM_COUNT; ++i) {
        float itemY = 330.0f - static_cast<float>(i) * 52.0f;
        if (i == menuSelection) {
            glColor3f(0.16f, 0.40f, 0.55f);
            drawRect(220.0f, itemY - 18.0f, 360.0f, 28.0f);
            glColor3f(1.0f, 1.0f, 0.72f);
            drawText(244.0f, itemY, std::string("> ") + MENU_ITEMS[i], GLUT_BITMAP_HELVETICA_18);
        } else {
            glColor3f(0.88f, 0.92f, 1.0f);
            drawText(244.0f, itemY, MENU_ITEMS[i], GLUT_BITMAP_HELVETICA_18);
        }
    }

    glColor3f(0.72f, 0.88f, 1.0f);
    drawText(222.0f, 132.0f, "Use Up / Down arrows or W / S to move in the menu.", GLUT_BITMAP_HELVETICA_18);
    drawText(248.0f, 108.0f, "Press Enter to select. Press ESC from here to exit.", GLUT_BITMAP_HELVETICA_18);
}

void drawInstructionsScreen() {
    drawBackground();
    drawPanel(120.0f, 85.0f, 560.0f, 430.0f);

    glColor3f(0.22f, 0.92f, 1.0f);
    drawText(330.0f, 470.0f, "INSTRUCTIONS", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(0.94f, 0.97f, 1.0f);
    drawText(152.0f, 430.0f, "Mission Goal", GLUT_BITMAP_HELVETICA_18);
    drawText(152.0f, 405.0f, "Collect all five energy crystals, avoid meteors, save fuel, and land on the pad.");

    drawText(152.0f, 365.0f, "Controls", GLUT_BITMAP_HELVETICA_18);
    drawText(152.0f, 340.0f, "W / Up Arrow     : Fire main thruster");
    drawText(152.0f, 318.0f, "A / Left Arrow   : Rotate lander left");
    drawText(152.0f, 296.0f, "D / Right Arrow  : Rotate lander right");
    drawText(152.0f, 274.0f, "P                : Pause / resume");
    drawText(152.0f, 252.0f, "R                : Restart mission");
    drawText(152.0f, 230.0f, "M or ESC         : Return to main menu");

    drawText(152.0f, 190.0f, "Safe Landing Rules", GLUT_BITMAP_HELVETICA_18);
    drawText(152.0f, 165.0f, "1. Touch the green landing pad.");
    drawText(152.0f, 143.0f, "2. Keep the ship nearly upright.");
    drawText(152.0f, 121.0f, "3. Land with low horizontal and vertical speed.");

    glColor3f(1.0f, 0.95f, 0.55f);
    drawText(208.0f, 98.0f, "Press ENTER, M, or ESC to go back to the main menu.", GLUT_BITMAP_HELVETICA_18);
}

void drawCreditsScreen() {
    drawBackground();
    drawPanel(120.0f, 80.0f, 560.0f, 440.0f);

    glColor3f(0.22f, 0.92f, 1.0f);
    drawText(280.0f, 475.0f, "CREDITS / TEAM CONTRIBUTIONS", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(0.94f, 0.97f, 1.0f);
    drawText(150.0f, 430.0f, "Suggested 5-member breakdown", GLUT_BITMAP_HELVETICA_18);
    drawText(150.0f, 398.0f, "1. Member 1: Lander drawing, thrust flame, and ship controls");
    drawText(150.0f, 370.0f, "2. Member 2: Physics, gravity, fuel usage, and landing rules");
    drawText(150.0f, 342.0f, "3. Member 3: Terrain, landing pad, and collision detection");
    drawText(150.0f, 314.0f, "4. Member 4: Meteors, crystals, particles, and visual effects");
    drawText(150.0f, 286.0f, "5. Member 5: Menu screens, HUD, score system, README, presentation");

    drawText(150.0f, 238.0f, "Presentation focus", GLUT_BITMAP_HELVETICA_18);
    drawText(150.0f, 214.0f, "- Explain how translation updates object positions every frame.");
    drawText(150.0f, 192.0f, "- Show rotation around the lander center and meteor spin.");
    drawText(150.0f, 170.0f, "- Describe collision checks for terrain, crystals, and meteors.");
    drawText(150.0f, 148.0f, "- Highlight particles, animation, game states, and HUD design.");

    glColor3f(1.0f, 0.95f, 0.55f);
    drawText(222.0f, 102.0f, "Press ENTER, M, or ESC to return to the main menu.", GLUT_BITMAP_HELVETICA_18);
}

void drawPausedOverlay() {
    drawPanel(255.0f, 235.0f, 290.0f, 110.0f);
    glColor3f(1.0f, 1.0f, 0.40f);
    drawText(360.0f, 304.0f, "PAUSED", GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0.92f, 0.95f, 1.0f);
    drawText(300.0f, 274.0f, "Press P to resume or M for menu.", GLUT_BITMAP_HELVETICA_18);
}

void drawEndScreen(bool won) {
    drawBackground();
    drawTerrain();
    drawParticles();

    drawPanel(180.0f, 160.0f, 440.0f, 250.0f);

    glColor3f(won ? 0.28f : 1.0f, won ? 1.0f : 0.30f, won ? 0.45f : 0.12f);
    drawText(won ? 292.0f : 316.0f, 365.0f, won ? "MISSION SUCCESS" : "MISSION FAILED", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(0.94f, 0.97f, 1.0f);
    drawText(238.0f, 330.0f, endMessage, GLUT_BITMAP_HELVETICA_18);

    std::stringstream ss;
    ss << "Final Score: " << score;
    drawText(332.0f, 294.0f, ss.str(), GLUT_BITMAP_HELVETICA_18);

    ss.str("");
    ss.clear();
    ss << "Crystals Collected: " << collectedCrystals << "/" << crystals.size();
    drawText(295.0f, 266.0f, ss.str(), GLUT_BITMAP_HELVETICA_18);

    ss.str("");
    ss.clear();
    ss << "Fuel Remaining: " << static_cast<int>(lander.fuel) << "%";
    drawText(318.0f, 238.0f, ss.str(), GLUT_BITMAP_HELVETICA_18);

    glColor3f(1.0f, 0.95f, 0.55f);
    drawText(240.0f, 200.0f, "Press ENTER to play again or M to return to the menu.", GLUT_BITMAP_HELVETICA_18);
}

void drawScene() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (screenShake > 0.0f && (state == PLAYING || state == PAUSED)) {
        glTranslatef(randFloat(-screenShake, screenShake), randFloat(-screenShake, screenShake), 0.0f);
    }

    if (state == MAIN_MENU) {
        drawMainMenu();
        glutSwapBuffers();
        return;
    }

    if (state == INSTRUCTIONS) {
        drawInstructionsScreen();
        glutSwapBuffers();
        return;
    }

    if (state == CREDITS) {
        drawCreditsScreen();
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

    drawBackground();
    drawTerrain();
    drawCrystals();
    drawMeteors();
    drawLander();
    drawParticles();
    drawFloatingTexts();
    drawHUD();

    if (state == PAUSED) {
        drawPausedOverlay();
    }

    glutSwapBuffers();
}

// Particle and text updates are separated so each effect can be discussed independently.
void updateStars() {
    for (size_t i = 0; i < stars.size(); ++i) {
        stars[i].y -= stars[i].speed;
        if (stars[i].y < -2.0f) {
            stars[i].y = WIN_H + randFloat(0.0f, 40.0f);
            stars[i].x = randFloat(0.0f, static_cast<float>(WIN_W));
        }
    }
}

void updateParticles() {
    for (size_t i = 0; i < particles.size(); ++i) {
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        particles[i].vy -= 0.01f;
        particles[i].vx *= 0.985f;
        particles[i].vy *= 0.985f;
        particles[i].life -= 0.018f;
        particles[i].size *= 0.992f;
    }

    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& particle) {
            return particle.life <= 0.0f || particle.size <= 0.4f;
        }), particles.end());
}

void updateFloatingTexts() {
    for (size_t i = 0; i < floatingTexts.size(); ++i) {
        floatingTexts[i].y += floatingTexts[i].vy;
        floatingTexts[i].life -= 0.018f;
    }

    floatingTexts.erase(std::remove_if(floatingTexts.begin(), floatingTexts.end(),
        [](const FloatingText& text) {
            return text.life <= 0.0f;
        }), floatingTexts.end());
}

void updateMeteors() {
    float elapsed = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.001f;

    for (size_t i = 0; i < meteors.size(); ++i) {
        Meteor& meteor = meteors[i];
        meteor.x += meteor.vx + std::sin(elapsed * 2.0f + meteor.swayPhase) * 0.12f;
        meteor.y += meteor.vy;
        meteor.angle += meteor.spin;

        if (meteor.y < -60.0f || meteor.x < -90.0f || meteor.x > WIN_W + 90.0f) {
            respawnMeteor(meteor, true);
            continue;
        }

        if (distance2D(lander.x, lander.y, meteor.x, meteor.y) < LANDER_RADIUS + meteor.radius) {
            lander.health = std::max(0, lander.health - 34);
            score = std::max(0, score - 25);
            screenShake = 6.0f;
            addExplosion(meteor.x, meteor.y, 1.0f, 0.5f, 0.12f, 24, 4.0f);
            addFloatingText(lander.x - 18.0f, lander.y + 26.0f, "-34 HP", 1.0f, 0.55f, 0.2f);
            respawnMeteor(meteor, true);

            if (lander.health <= 0) {
                handleCrash("The lander was destroyed by repeated meteor impacts.");
                return;
            }
        }
    }
}

void updateCrystals() {
    for (size_t i = 0; i < crystals.size(); ++i) {
        if (crystals[i].collected) {
            continue;
        }

        if (distance2D(lander.x, lander.y, crystals[i].x, crystals[i].y) < LANDER_RADIUS + 14.0f) {
            crystals[i].collected = true;
            collectedCrystals++;
            score += 120;
            lander.fuel = clampFloat(lander.fuel + 10.0f, 0.0f, 100.0f);
            addExplosion(crystals[i].x, crystals[i].y, 0.18f, 1.0f, 0.88f, 18, 3.2f);
            addFloatingText(crystals[i].x - 18.0f, crystals[i].y + 22.0f, "+120", 1.0f, 0.95f, 0.4f);
            addStatusMessage("Crystal collected. Small fuel refill awarded.");
        }
    }
}

void updateLander() {
    if (keys['a'] || keys['A'] || specialKeys[GLUT_KEY_LEFT]) {
        lander.angle += 3.0f;
    }
    if (keys['d'] || keys['D'] || specialKeys[GLUT_KEY_RIGHT]) {
        lander.angle -= 3.0f;
    }

    if (lander.angle > 180.0f) {
        lander.angle -= 360.0f;
    }
    if (lander.angle < -180.0f) {
        lander.angle += 360.0f;
    }

    bool thrusting = keys['w'] || keys['W'] || specialKeys[GLUT_KEY_UP];
    if (thrusting && lander.fuel > 0.0f) {
        float angle = degToRad(lander.angle);
        float thrust = 0.12f;
        lander.vx += std::sin(angle) * thrust;
        lander.vy += std::cos(angle) * thrust;
        lander.fuel = std::max(0.0f, lander.fuel - 0.22f);
        addThrustParticles();
    }

    lander.vy -= 0.036f;
    lander.vx *= 0.996f;
    lander.vy *= 0.998f;

    lander.x += lander.vx;
    lander.y += lander.vy;

    if (lander.x < -35.0f || lander.x > WIN_W + 35.0f || lander.y > WIN_H + 85.0f || lander.y < -80.0f) {
        handleCrash("The lander drifted outside the rescue zone.");
        return;
    }

    // Landing success depends on pad contact, low speed, and a near-upright angle.
    float terrainY = getTerrainY(lander.x);
    if (lander.y - LANDER_RADIUS <= terrainY) {
        bool touchingPad = (lander.x >= PAD_X1 && lander.x <= PAD_X2);
        bool upright = std::fabs(lander.angle) <= SAFE_LANDING_ANGLE;
        bool slowEnough = std::fabs(lander.vx) <= SAFE_LANDING_VX && std::fabs(lander.vy) <= SAFE_LANDING_VY;

        if (touchingPad && upright && slowEnough) {
            lander.y = terrainY + LANDER_RADIUS + 1.0f;
            lander.vx = 0.0f;
            lander.vy = 0.0f;

            if (collectedCrystals == static_cast<int>(crystals.size())) {
                finishGame(true, "All rescue crystals secured. Excellent landing!");
            } else {
                addStatusMessage("Landing is stable, but more crystals still need rescue.");
                addFloatingText(lander.x - 60.0f, lander.y + 28.0f, "Collect all crystals first", 1.0f, 0.95f, 0.5f);
                lander.vy = 0.7f;
            }
        } else {
            handleCrash("Hard landing. Reduce speed, keep upright, and touch the landing pad.");
        }
    }
}

void updateGame() {
    updateStars();
    updateParticles();
    updateFloatingTexts();

    if (screenShake > 0.0f) {
        screenShake *= 0.86f;
        if (screenShake < 0.15f) {
            screenShake = 0.0f;
        }
    }

    if (statusMessageTimer > 0.0f) {
        statusMessageTimer -= 0.016f;
        if (statusMessageTimer < 0.0f) {
            statusMessageTimer = 0.0f;
        }
    }

    if (state != PLAYING) {
        return;
    }

    missionTime += 0.016f;
    updateLander();
    if (state != PLAYING) {
        return;
    }

    updateMeteors();
    if (state != PLAYING) {
        return;
    }

    updateCrystals();
}

void timer(int) {
    updateGame();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void moveMenuSelection(int direction) {
    menuSelection += direction;
    if (menuSelection < 0) {
        menuSelection = MENU_ITEM_COUNT - 1;
    }
    if (menuSelection >= MENU_ITEM_COUNT) {
        menuSelection = 0;
    }
}

void activateMenuItem() {
    if (menuSelection == 0) {
        startNewGame();
    } else if (menuSelection == 1) {
        state = INSTRUCTIONS;
    } else if (menuSelection == 2) {
        state = CREDITS;
    } else if (menuSelection == 3) {
        std::exit(0);
    }
}

// Menu navigation and gameplay input are handled in one place through the game state.
void keyDown(unsigned char key, int, int) {
    keys[static_cast<unsigned char>(key)] = true;

    if (state == MAIN_MENU) {
        if (key == 'w' || key == 'W') {
            moveMenuSelection(-1);
            return;
        }
        if (key == 's' || key == 'S') {
            moveMenuSelection(1);
            return;
        }
        if (key == 13) {
            activateMenuItem();
            return;
        }
        if (key == 27) {
            std::exit(0);
        }
        return;
    }

    if (state == INSTRUCTIONS || state == CREDITS) {
        if (key == 13 || key == 'm' || key == 'M' || key == 27) {
            state = MAIN_MENU;
        }
        return;
    }

    if (key == 27) {
        if (state == PLAYING || state == PAUSED || state == GAME_OVER || state == GAME_WON) {
            state = MAIN_MENU;
        }
        return;
    }

    if (key == 'm' || key == 'M') {
        state = MAIN_MENU;
        return;
    }

    if (key == 13) {
        if (state == GAME_OVER || state == GAME_WON) {
            startNewGame();
            return;
        }
    }

    if (key == 'r' || key == 'R') {
        restartCurrentMission();
        return;
    }

    if (key == 'p' || key == 'P') {
        if (state == PLAYING) {
            state = PAUSED;
        } else if (state == PAUSED) {
            state = PLAYING;
        }
        return;
    }
}

void keyUp(unsigned char key, int, int) {
    keys[static_cast<unsigned char>(key)] = false;
}

void specialDown(int key, int, int) {
    if (key >= 0 && key < 256) {
        specialKeys[key] = true;
    }

    if (state == MAIN_MENU) {
        if (key == GLUT_KEY_UP) {
            moveMenuSelection(-1);
        } else if (key == GLUT_KEY_DOWN) {
            moveMenuSelection(1);
        }
    }
}

void specialUp(int key, int, int) {
    if (key >= 0 && key < 256) {
        specialKeys[key] = false;
    }
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, static_cast<double>(WIN_W), 0.0, static_cast<double>(WIN_H));
    glMatrixMode(GL_MODELVIEW);
}

void initOpenGL() {
    glClearColor(0.01f, 0.01f, 0.04f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, static_cast<double>(WIN_W), 0.0, static_cast<double>(WIN_H));
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_DEPTH_TEST);
}

int main(int argc, char** argv) {
    srand(static_cast<unsigned int>(time(NULL)));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(120, 80);
    glutCreateWindow("Lunar Rescue Mission - OpenGL CG Project");

    initOpenGL();
    initStars();
    initTerrain();
    resetCrystals();
    resetMeteors();
    resetLanderAtSpawn();
    lander.fuel = 100.0f;
    state = MAIN_MENU;

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
