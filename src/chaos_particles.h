/*
 * Complex particle system.
 * Inspired by fractals,
 * leading to chaos.
 *
 * (c) 2014 Micah Elizabeth Scott
 * http://creativecommons.org/licenses/by/3.0/
 */

#pragma once

#include <vector>
#include "lib/particle.h"
#include "lib/prng.h"
#include "lib/texture.h"


class ChaosParticles : public ParticleEffect
{
public:
    ChaosParticles();
    void reseed(Vec2 location, unsigned seed);

    bool isRunning();
    float getTotalIntensity();

    virtual void beginFrame(const FrameInfo &f);
    virtual void debug(const DebugInfo &di);

private:
    static const unsigned numParticles = 700;
    static const float generationScale = 1.0 / 14;
    static const float speedMin = 0.9;
    static const float speedMax = 1.7;
    static const float spinMin = M_PI / 6;
    static const float spinMax = spinMin + M_PI * 0.05;
    static const float relativeSize = 0.25;
    static const float intensity = 0.5;
    static const float intensityExp = 1.0 / 2.5;
    static const float initialSpeed = 0.005;
    static const float stepSize = 1.0 / 500;
    static const float colorRate = 0.02;
    static const float outsideMargin = 8.0;
    static const unsigned maxAge = 15000;

    struct ParticleDynamics {
        Vec2 position;
        Vec2 velocity;
        bool escaped, dead;
        unsigned generation;
        unsigned age;
    };

    Texture palette;
    std::vector<ParticleDynamics> dynamics;
    float timeDeltaRemainder;
    float colorCycle;
    float totalIntensity;
    bool running;

    void runStep(const FrameInfo &f);
};


/*****************************************************************************************
 *                                   Implementation
 *****************************************************************************************/


inline ChaosParticles::ChaosParticles()
    : palette("data/bang-palette.png"),
      timeDeltaRemainder(0),
      colorCycle(0)
{
    reseed(Vec2(0,0), 42);
}

inline bool ChaosParticles::isRunning()
{
    return running;
}

inline float ChaosParticles::getTotalIntensity()
{
    return totalIntensity;
}

inline void ChaosParticles::reseed(Vec2 location, unsigned seed)
{
    running = true;
    totalIntensity = nanf("");

    appearance.resize(numParticles);
    dynamics.resize(numParticles);

    PRNG prng;
    prng.seed(seed);

    colorCycle = prng.uniform(0, M_PI * 2);

    for (unsigned i = 0; i < dynamics.size(); i++) {
        dynamics[i].position = location;
        dynamics[i].velocity = prng.ringVector(0.01, 1.0) * initialSpeed;
        dynamics[i].age = 0;
        dynamics[i].generation = 0;
        dynamics[i].dead = false;
        dynamics[i].escaped = false;
    }
}

inline void ChaosParticles::beginFrame(const FrameInfo &f)
{    
    if (!running) {
        return;
    }

    float t = f.timeDelta + timeDeltaRemainder;
    int steps = t / stepSize;
    timeDeltaRemainder = t - steps * stepSize;

    while (steps > 0) {
        runStep(f);
        steps--;
    }

    colorCycle = fmodf(colorCycle + f.timeDelta * colorRate, 2 * M_PI);
}

inline void ChaosParticles::debug(const DebugInfo &di)
{
    fprintf(stderr, "\t[chaos-particles] running = %d\n", running);
    fprintf(stderr, "\t[chaos-particles] totalIntensity = %f\n", totalIntensity);
}

inline void ChaosParticles::runStep(const FrameInfo &f)
{
    PRNG prng;
    prng.seed(19);

    unsigned numLiveParticles = 0;
    float intensityAccumulator = 0;

    // Update dynamics
    for (unsigned i = 0; i < dynamics.size(); i++) {

        dynamics[i].position += dynamics[i].velocity;
        dynamics[i].age++;

        if (dynamics[i].age > maxAge) {
            dynamics[i].dead = true;
        }
        if (dynamics[i].dead) {
            appearance[i].intensity = 0;
            continue;
        }
        float ageF = dynamics[i].age / (float)maxAge;

        // XZ plane
        appearance[i].point[0] = dynamics[i].position[0];
        appearance[i].point[2] = dynamics[i].position[1];

        // Fade in/out
        float fade = pow(std::max(0.0f, sinf(ageF * M_PI)), intensityExp);
        float particleIntensity = intensity * fade;
        appearance[i].intensity = particleIntensity;
        appearance[i].radius = f.modelDiameter * relativeSize * fade;

        numLiveParticles++;
        intensityAccumulator += particleIntensity;

        float c = (dynamics[i].generation + ageF) * generationScale;
        appearance[i].color = palette.sample(c, 0.5 + 0.5 * sinf(colorCycle));

        dynamics[i].escaped = f.distanceOutsideBoundingBox(appearance[i].point) >
            outsideMargin * appearance[i].radius;

        prng.remix(dynamics[i].position[0] * 1e8);
        prng.remix(dynamics[i].position[1] * 1e8);
    }

    // Reassign each escaped particle randomly to be near a non-escaped one
    for (unsigned i = 0; i < dynamics.size(); i++) {
        if (dynamics[i].escaped && !dynamics[i].dead) {
            for (unsigned attempt = 0; attempt < 100; attempt++) {
                unsigned seed = prng.uniform(0, dynamics.size() - 0.0001);
                if (!dynamics[seed].escaped) {

                    // Fractal respawn at seed position
                    dynamics[i] = dynamics[seed];
                    dynamics[i].generation++;
                    dynamics[i].age = 0;
                    Vec2 &v = dynamics[i].velocity;

                    // Speed modulation
                    v *= prng.uniform(speedMin, speedMax);

                    // Direction modulation
                    float t = prng.uniform(spinMin, spinMax);
                    float c = cosf(t);
                    float s = sinf(t);
                    v = Vec2( v[0] * c - v[1] * s ,
                              v[0] * s + v[1] * c );

                    break;
                }
            }
        }
    }

    totalIntensity = intensityAccumulator;
    if (!numLiveParticles) {
        running = false;
    }

    ParticleEffect::beginFrame(f);
}
