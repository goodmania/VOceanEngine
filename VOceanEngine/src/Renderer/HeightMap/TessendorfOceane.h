#pragma once

#include "Renderer/HeightMap/HeightMap.h"

namespace voe
{
    class VOE_API TessendorfOceane
    {
    public:

        TessendorfOceane(uint32_t size) 
            : m_MeshSize(size), m_OceanSizeLx(size * 5 / 2), m_OceanSizeLz(size * 5 / 2)
        {
            
        }

        ~TessendorfOceane() = default;

        // Generates Gaussian random number with mean 0 and standard deviation 1.
        glm::vec2 GaussianRanndomNum()
        {
            constexpr float epsilon = std::numeric_limits<float>::epsilon();
            constexpr float two_pi = 2.0 * glm::pi<float>();

            //initialize the random uniform number generator (runif) in a range 0 to 1
            static std::mt19937 rng(std::random_device{}()); // Standard mersenne_twister_engine seeded with rd()
            static std::uniform_real_distribution<> runif(0.0, 1.0);

            //create two random numbers, make sure u1 is greater than epsilon
            double u1, u2;
            do
            {
                u1 = runif(rng);
                u2 = runif(rng);
            } while (u1 <= epsilon);

            //compute z0 and z1
            auto mag = sqrt(-2.0 * log(u1));
            auto z0 = mag * cos(two_pi * u2);
            auto z1 = mag * sin(two_pi * u2);

            return glm::vec2(z0, z1);
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
        void Generate(std::vector<glm::vec2>& h0Buffer)
        {
            for (uint32_t y = 0; y < m_MeshSize; y++)
            {
                for (uint32_t x = 0; x < m_MeshSize; x++)
                {
                    float kx = (-(int)m_MeshSize / 2.0f + x) * (2.0f * glm::pi<float>() / m_OceanSizeLx);
                    float ky = (-(int)m_MeshSize / 2.0f + y) * (2.0f * glm::pi<float>() / m_OceanSizeLz);
                    float P = GeneratePhillipsSpectrum(kx, ky);

                    if (kx == 0.0f && ky == 0.0f)
                    {
                        P = 0.0f;
                    }
                    h0Buffer[y * m_MeshSize + x] = glm::sqrt(P * 0.5f) * GaussianRanndomNum();
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
        const float A = 0.00000161f;
        const float windSpeed = 30.0f;
        const float windDir = glm::pi<float>() / 3.0f;
    };
}

