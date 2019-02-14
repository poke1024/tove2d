/*
 * TÖVE - Animated vector graphics for LÖVE.
 * https://github.com/poke1024/tove2d
 *
 * Copyright (c) 2018, Bernhard Liebl
 *
 * Distributed under the MIT license. See LICENSE file for details.
 *
 * All rights reserved.
 */

#ifndef __TOVE_INTERSECT
#define __TOVE_INTERSECT 1

BEGIN_TOVE_NAMESPACE

class AbstractRay {
public:
	virtual void computeP(double *P, const coeff *bx, const coeff *by) const = 0;
};

class AbstractIntersecter {
protected:
	virtual int at(const coeff *bx, const coeff *by, double t) = 0;

public:
	int intersect(const coeff *bx, const coeff *by, const AbstractRay &ray) {
		double P[4];
		ray.computeP(P, bx, by);

		double a = P[0];
		double b = P[1];
		double c = P[2];
		double d = P[3];

		if (abs(a) < 1e-6) {
			// not cubic.
			if (abs(b) < 1e-6) {
				// linear.
				return at(bx, by, -d / c);
			} else {
				// 	quadratic.
				double D = sqrt(c * c - 4 * b * d);
				double n = 2 * b;
				int z = at(bx, by, (-c + D) / n);
				if (D > 0.0) {
					z += at(bx, by, (-c - D) / n);
				}
				return z;
			}
		} else {
			double A = b / a;
			double B = c / a;
			double C = d / a;

			double Q = (3 * B - pow(A, 2)) / 9;
			double R = (9 * A * B - 27 * C - 2 * pow(A, 3)) / 54;
			double D = pow(Q, 3) + pow(R, 2);
			double A_third = A / 3.0;

			if (D >= 0.0) { // complex or duplicate roots
				double sqrtD = sqrt(D);

				double r0 = R + sqrtD;
				double r1 = R - sqrtD;

				double S = copysign(pow(abs(r0), 1.0 / 3.0), r0);
				double T = copysign(pow(abs(r1), 1.0 / 3.0), r1);
				double st = S + T;

				int z = at(bx, by, -A_third + st);
				if (std::abs(S - T) < 1e-6) {
					z += at(bx, by, -A_third - st / 2) * 2;
				}
				return z;
			} else { // distinct real roots
				double th = acos(R / sqrt(-pow(Q, 3)));
				double u = 2 * sqrt(-Q);

				return at(bx, by, u * cos(th / 3) - A_third) +
					at(bx, by, u * cos((th + 2 * M_PI) / 3) - A_third) +
					at(bx, by, u * cos((th + 4 * M_PI) / 3) - A_third);
			}
		}
	}
};

class Intersecter : public AbstractIntersecter {
private:
    std::vector<float> hits;

protected:
    virtual int at(const coeff *bx, const coeff *by, double t) {
        if (t >= 0.0 && t <= 1.0) {
            double t2 = t * t;
			double t3 = t2 * t;

            hits.push_back(dot4(bx, t3, t2, t, 1));
            hits.push_back(dot4(by, t3, t2, t, 1));
        }

        return 0;
    }

    const std::vector<float> &get() const {
        return hits;
    };
};

class RuntimeRay : public AbstractRay {
private:
	const float A, B, C;

public:
	inline RuntimeRay(float x1, float y1, float x2, float y2) :
		A(y2 - y1),
		B(x1 - x2),
		C(x1 * (y1 - y2) + y1 * (x2 - x1)) {
	}

	virtual void computeP(double *P, const coeff *bx, const coeff *by) const {
		P[0] = A * bx[0] + B * by[0];     // t^3
		P[1] = A * bx[1] + B * by[1];     // t^2
		P[2] = A * bx[2] + B * by[2];     // t
		P[3] = A * bx[3] + B * by[3] + C; // 1
	}
};

template<int DX, int DY>
class CompiledRay : public AbstractRay {
private:
	const float x1, y1;

public:
	inline CompiledRay(float x1, float y1) : x1(x1), y1(y1) {
	}

	virtual void computeP(double *P, const coeff *bx, const coeff *by) const {
		constexpr int A = DY;
		constexpr int B = -DX;
		const float C = x1 * (-DY) + y1 * DX;
		P[0] = A * bx[0] + B * by[0];     // t^3
		P[1] = A * bx[1] + B * by[1];     // t^2
		P[2] = A * bx[2] + B * by[2];     // t
		P[3] = A * bx[3] + B * by[3] + C; // 1
	}
};

template<int DX, int DY>
class NonZeroCounter : public AbstractIntersecter {
private:
	const double x;
	const double y;

	virtual int at(const coeff *bx, const coeff *by, double t) {
		if (t >= 0.0 && t <= 1.0) {
			double t2 = t * t;
			double t3 = t2 * t;

			if (DX * (x - dot4(bx, t3, t2, t, 1)) >= 0 &&
				DY * (y - dot4(by, t3, t2, t, 1)) >= 0) {
				return sgn(dot3(by, 3 * t2, 2 * t, 1));
			}
		}

		return 0;
	}

	const CompiledRay<DX, DY> ray;

public:
	inline NonZeroCounter(double x, double y) : x(x), y(y), ray(x, y) {
	}

	inline int operator()(const coeff *bx, const coeff *by) {
		return intersect(bx, by, ray);
	}
};

template<int DX, int DY>
class EvenOddCounter : public AbstractIntersecter {
private:
	const double x;
	const double y;

	virtual int at(const coeff *bx, const coeff *by, double t) {
		if (t >= 0.0 && t <= 1.0) {
			double t2 = t * t;
			double t3 = t2 * t;

			if (DX * (x - dot4(bx, t3, t2, t, 1)) >= 0 &&
				DY * (y - dot4(by, t3, t2, t, 1)) >= 0) {
				return 1;
			}
		}

		return 0;
	}

	const CompiledRay<DX, DY> ray;

public:
	inline EvenOddCounter(double x, double y) : x(x), y(y), ray(x, y) {
	}

	inline int operator()(const coeff *bx, const coeff *by) {
		return intersect(bx, by, ray);
	}
};

class AbstractInsideTest {
protected:
	int counts[3];

	template<template <int, int> class Counter>
    void _add(const coeff *bx, const coeff *by, float x, float y) {
		Counter<1, 0> c1(x, y);
		counts[0] += c1(bx, by);

		Counter<0, 1> c2(x, y);
		counts[1] += c2(bx, by);

		Counter<1, 1> c3(x, y);
		counts[2] += c3(bx, by);
	}

public:
    inline AbstractInsideTest() {
        for (int i = 0; i < 3; i++) {
            counts[i] = 0;
        }
    }

	virtual void add(const coeff *bx, const coeff *by, float x, float y) = 0;
};

class NonZeroInsideTest : public AbstractInsideTest {
public:
	virtual void add(const coeff *bx, const coeff *by, float x, float y) {
		_add<NonZeroCounter>(bx, by, x, y);
	}

	virtual bool get() const {
		int votes = 0;
		for (int i = 0; i < 3; i++) {
			votes += counts[i] != 0 ? 1 : 0;
		}
		return votes >= 2;
	}
};

class EvenOddInsideTest : public AbstractInsideTest {
public:
	virtual void add(const coeff *bx, const coeff *by, float x, float y) {
		_add<EvenOddCounter>(bx, by, x, y);
	}

	virtual bool get() const {
		int votes = 0;
		for (int i = 0; i < 3; i++) {
			votes += (counts[i] & 1) != 0 ? 1 : 0;
		}
		return votes >= 2;
	}
};

END_TOVE_NAMESPACE

#endif // __TOVE_INTERSECT
