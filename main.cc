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

  #pragma omp parallel for // OpenMP parallelisatie van de loop over pixels. We hebben een nested loop dus dit geldt alleen voor de buitenste loop.
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
  
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // houdt bij welk proces we zijn
  MPI_Comm_size(MPI_COMM_WORLD, &size); // houdt bij hoeveel processen er zijn

  // Bereken welk frameblok dit proces krijgt
  unsigned int base = FRAMES / size; //hoeveel frames krijgt elk proces
  unsigned int rest = FRAMES % size; //hoeveel frames blijven over indien we de frames niet netjes kunnen verdelen

  unsigned int local_count;
  unsigned int start;

  if ((unsigned int)rank < rest) { // de eerste 'rest' processen krijgen 1 frame extra
    local_count = base + 1; // hoeveel frames krijgt dit proces
    start = rank * local_count; // welk frame is het eerste frame dat dit proces moet renderen 
  } else {
    local_count = base; // hoeveel frames krijgt dit proces (geen extra frames)
    start = rest * (base + 1) + (rank - rest) * base; // welk frame moeten we renderen: Kijkt eerst naar welke frames al zijn verdeeld onder de rest processen, en daarna naar welke frames dit proces moet renderen.
  }
  unsigned int end = start + local_count; // welk frame is het eerste frame dat dit proces niet meer hoeft te renderen (dus: het frame na het laatste frame dat dit proces moet renderen)

  // cout << "Rank " << rank
  //     << " doet frames " << start
  //     << " t/m " << (end - 1)
  //     << " (" << local_count << " frames)" << endl;

  // Needed to send frames over MPI
  MPI_Datatype mpi_img;
  MPI_Type_contiguous(FRAME_SIZE, MPI_BYTE, &mpi_img);
  MPI_Type_commit(&mpi_img);

  animation local_frames(local_count); // de fframes die het proces moet renderen

  MPI_Barrier(MPI_COMM_WORLD); // wacht tot alle processen klaar zijn met bovenstaande setup voor een correcte meting
  double start_time = MPI_Wtime();

  // parallellisatie van het renderen
  for (unsigned int f = start; f < end; f++) {
    // cout << "Rank " << rank << " rendert frame " << f << endl;
    renderFrame(local_frames, f, start);
  }

  animation all_frames;

  // Alleen root heeft ruimte nodig voor alle frames
  if (rank == 0) {
    all_frames.initialise(FRAMES);
  }

  // Root bereidt voor hoeveel frames van elk proces komen en waar ze moeten komen
  int* recvcounts = nullptr; // Aantal frames dat elk proces stuurt naar root
  int* displs = nullptr; // De positie waar alle stukken terecht moeten komen


  // Hieronder eigenlijk hetzelfde idee als bij rank, maar nu voor frames in plaats van processen.
  if (rank == 0) {
    recvcounts = new int[size];
    displs = new int[size];

    for (int p = 0; p < size; p++) {
      unsigned int p_count;
      unsigned int p_start;

      if ((unsigned int)p < rest) {
        p_count = base + 1;
        p_start = p * p_count;
      } else {
        p_count = base;
        p_start = rest * (base + 1) + (p - rest) * base;
      }

      recvcounts[p] = static_cast<int>(p_count);
      displs[p] = static_cast<int>(p_start);
    }
  }

  MPI_Gatherv(
    local_frames.data(),                   // wat dit proces stuurt
    static_cast<int>(local_count),         // hoeveel frames dit proces stuurt
    mpi_img,                               // datatype van 1 frame
    all_frames.data(),                     // waar root alles ontvangt
    recvcounts,                            // hoeveel per proces
    displs,                                // waar elk blok moet komen
    mpi_img,                               // datatype van 1 frame
    0,                                     // root = rank 0
    MPI_COMM_WORLD
  );

  // Alleen root heeft na het gatheren alle frames,
  // dus alleen root kan de volledige video opslaan.
  if (rank == 0) {
    cimg_library::CImg<byte> img(WIDTH, HEIGHT, FRAMES, 3);

    cimg_forXYZ(img, x, y, z) {
      img(x,y,z,RED) = all_frames[z].get_channel(x,y,RED);
      img(x,y,z,GREEN) = all_frames[z].get_channel(x,y,GREEN);
      img(x,y,z,BLUE) = all_frames[z].get_channel(x,y,BLUE);
    }

    std::string filename = std::string("animation.avi");
    img.save_video(filename.c_str());

    delete[] recvcounts;
    delete[] displs;
  }

  double end_time = MPI_Wtime();

  if (rank == 0) {
  cout << "Totale tijd: " << (end_time - start_time) << " seconden" << endl;
  }

  // Also needed to send frames over MPI
  MPI_Type_free(&mpi_img);
  MPI_Finalize();
}
