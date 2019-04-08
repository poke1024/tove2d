/*!
	the following BlueNoise class is adapted for TÖVE from
	https://bitbucket.org/wkjarosz/hdrview/src
	
	original comments:

    force-random-dither.cpp -- Generate a dither matrix using the force-random-dither method from:

	W. Purgathofer, R. F. Tobler and M. Geiler.
	"Forced random dithering: improved threshold matrices for ordered dithering"
	Image Processing, 1994. Proceedings. ICIP-94., IEEE International Conference,
	Austin, TX, 1994, pp. 1032-1035 vol.2.
	doi: 10.1109/ICIP.1994.413512

    \author Wojciech Jarosz

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#ifdef __linux__
// fixes a nasty compilation problem under Ubuntu.
#undef __SSE3__
#endif

#include <random>
#include <algorithm>

class BlueNoise {
	const int Sm;

	struct Vector2i {
		int16_t x;
		int16_t y;

		inline Vector2i(int16_t x, int16_t y) : x(x), y(y) {
		}

		inline bool operator==(const Vector2i &v) {
			return x == v.x && y == v.y;
		}
	};

	struct Matrix {
		float * const data;
		const int size;
		
		Matrix(int size) : size(size), data(new float[size * size]) {
		}

		~Matrix() {
			delete[] data;
		}

		inline float &operator()(int x, int y) {
			return data[x + y * size];
		}
	};

	Matrix *matrix;
	
	inline float toroidalMinimumDistance(const Vector2i & a, const Vector2i & b) {
		int x0 = std::min(a.x, b.x);
		int x1 = std::max(a.x, b.x);
		int y0 = std::min(a.y, b.y);
		int y1 = std::max(a.y, b.y);
		float deltaX = std::min(x1-x0, x0+Sm-x1);
		float deltaY = std::min(y1-y0, y0+Sm-y1);
		return sqrt(deltaX*deltaX + deltaY*deltaY);
	}

	inline float force(float r) {
		return exp(-sqrt(2*r));
	}

public:
	inline const float *get() const {
		return matrix->data;
	}

	BlueNoise(const int Sm = 128) : Sm(Sm) {
        std::random_device rng;
        std::mt19937 urng(rng());

		const int Smk = Sm * Sm;
		matrix = new Matrix(Sm);
		Matrix &M = *matrix;

		std::vector<Vector2i> freeLocations;
		Matrix forceField(Sm);

		// initialize free locations
		for (int y = 0; y < Sm; ++y) {
			for (int x = 0; x < Sm; ++x) {
				M(x, y) = 0;
				forceField(x, y) = 0;
				freeLocations.push_back(Vector2i(x, y));
			}
		}
		
		for (int ditherValue = 0; ditherValue < Smk; ++ditherValue) {
			std::shuffle(freeLocations.begin(), freeLocations.end(), urng);
			
			float minimum = std::numeric_limits<float>::max();
			Vector2i minimumLocation(0, 0);

			// int halfP = freeLocations.size();
			int halfP = std::min(std::max(1, (int)sqrt(freeLocations.size()*3/4)), (int)freeLocations.size());
			// int halfP = min(10, (int)freeLocations.size());
			for (int i = 0; i < halfP; ++i)
			{
				const Vector2i & location = freeLocations[i];
				if (forceField(location.x, location.y) < minimum) {
					minimum = forceField(location.x, location.y);
					minimumLocation = location;
				}
			}

			Vector2i cell(0, 0);
			for (cell.y = 0; cell.y < Sm; ++cell.y) {
				for (cell.x = 0; cell.x < Sm; ++cell.x) {
					float r = toroidalMinimumDistance(cell, minimumLocation);
					forceField(cell.x, cell.y) += force(r);
				}
			}

			freeLocations.erase(remove(freeLocations.begin(), freeLocations.end(), minimumLocation), freeLocations.end());
			M(minimumLocation.x, minimumLocation.y) = ditherValue;
		}

        float mm = 0.0f;
		for (int y = 0; y < Sm; ++y) {
			for (int x = 0; x < Sm; ++x) {
                mm = std::max(mm, M(x, y));
            }
        }
		for (int y = 0; y < Sm; ++y) {
			for (int x = 0; x < Sm; ++x) {
                M(x, y) /= mm;
            }
        }
	}

	~BlueNoise() {
		delete matrix;
	}
};
