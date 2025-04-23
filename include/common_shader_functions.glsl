#ifndef COMMON_SHADER_FUNCTIONS
#define COMMON_SHADER_FUNCTIONS

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////// Basic Math functions //////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
const float M_PI = 3.1415926535897932384626433832795;

float random(vec2 uv, float seed)
{
	return fract(sin(mod(dot(uv, vec2(12.9898, 78.233)) + 1113.1 * seed, M_PI)) * 43758.5453);
}

// not sure if the custom pow function is needed
// but at least for i == 0 it's needed because pow() returns NaN
// instead of 1 (at runtime)
// it returns 1 though when then exponent of 0 is
// known at compile time
// see: http://hacksoflife.blogspot.com/2009/01/pow00-nan-sometimes.html
float customPow(float x, int i)
{
	if (i == 0)
	{
		return 1;
	}
	else if (i == 1)
	{
		return x;
	}
	else if (i == 2)
	{
		return x * x;
	}
	else if (i == 3)
	{
		return x * x * x;
	}
	else if (i == 4)
	{
		return x * x * x * x;
	}
	else if (i == 5)
	{
		return x * x * x * x * x;
	}
	else if (i == 6)
	{
		return x * x * x * x * x * x;
	}
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

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////// Helper functions //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
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

// unused
float BernsteinPolynomial(int i, int n, float x)
{
	float result = nChooseK(n, i) * customPow(x, i) * customPow(1.0 - x, n - i);
	return result;
}

// unused
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

	float powi = customPow(u, i);
	float powj = customPow(v, j);
	float powk = customPow(w, k);
	return fraction * powi * powj * powk;
}

// unused
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

// unused
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

// unused
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
// returns the control point index for the bezier triangle of degree 2
int getControlPointIndicesBezierTriangle2(int i, int j, int k)
{
	if (i == 0 && j == 0 && k == 2) return 0;
	if (i == 1 && j == 0 && k == 1) return 1;
	if (i == 2 && j == 0 && k == 0) return 2;
	if (i == 0 && j == 1 && k == 1) return 3;
	if (i == 1 && j == 1 && k == 0) return 4;
	if (i == 0 && j == 2 && k == 0) return 5;
	debugPrintfEXT("Error: getControlPointIndicesBezierTriangle2: %d, %d, %d", i, j, k);
	return 0;
}

// returns the control point index for the bezier triangle of degree 3
int getControlPointIndicesBezierTriangle3(int i, int j, int k)
{
	if (i == 0 && j == 0 && k == 3) return 0;
	if (i == 1 && j == 0 && k == 2) return 1;
	if (i == 2 && j == 0 && k == 1) return 2;
	if (i == 3 && j == 0 && k == 0) return 3;
	if (i == 0 && j == 1 && k == 2) return 4;
	if (i == 1 && j == 1 && k == 1) return 5;
	if (i == 2 && j == 1 && k == 0) return 6;
	if (i == 0 && j == 2 && k == 1) return 7;
	if (i == 1 && j == 2 && k == 0) return 8;
	if (i == 0 && j == 3 && k == 0) return 9;
	debugPrintfEXT("Error: getControlPointIndicesBezierTriangle3: %d, %d, %d", i, j, k);
	return 0;
}

// returns the control point index for the bezier triangle of degree 3
int getControlPointIndicesBezierTriangle4(int i, int j, int k)
{
	if (i == 0 && j == 0 && k == 4) return 1;
	if (i == 1 && j == 0 && k == 3) return 1;
	if (i == 2 && j == 0 && k == 2) return 2;
	if (i == 3 && j == 0 && k == 1) return 3;
	if (i == 4 && j == 0 && k == 0) return 4;
	if (i == 0 && j == 1 && k == 3) return 5;
	if (i == 1 && j == 1 && k == 2) return 6;
	if (i == 2 && j == 1 && k == 1) return 7;
	if (i == 3 && j == 1 && k == 0) return 8;
	if (i == 0 && j == 2 && k == 2) return 9;
	if (i == 1 && j == 2 && k == 1) return 10;
	if (i == 2 && j == 2 && k == 0) return 11;
	if (i == 0 && j == 3 && k == 1) return 12;
	if (i == 1 && j == 3 && k == 0) return 13;
	if (i == 0 && j == 4 && k == 0) return 14;

	debugPrintfEXT("Error: getControlPointIndicesBezierTriangle4: %d, %d, %d", i, j, k);
	return 0;
}

