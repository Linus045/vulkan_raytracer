#ifndef COMMON_SHADER_FUNCTIONS
#define COMMON_SHADER_FUNCTIONS

const float M_PI = 3.1415926535897932384626433832795;

float random(vec2 uv, float seed)
{
	return fract(sin(mod(dot(uv, vec2(12.9898, 78.233)) + 1113.1 * seed, M_PI)) * 43758.5453);
}

float nChooseK(int N, int K)
{
	// This function gets the total number of unique combinations based upon N and K.
	// N is the total number of items.
	// K is the size of the group.
	// Total number of unique combinations = N! / ( K! (N - K)! ).
	// This function is less efficient, but is more likely to not overflow when N and K are large.
	// Taken from:  http://blog.plover.com/math/choose.html
	//
	int r = 1;
	int d;
	if (K > N) return 0;
	for (d = 1; d <= K; d++)
	{
		r *= N--;
		r /= d;
	}
	return r;
}

int iter_factorial(int n)
{
	int ret = 1;
	for (int i = 1; i <= n; ++i)
		ret *= i;
	return ret;
}

float PointInOrOn(vec3 P1, vec3 P2, vec3 A, vec3 B)
{
	vec3 CP1 = cross(B - A, P1 - A);
	vec3 CP2 = cross(B - A, P2 - A);
	return step(0.0, dot(CP1, CP2));
}

