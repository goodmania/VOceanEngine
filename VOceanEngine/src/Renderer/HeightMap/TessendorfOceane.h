#pragma once

#include "Renderer/HeightMap/HeightMap.h"

namespace voe
{
    class TessendorfOceane
    {
    public:

        TessendorfOceane(uint32_t gridSize) 
            : m_MeshSize(gridSize), m_OceanSizeLx(gridSize * 5 / 2), m_OceanSizeLz(gridSize * 5 / 2)
        {
        }

        ~TessendorfOceane() = default;

        // Generates Gaussian random number with mean 0 and standard deviation 1.
        glm::vec2 GaussianRanndomNum()
        {
            std::random_device seed_gen;
            std::default_random_engine engine(seed_gen());

            std::normal_distribution<float> dist(0.0, 1.0);
            float val1 = dist(engine);
            float val2 = dist(engine);

            return glm::vec2(val1, val2);
        }

        // Phillips spectrum
        // (Kx, Ky) - normalized wave vector
        float GeneratePhillipsSpectrum(float Kx, float Ky)
        {
            float k2mag = Kx * Kx + Ky * Ky;

            if (k2mag == 0.0f)
            {
                return 0.0f;
            }
            float k4mag = k2mag * k2mag;

            // largest possible wave from constant wind of velocity v
            float L = windSpeed * windSpeed / G;

            float k_x = Kx / glm::sqrt(k2mag);
            float k_y = Ky / glm::sqrt(k2mag);
            float w_dot_k = k_x * glm::cos(windDir) + k_y * glm::sin(windDir);

            float phillips = A * glm::exp(-1.0f / (k2mag * L * L)) / k4mag * w_dot_k * w_dot_k;

            // damp out waves with very small length w << l
            float l2 = (L / 1000) * (L / 1000);
            phillips *= glm::exp(-k2mag * l2);
            return phillips;
        }

        // Generate base heightfield in frequency space
        void Generate(std::vector<HeightMap::Ocean>& oceanBuffer)
        {
            for (uint32_t y = 0; y < m_MeshSize; y++)
            {
                for (uint32_t x = 0; x < m_MeshSize; x++)
                {
                    float kx = (-(int32_t)m_MeshSize / 2.0f + x) * (2.0f * glm::pi<float>() / m_OceanSizeLx);
                    float ky = (-(int32_t)m_MeshSize / 2.0f + y) * (2.0f * glm::pi<float>() / m_OceanSizeLz);
                    float P = GeneratePhillipsSpectrum(kx, ky);

                    if (kx == 0.0f && ky == 0.0f)
                    {
                        P = 0.0f;
                    }
                    oceanBuffer[y * m_MeshSize + x].h = glm::sqrt(P * 0.5f) * GaussianRanndomNum();
                }
            }
        }

        // Ocean params
        const uint32_t m_MeshSize;
        const uint32_t m_OceanSizeLx;
        const uint32_t m_OceanSizeLz;

    private:
        // gravitational constant
        const float G = 9.81f;  
        // wave scale factor  A - constant
        const float A = 0.000001f;              
        const float windSpeed = 30.0f;
        const float windDir = glm::pi<float>() * 1.234f; 
    };
}

