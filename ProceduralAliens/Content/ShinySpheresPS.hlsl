cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

cbuffer LightConstantBuffer : register(b1)
{
	float4 lightPos;
	float4 lightColour;
}

cbuffer CameraConstantBuffer : register (b2)
{
	float4 eyePos;
	float4 lookAt;
	float4 upDir;
	float4 backgroundColour;
	float nearPlane;
	float farPlane;
	float2 padding;
}

struct Ray
{
	float3 o;
	float3 d;
};

struct PixelShaderOutput
{
	float4 colour : SV_TARGET;
	float depth : SV_DEPTH;
};
static PixelShaderOutput output;

//Generate random number
float Hash(float2 grid) {
	float h = dot(grid, float2 (127.1, 311.7));
	return frac(sin(h)*43758.5453123);
}
//Smooth noise
float Noise(in float2 p)
{
	float2 grid = floor(p);
	float2 f = frac(p);
	float2 uv = f * f*(3.0 - 2.0*f);
	float n1, n2, n3, n4;
	n1 = Hash(grid + float2(0.0, 0.0)); n2 = Hash(grid + float2(1.0, 0.0));
	n3 = Hash(grid + float2(0.0, 1.0)); n4 = Hash(grid + float2(1.0, 1.0));
	n1 = lerp(n1, n2, uv.x); n2 = lerp(n3, n4, uv.x);
	n1 = lerp(n1, n2, uv.y);
	return n1;//2*(2.0*n1 -1.0);
}
//Layer noise
float FractalNoise(in float2 xy)
{
	float w = .7;
	float f = 0.0;
	for (int i = 0; i < 4; i++)
	{
		f += Noise(xy) * w;
		w *= 0.5;
		xy *= 2.7;
	}
	return f;
}

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 canvasXY : TEXCOORD0;
};

struct Sphere
{
	float3 centre;
	float rad2;
	float4 colour;
	float Kd, Ks, Kr, shininess;
};
static float4 sphereCol1 = float4(1, 0, 0, 1);
static float4 sphereCol2 = float4(0, 1, 0, 1);
static float4 sphereCol3 = float4(.8, .4, .4, 1);
static float shininess = 40;

#define NOBJECTS 3
static Sphere object[NOBJECTS] = {
	//sphere1
	{0,0,30,		1,		sphereCol1,		0.3, 0.5, 0.7,	shininess},
	//sphere2
	{2,3,30,	0.25,	sphereCol2,		0.5, 0.7, 0.4,	shininess},
	//sphere3
	{0,4,30,		2,		sphereCol3,		0.5, 0.3, 0.3,	shininess}
};

float SphereIntersect(Sphere s, Ray ray, out bool hit)
{
	float t; //find t at point of intersection between ray and sphere
	float3 v = s.centre - ray.o; //the origin of the ray relative to the sphere centre	//viewDir

	//x^2 + y^2 + z^2 = R^2 (sphere) intersection with o+td (ray)
	//x^2 + y^2 + z^2 = dot(x,y,z) so dot(Pos,Pos)
	//due to ray intersection P must equal a point on ray so dot(o+td, o+td)
	//o.o + t2(d.d) + 2td
	float A = dot(v, ray.d); //A = ?
	float B = dot(v, v) - A * A; // B = point of intersection
	//dot(v,v) = |v|^2 - use for attenuation?


	if (B > s.rad2) // if point of intersection is larger than radius - no hit
	{
		hit = false;
		t = farPlane;
	}
	else //hit
	{
		float disc = sqrt(s.rad2 - B);
		t = A - disc;
		if (t < 0.0)
		{
			hit = false;
		}
		else
		{
			hit = true;
		}
	}
	return t;
}

float3 SphereNormal(Sphere s, float3 pos)
{
	return normalize(pos - s.centre);
}

float3 NearestHit(Ray ray, out int hitObj, out bool anyHit)
{
	float minT = farPlane;
	hitObj = -1;
	anyHit = false;
	for (int i = 0; i < NOBJECTS; i++)
	{
		bool hit = false;
		float t = SphereIntersect(object[i], ray, hit);
		if (hit)
		{
			if (t < minT)
			{
				hitObj = i;
				minT = t;
				anyHit = true;
			}
		}
	}
	return ray.o + ray.d * minT;
}

float4 Phong(float3 n, float3 l, float3 v, float shininess, float4 diffuseColour, float4 specularColour)
{
	float NdotL = dot(n, l);
	float diff = saturate(NdotL);
	float3 r = reflect(l, n);
	float spec = pow(saturate(dot(v, r)), shininess) * (NdotL > 0.0);
	return diff * diffuseColour + spec * specularColour;
}

bool Shadow(Ray ray)
{
	bool anyHit = false;
	int hits = 0;
	float minT = farPlane;
	for (int i = 0; i < NOBJECTS; i++)
	{
		float t = SphereIntersect(object[i], ray, anyHit);
		if (anyHit)
		{
			return true;
		}
	}
	return false;
}

float4 Shade(float3 hitPos, float3 normal, float3 viewDir, int hitObj, float lightIntensity)
{
	float3 lightDir = normalize(lightPos.xyz - hitPos);
	float4 diff = object[hitObj].colour * object[hitObj].Kd * FractalNoise(hitPos.xy);
	float4 spec = object[hitObj].colour * object[hitObj].Ks;

	Ray lightRay;
	lightRay.o = hitPos;
	lightRay.d = lightDir;

	return !Shadow(lightRay) * lightColour * lightIntensity * Phong(normal, lightDir, viewDir, object[hitObj].shininess, diff, spec);
}


float4 RayTracing(Ray ray)
{
	int hitObj;
	bool hit = false;
	float3 n;
	float4 c = (float4)0;
	float lightIntensity = 1;

	//calculate the nearest hit
	float3 i = NearestHit(ray, hitObj, hit);

	if (!hit)
	{
		discard;
	}
	float4 depthPos = mul(mul(float4(i, 1), view), projection);
	output.depth = depthPos.z / depthPos.w;

	for (int depth = 1; depth < 5; depth++)
	{
		if (hit)
		{
			n = SphereNormal(object[hitObj], i);
			c += Shade(i, n, ray.d, hitObj, lightIntensity);

			//shoot reflected ray
			lightIntensity *= object[hitObj].Kr;
			ray.o = i;
			ray.d = reflect(ray.d, n);
			i = NearestHit(ray, hitObj, hit);
		}
		else
		{
			c += backgroundColour / depth / depth;
		}
	}
	return c;
}

PixelShaderOutput main(PixelShaderInput input)
{
	float zoom = 5;

	float2 xy = zoom * input.canvasXY;
	float distEye2Canvas = nearPlane;
	float3 PixelPos = float3(xy, -distEye2Canvas);

	float3 viewDir = normalize(eyePos.xyz - lookAt.xyz);
	float3 viewLeft = cross(upDir.xyz, viewDir);
	float3 viewUp = cross(viewDir, viewLeft);
	viewLeft = normalize(viewLeft);
	viewUp = normalize(viewUp);

	//float3 PixelWorld = PixelPos.x*viewLeft + PixelPos.y*viewUp + PixelPos.z*viewDir;

	//PixelPos += eyePos;
	Ray eyeRay;
	eyeRay.o = eyePos.xyz;
	eyeRay.d = normalize(PixelPos - eyePos.xyz);

	output.colour = RayTracing(eyeRay);
	return output;
}