float PointInTriangle(vec3 px, vec3 p0, vec3 p1, vec3 p2)
{
	return PointInOrOn(px, p0, p1, p2) * PointInOrOn(px, p1, p2, p0) * PointInOrOn(px, p2, p0, p1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////// Tetrahedron Bezier functions //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
int getControlPointIndicesTetrahedron2(int i, int j, int k)
{
	if (i == 0 && j == 0 && k == 0) return 0;
	if (i == 2 && j == 0 && k == 0) return 1;
	if (i == 0 && j == 2 && k == 0) return 2;
	if (i == 0 && j == 0 && k == 2) return 3;
	if (i == 1 && j == 0 && k == 0) return 4;
	if (i == 0 && j == 1 && k == 0) return 5;
	if (i == 0 && j == 0 && k == 1) return 6;
	if (i == 1 && j == 1 && k == 0) return 7;
	if (i == 1 && j == 0 && k == 1) return 8;
	if (i == 0 && j == 1 && k == 1) return 9;
	return 0;
}

float BernsteinPolynomial(int i, int n, float x)
{
	// pow(0,0) is undefined at runtime! (but 0 when then arguments are known at compile time)
	// http://hacksoflife.blogspot.com/2009/01/pow00-nan-sometimes.html
	float xpowi = pow(x, i);
	if (i == 0)
	{
		xpowi = 1;
	}
	float powOther = pow(1.0 - x, n - i);
	if (abs(n - 1) <= 0.0001)
	{
		powOther = 1;
	}

	float result = nChooseK(n, i) * xpowi * powOther;

	return result;
}

float BernsteinPolynomialTetrahedral(int n, int i, int j, int k, float u, float v, float w)
{
	int d = n;
	int l = d - i - j - k;
	float fraction
	    = iter_factorial(d)
	      / (iter_factorial(i) * iter_factorial(j) * iter_factorial(k) * iter_factorial(l));

	float powi = pow(u, i);
	if (i == 0)
	{
		powi = 1;
	}

	float powj = pow(v, j);
	if (j == 0)
	{
		powj = 1;
	}

	float powk = pow(w, k);
	if (k == 0)
	{
		powk = 1;
	}

	float z = 1.0 - u - v - w;
	float powz = pow(z, l);
	if (l == 0)
	{
		powz = 1;
	}

	return fraction * powi * powj * powk * powz;
}

float BernsteinPolynomialBivariate(int n, int i, int j, int k, float u, float v, float w)
{
	if (i < 0 || j < 0 || k < 0)
	{
		return 0;
	}

	float fraction
	    = iter_factorial(n) / (iter_factorial(i) * iter_factorial(j) * iter_factorial(k));

	float powi = pow(u, i);
	if (i == 0)
	{
		powi = 1;
	}

	float powj = pow(v, j);
	if (j == 0)
	{
		powj = 1;
	}

	float powk = pow(w, k);
	if (k == 0)
	{
		powk = 1;
	}

	return fraction * powi * powj * powk;
}

vec3 partialHu(
    const vec3 controlPoints[10], const int n, const float u, const float v, const float w)
{
	vec3 sum = vec3(0);

	for (int k = 0; k <= n - 1; k++)
	{
		for (int j = 0; j <= n - 1 - k; j++)
		{
			for (int i = 0; i <= n - 1 - k - j; i++)
			{
				int idx1 = getControlPointIndicesTetrahedron2(i + 1, j, k);
				int idx2 = getControlPointIndicesTetrahedron2(i, j, k);
				sum += (controlPoints[idx1] - controlPoints[idx2])
				       * BernsteinPolynomialTetrahedral(n - 1, i, j, k, u, v, w);
			}
		}
	}

	return n * sum;
}

vec3 partialHv(
    const vec3 controlPoints[10], const int n, const float u, const float v, const float w)
{
	vec3 sum = vec3(0);

	for (int k = 0; k <= n - 1; k++)
	{
		for (int j = 0; j <= n - 1 - k; j++)
		{
			for (int i = 0; i <= n - 1 - k - j; i++)
			{
				int idx1 = getControlPointIndicesTetrahedron2(i, j + 1, k);
				int idx2 = getControlPointIndicesTetrahedron2(i, j, k);
				sum += (controlPoints[idx1] - controlPoints[idx2])
				       * BernsteinPolynomialTetrahedral(n - 1, i, j, k, u, v, w);
			}
		}
	}

	return n * sum;
}

vec3 partialHw(
    const vec3 controlPoints[10], const int n, const float u, const float v, const float w)
{
	vec3 sum = vec3(0);

	for (int k = 0; k <= n - 1; k++)
	{
		for (int j = 0; j <= n - 1 - k; j++)
		{
			for (int i = 0; i <= n - 1 - k - j; i++)
			{
				int idx1 = getControlPointIndicesTetrahedron2(i, j, k + 1);
				int idx2 = getControlPointIndicesTetrahedron2(i, j, k);
				sum += (controlPoints[idx1] - controlPoints[idx2])
				       * BernsteinPolynomialTetrahedral(n - 1, i, j, k, u, v, w);
			}
		}
	}

	return n * sum;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////// Bezier Triangle functions /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
int getControlPointIndicesBezierTriangle2(int i, int j, int k)
{
	if (i == 2 && j == 0 && k == 0) return 0;
	if (i == 0 && j == 2 && k == 0) return 1;
	if (i == 0 && j == 0 && k == 2) return 2;
	if (i == 1 && j == 1 && k == 0) return 3;
	if (i == 1 && j == 0 && k == 1) return 4;
	if (i == 0 && j == 1 && k == 1) return 5;
	return 0;
}

vec3 casteljauAlgorithmIntermediatePoint(
    vec3 controlPoints[6], int r, vec3 direction, int i, int j, int k)
{
	vec3 sum = vec3(0);
	int n = 2;
	for (int jk = 0; jk <= n; jk++)
	{
		for (int jj = 0; jj <= n - jk; jj++)
		{
			for (int ji = 0; ji <= n - jk - jj; ji++)
			{
				if (ji + jj + jk == r)
				{
					int idx = getControlPointIndicesBezierTriangle2(i + ji, j + jj, k + jk);
					sum += controlPoints[idx]
					       * BernsteinPolynomialBivariate(
					           r, ji, jj, jk, direction.x, direction.y, direction.z);
				}
			}
		}
	}

	return sum;
}

vec3 partialBezierTriangle2Directional(vec3 controlPoints[6], vec3 direction, float u, float v)
{
	int n = 2;
	vec3 sum = vec3(0);
	float w = 1.0 - u - v;

	// TODO: remove this loop and write out the formula
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				if (i + j + k == n - 1)
				{
					sum += casteljauAlgorithmIntermediatePoint(controlPoints, 1, direction, i, j, k)
					       * BernsteinPolynomialBivariate(n - 1, i, j, k, u, v, w);
				}
			}
		}
	}
	sum *= n;
	return sum;
}

vec3 partialBezierTriangle2U(vec3 controlPoints[6], float u, float v)
{
	int n = 2;
	vec3 sum = vec3(0);

	float w = 1.0 - u - v;

	// check if values are in the right range and if sum is equal to 1
	if (u < 0 || u > 1 || v < 0 || v > 1 || w < 0 || w > 1 || (abs(u + v + w - 1) > 0.00001))
	{
		return sum;
	}

	// TODO: remove this loop and write out the formula
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				if (i + j + k == n - 1)
				{
					int idx = getControlPointIndicesBezierTriangle2(i + 1, j + 0, k + 0);
					sum += controlPoints[idx]
					       * BernsteinPolynomialBivariate(n - 1, i, j, k, u, v, w);
				}
			}
		}
	}
	sum *= n;
	return sum;
}

