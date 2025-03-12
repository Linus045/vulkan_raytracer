#ifndef COMMON_SHADER_FUNCTIONS
#define COMMON_SHADER_FUNCTIONS

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
int getControlPointIndices(int i, int j, int k)
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
	if (k == 0)
	{
		powz = 1;
	}

	return fraction * powi * powj * powk * powz;
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
				int idx1 = getControlPointIndices(i + 1, j, k);
				int idx2 = getControlPointIndices(i, j, k);
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
				int idx1 = getControlPointIndices(i, j + 1, k);
				int idx2 = getControlPointIndices(i, j, k);
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
				int idx1 = getControlPointIndices(i, j, k + 1);
				int idx2 = getControlPointIndices(i, j, k);
				sum += (controlPoints[idx1] - controlPoints[idx2])
				       * BernsteinPolynomialTetrahedral(n - 1, i, j, k, u, v, w);
			}
		}
	}

	return n * sum;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////// Intersection functions ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
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

#endif // COMMON_SHADER_FUNCTIONS
