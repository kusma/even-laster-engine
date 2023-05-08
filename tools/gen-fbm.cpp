#include "FastNoise.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
   if (argc != 2) {
      fprintf(stderr, "USAGE: gen-fbm <filename>\n");
      return 1;
   }

   const char *filename = argv[1];

   int width = 64, height = 64, depth = 64;
   auto size = 4 * width * height * depth;
   float *buf = new float[size];
   FastNoise noiseX(1337), noiseY(1338), noiseZ(1339);

   auto type = FastNoise::NoiseType::PerlinFractal;
   noiseX.SetNoiseType(type);
   noiseY.SetNoiseType(type);
   noiseZ.SetNoiseType(type);

   auto octaves = 10;
   noiseX.SetFractalOctaves(octaves);
   noiseY.SetFractalOctaves(octaves);
   noiseZ.SetFractalOctaves(octaves);

   noiseX.SetFrequency(4.0f / width);
   noiseY.SetFrequency(4.0f / height);
   noiseZ.SetFrequency(4.0f / depth);

   FastNoise noises[] = { noiseX, noiseY, noiseZ };

   for (auto z = 0; z < depth; ++z) {
      float zw = float(z) / depth;
      for (auto y = 0; y < height; ++y) {
         float yw = float(y) / height;
         auto row = buf + 4 * (z * width * height + y * width);
         for (auto x = 0; x < width; ++x) {
            float xw = float(x) / width;

            for (int i = 0; i < 3; ++i) {
               auto a = noises[i].GetNoise(x,         y,          z);
               auto b = noises[i].GetNoise(x + width, y,          z);
               auto c = noises[i].GetNoise(x,         y + height, z);
               auto d = noises[i].GetNoise(x + width, y + height, z);

               auto e = noises[i].GetNoise(x,         y,          z + depth);
               auto f = noises[i].GetNoise(x + width, y,          z + depth);
               auto g = noises[i].GetNoise(x,         y + height, z + depth);
               auto h = noises[i].GetNoise(x + width, y + height, z + depth);

               float h1 = a * xw + b * (1 - xw);
               float h2 = c * xw + d * (1 - xw);
               float h3 = e * xw + f * (1 - xw);
               float h4 = g * xw + h * (1 - xw);

               float v1 = h1 * yw + h2 * (1 - yw);
               float v2 = h3 * yw + h4 * (1 - yw);

               row[x * 4 + i] = v1 * zw + v2 * (1 - zw);
            }
            row[x * 4 + 3] = 0.0f;
         }
      }
   }

   FILE *fp = fopen(filename, "wb");
   fwrite(buf, 1, sizeof(float) * size, fp);
   delete[] buf;
   fclose(fp);
   return 0;
}
