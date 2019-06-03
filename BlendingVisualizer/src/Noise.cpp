#include "detail/Noise.h"

#define BM 0xff
#define N 0x1000

Noise::Noise(const Ogre::Real& alpha, const Ogre::Real& beta, int iterationNum, const Ogre::Real& cycle, const Ogre::Real& heightScale)
	: mAlpha(alpha)
	, mBeta(beta)
	, mIterationNum(iterationNum)
	, mCycle(cycle)
	, mHeightScale(heightScale)
	, mOriginPoint(0.f)
{
	for (int i = 0; i < B; i++) {
		p[i] = i;
		g1[i] = Ogre::Math::SymmetricRandom();

		g2[i] = Ogre::Vector2(Ogre::Math::SymmetricRandom(), Ogre::Math::SymmetricRandom());
		g2[i].normalise();

		g3[i] = Ogre::Vector3(Ogre::Math::SymmetricRandom(), Ogre::Math::SymmetricRandom(), Ogre::Math::SymmetricRandom());
		g3[i].normalise();
	}

	for (int i = 0; i < B; i++) {
		int j = (int)Ogre::Math::RangeRandom(0, B);

		int k = p[i];
		p[i] = p[j];
		p[j] = k;
	}

	for (int i = 0; i < B + 2; i++) {
		p[B + i] = p[i];
		g1[B + i] = g1[i];
		g2[B + i] = g2[i];
		g3[B + i] = g3[i];
	}
}

Ogre::Real Noise::get(const Ogre::Vector2& vec2)
{
	Ogre::Vector2 tempVec(vec2);
	Ogre::Real sum = 0;
	Ogre::Real scale = 1;

	for (int i = 0; i < mIterationNum; i++) {
		sum += noise(tempVec) / scale;
		scale *= mAlpha;
		tempVec *= mBeta;
	}
	return sum;
}

Ogre::Real Noise::noise(const Ogre::Vector2& vec2)
{
	int bx0, bx1, by0, by1, b00, b10, b01, b11;
	Ogre::Real rx0, rx1, ry0, ry1, sx, sy, a, b, u, v;

	Noise::setup(vec2.x, bx0, bx1, rx0, rx1);
	Noise::setup(vec2.y, by0, by1, ry0, ry1);

	int i = p[bx0];
	int j = p[bx1];

	b00 = p[i + by0];
	b10 = p[j + by0];
	b01 = p[i + by1];
	b11 = p[j + by1];

	sx = sCurve(rx0);
	sy = sCurve(ry0);

	u = g2[b00].dotProduct(Ogre::Vector2(rx0, ry0));
	v = g2[b10].dotProduct(Ogre::Vector2(rx1, ry0));
	a = lerp(sx, u, v);

	u = g2[b01].dotProduct(Ogre::Vector2(rx0, ry1));
	v = g2[b11].dotProduct(Ogre::Vector2(rx1, ry1));
	b = lerp(sx, u, v);

	return lerp(sy, a, b);
}

void Noise::setup(const Ogre::Real& target, int& b0, int& b1, Ogre::Real& r0, Ogre::Real& r1)
{
	Ogre::Real t = target + N;
	b0 = ((int)t) & BM;
	b1 = (b0 + 1) & BM;
	r0 = t - (int)t;
	r1 = r0 - 1;
}