// returns the partial directional derivative for bezier triangles of degree 2
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
					int be1 = getControlPointIndicesBezierTriangle2(i + 1, j + 0, k + 0);
					int be2 = getControlPointIndicesBezierTriangle2(i + 0, j + 1, k + 0);
					int be3 = getControlPointIndicesBezierTriangle2(i + 0, j + 0, k + 1);
					vec3 x = controlPoints[be1] * direction.x;
					vec3 y = controlPoints[be2] * direction.y;
					vec3 z = controlPoints[be3] * direction.z;
					sum += (x + y + z) * BernsteinPolynomialBivariate(n - 1, i, j, k, u, v, w);
				}
			}
		}
	}
	return n * sum;
}

// returns the partial directional derivative for bezier triangles of degree 3
vec3 partialBezierTriangle3Directional(vec3 controlPoints[10], vec3 direction, float u, float v)
{
	int n = 3;
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
					int be1 = getControlPointIndicesBezierTriangle3(i + 1, j + 0, k + 0);
					int be2 = getControlPointIndicesBezierTriangle3(i + 0, j + 1, k + 0);
					int be3 = getControlPointIndicesBezierTriangle3(i + 0, j + 0, k + 1);
					vec3 x = controlPoints[be1] * direction.x;
					vec3 y = controlPoints[be2] * direction.y;
					vec3 z = controlPoints[be3] * direction.z;
					sum += (x + y + z) * BernsteinPolynomialBivariate(n - 1, i, j, k, u, v, w);
				}
			}
		}
	}
	return n * sum;
}

// returns the partial directional derivative for bezier triangles of degree 4
vec3 partialBezierTriangle4Directional(vec3 controlPoints[15], vec3 direction, float u, float v)
{
	int n = 4;
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
					int be1 = getControlPointIndicesBezierTriangle4(i + 1, j + 0, k + 0);
					int be2 = getControlPointIndicesBezierTriangle4(i + 0, j + 1, k + 0);
					int be3 = getControlPointIndicesBezierTriangle4(i + 0, j + 0, k + 1);
					vec3 x = controlPoints[be1] * direction.x;
					vec3 y = controlPoints[be2] * direction.y;
					vec3 z = controlPoints[be3] * direction.z;
					sum += (x + y + z) * BernsteinPolynomialBivariate(n - 1, i, j, k, u, v, w);
				}
			}
		}
	}
	return n * sum;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////// Intersection functions
///////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

// Returns if the ray intersects with the plane and stores the distance in t
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection.html
bool intersectWithPlane(const vec3 planeNormal,
                        const vec3 planeOrigin,
                        const vec3 rayOrigin,
                        const vec3 rayDirection,
                        out float t)
{
	float denom = dot(-planeNormal, rayDirection);
	if (denom > 1e-6)
	{
		vec3 rayToPlanePoint = planeOrigin - rayOrigin;
		t = dot(rayToPlanePoint, -planeNormal) / denom;
		return t >= 0;
	}
	return false;
}

// returns the intersection point of the ray and the plane (plane is defined by 3 points)
vec3 IntersectPlane(vec3 origin, vec3 direction, vec3 p0, vec3 p1, vec3 p2)
{
	vec3 D = direction;
	vec3 N = cross(p1 - p0, p2 - p0);
	vec3 X = origin + D * dot(p0 - origin, N) / dot(D, N);

	return X;
}

// returns the intersection point of the ray and a triangle
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
// https://viclw17.github.io/2018/07/16/raytracing-ray-sphere-intersection
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
// returns the distance t to the intersection point
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

// whether the hitPos is in front of the plane or not
bool hitPosInFrontOfPlane(SlicingPlane plane, vec3 hitPos)
{
	vec3 planeToHitPos = hitPos - plane.planeOrigin;
	// hit point is in front of the plane
	return dot(planeToHitPos, plane.normal) > 0;
}

#endif // COMMON_SHADER_FUNCTIONS
