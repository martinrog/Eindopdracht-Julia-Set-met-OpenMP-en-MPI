#include <cmath>
#include <complex>
#include <numbers>
#include <ostream>
#include <omp.h>
#include <iostream>
#include <mpi.h>

#include "consts.h"
#include "frame.h"
#include "animation.h"

#define cimg_display 0        // No window plz
#include "CImg.h"

using std::cout, std::endl;
using namespace std::literals::complex_literals;

// Colour based on ratio between number of iterations and MAX_ITER
inline constexpr pixel COLOURISE(double iter) { 
  iter = fmod(4 - iter * 5 / MAX_ITER, 6);
  byte x = static_cast<byte>(255 * (1 - std::abs(fmod(iter, 2) - 1)));
  byte r, g, b;

  if      (             iter < 1) { r = 255; g =   x; b =   0; }
  else if (iter >= 1 && iter < 2) { r =   x; g = 255; b =   0; }
  else if (iter >= 2 && iter < 3) { r =   0; g = 255; b =   x; }
  else if (iter >= 3 && iter < 4) { r =   0; g =   x; b = 255; }
  else if (iter >= 4 && iter < 5) { r =   x; g =   0; b = 255; }
  else                            { r = 255; g =   0; b =   x; }
  return { r, g, b };
}

void renderFrame(animation &frames, unsigned int t, unsigned int offset) {

  // Code voor het getal C dat we gebruiken in de formule z = z² + c.
  double a = 2 * std::numbers::pi * t / CYCLE_FRAMES;
  double r = 0.7885;
  std::complex<double> c = r * cos(a) + 1i * r * sin(a);

  // Loop over alle pixels (x, y) in het frame
  for (unsigned int y = 0; y < HEIGHT; y++) {
    for (unsigned int x = 0; x < WIDTH; x++) {

      // Code voor het bepalen van het startpunt z
      double x_y_range = 2;
      double scale = 1.5 - 1.45 * log(1 + 9.0 * t / FRAMES) / log(10);    // iets interessanter om naar te kijken

      std::complex<double> z = 2 * x_y_range * std::complex(static_cast<double>(x)/WIDTH, static_cast<double>(y)/HEIGHT)
          - std::complex(x_y_range*3/4, x_y_range);

      z *= scale;


      // Herhaal z = z² + c totdat z "ontsnapt" of we MAX_ITER halen.
      int iter = 0; // telt hoe vaak we de update uitvoeren.

      while (std::norm(z) < x_y_range && iter < MAX_ITER) {
        z = z * z + c;
        iter++;
      }

      // Als we MAX_ITER halen, kleuren we zwart.
      // Anders geven we een kleur op basis van iter
      if (iter == MAX_ITER) {
        frames[t - offset].set_colour(x, y, {0, 0, 0});
      } else {
        frames[t - offset].set_colour(x, y, COLOURISE(iter));
      }
    }
  }
}

int main (int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  // Needed to send frames over MPI
  MPI_Datatype mpi_img;
  MPI_Type_contiguous(FRAME_SIZE, MPI_BYTE, &mpi_img);
  MPI_Type_commit(&mpi_img);

  animation frames(FRAMES);

  // TODO - parallellisation
  for (unsigned int f = 0; f < FRAMES; f++) {
    cout << endl << "Rendering frame " << f << endl;
    renderFrame(frames, f, 0);
  }

  cimg_library::CImg<byte> img(WIDTH,HEIGHT,FRAMES,3);
  cimg_forXYZ(img, x, y, z) { 
    img(x,y,z,RED) = (frames)[z].get_channel(x,y,RED);
    img(x,y,z,GREEN) = (frames)[z].get_channel(x,y,GREEN);
    img(x,y,z,BLUE) = (frames)[z].get_channel(x,y,BLUE);
  }

  std::string filename = std::string("animation.avi");
  img.save_video(filename.c_str());

  // Also needed to send frames over MPI
  MPI_Type_free(&mpi_img);
  MPI_Finalize();
}