vec3 partialBezierTriangle2V(vec3 controlPoints[6], float u, float v)
{
	int n = 2;
	vec3 sum = vec3(0);
	float w = 1.0 - u - v;

	// check if values are in the right range and if sum is equal to 1
	if (u < 0 || u > 1 || v < 0 || v > 1 || w < 0 || w > 1 || (abs(u + v + w - 1) > 0.00001))
	{
		return sum;
	}

	// TODO: remove this loop and write out the formula
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				if (i + j + k == n - 1)
				{
					int idx = getControlPointIndicesBezierTriangle2(i + 0, j + 1, k + 0);
					sum += controlPoints[idx]
					       * BernsteinPolynomialBivariate(n - 1, i, j, k, u, v, w);
				}
			}
		}
	}
	sum *= n;
	return sum;
}

vec3 partialBezierTriangle2UVersion2(vec3 controlPoints[6], float u, float v)
{
	int n = 2;
	vec3 sum = vec3(0);
	float w = 1.0 - u - v;

	// TODO: remove this loop and write out the formula
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				if (i + j + k == n)
				{
					sum += n
					       * (BernsteinPolynomialBivariate(n - 1, i - 1, j, k, u, v, w)
					          - BernsteinPolynomialBivariate(n - 1, i, j - 1, k, u, v, w));
				}
			}
		}
	}
	return sum;
}

vec3 partialBezierTriangle2VVersion2(vec3 controlPoints[6], float u, float v)
{
	int n = 2;
	vec3 sum = vec3(0);
	float w = 1.0 - u - v;

	// TODO: remove this loop and write out the formula
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				if (i + j + k == n)
				{
					sum += n
					       * (BernsteinPolynomialBivariate(n - 1, i, j - 1, k, u, v, w)
					          - BernsteinPolynomialBivariate(n - 1, i, j, k - 1, u, v, w));
				}
			}
		}
	}
	return sum;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////// Intersection functions
///////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection.html
bool intersectWithPlane(const vec3 planeNormal,
                        const vec3 planeOrigin,
                        const vec3 rayOrigin,
                        const vec3 rayDirection,
                        out float t)
{
	float denom = dot(planeNormal, rayDirection);
	if (denom > 1e-6)
	{
		vec3 rayToPlanePoint = planeOrigin - rayOrigin;
		t = dot(rayToPlanePoint, planeNormal) / denom;
		return t >= 0;
	}
	return false;
}

vec3 IntersectPlane(vec3 origin, vec3 direction, vec3 p0, vec3 p1, vec3 p2)
{
	vec3 D = direction;
	vec3 N = cross(p1 - p0, p2 - p0);
	vec3 X = origin + D * dot(p0 - origin, N) / dot(D, N);

	return X;
}

bool IntersectTriangle(vec3 origin, vec3 direction, vec3 p0, vec3 p1, vec3 p2, out float t)
{
	vec3 X = IntersectPlane(origin, direction, p0, p1, p2);

	if (PointInTriangle(X, p0, p1, p2) > 0)
	{
		// debugPrintfEXT("IntersectPlane: %f, %f, %f", X.x,X.y,X.z);
		float dist = distance(origin, X);
		// debugPrintfEXT("Dist: %f", dist);
		t = dist;
		return dist > 0;
	}
	return false;
}

// Ray-Sphere intersection
// http://viclw17.github.io/2018/07/16/raytracing-ray-sphere-intersection/
float hitSphere(const Sphere s, const Ray r)
{
	vec3 oc = r.origin - s.center;
	float a = dot(r.direction, r.direction);
	float b = 2.0 * dot(oc, r.direction);
	float c = dot(oc, oc) - s.radius * s.radius;
	float discriminant = b * b - 4 * a * c;
	if (discriminant < 0)
	{
		return -1.0;
	}
	else
	{
		return (-b - sqrt(discriminant)) / (2.0 * a);
	}
}

// Ray-AABB intersection
float hitAabb(const Aabb aabb, const Ray r)
{
	vec3 invDir = 1.0 / r.direction;
	vec3 tbot = invDir * (aabb.minimum - r.origin);
	vec3 ttop = invDir * (aabb.maximum - r.origin);
	vec3 tmin = min(ttop, tbot);
	vec3 tmax = max(ttop, tbot);
	float t0 = max(tmin.x, max(tmin.y, tmin.z));
	float t1 = min(tmax.x, min(tmax.y, tmax.z));
	return t1 > max(t0, 0.0) ? t0 : -1.0;
}

#endif // COMMON_SHADER_FUNCTIONS
