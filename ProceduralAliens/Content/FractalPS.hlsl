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
	float4 backgroundColour;
	float4 lookAt;
	float4 upDir;
	float nearPlane;
	float farPlane;
	float2 padding;
}

cbuffer TimeConstantBuffer : register(b3)
{
	float time;
	float3 padding2;
}

struct Ray
{
	float3 o;
	float3 d;
};

// Per-pixel color data passed through the pixel shader.
struct VS_QUAD
{
	float4 pos : SV_POSITION;
	float2 canvasXY : TEXCOORD0;
};

struct PixelShaderOutput
{
	float4 colour : SV_TARGET;
	float depth : SV_DEPTH;
};
static PixelShaderOutput output;

float2 opU(float2 d1, float2 d2)
{
	return (d1.x < d2.x) ? d1 : d2;
}

float DE1(float3 z, float scale, float iterations) {
	//tetra-vertices 
	float3 v1 = float3(0, 1.5, 0);
	float3 v2 = float3(1.0, 0.0, 0.0);
	float3 v3 = float3(cos(2 * 3.1415 / 3.0), 0.0, sin(2 * 3.1415 / 3.0));
	float3 v4 = float3(cos(4 * 3.1415 / 3.0), 0.0, sin(4 * 3.1415 / 3.0));
	float3 c;  //centre 
	int n = 0;
	float dist, d;
	while (n < iterations) {
		c = v1;
		dist = length(z - v1);
		d = length(z - v2);
		if (d < dist)
		{
			c = v2;
			dist = d;
		}
		d = length(z - v3);
		if (d < dist)
		{
			c = v3;
			dist = d;
		}
		d = length(z - v4);
		if (d < dist)
		{
			c = v4;
			dist = d;
		}
		z = scale * (z - c) + c;
		n++;
	}
	return  length(z)*pow(scale, float(-n));;
}

float Mandelbulb(float3 p)
{
	float3 w = p;
	float m = dot(w, w);

	float4 trap = float4(abs(w), m);
	float dz = 1.0;


	for (int i = 0; i < 25; i++)
	{
		float m2 = m * m;
		float m4 = m2 * m2;
		dz = 8*sqrt(m4*m2*m)*dz + 1.0;

		float x = w.x; float x2 = x * x; float x4 = x2 * x2;
		float y = w.y; float y2 = y * y; float y4 = y2 * y2;
		float z = w.z; float z2 = z * z; float z4 = z2 * z2;

		float k3 = x2 + z2;
		float k2 = rsqrt(k3*k3*k3*k3*k3*k3*k3);
		float k1 = x4 + y4 + z4 - 6.0*y2*z2 - 6.0*x2*y2 + 2.0*z2*x2;
		float k4 = x2 - y2 + z2;

		w.x = p.x + 64.0*x*y*z*(x2 - z2)*k4*(x4 - 6.0*x2*z2 + z4)*k1*k2;
		w.y = p.y + -16.0*y2*k3*k4*k4 + k1 * k1;
		w.z = p.z + -8.0*y*k4*(x4*x4 - 28.0*x4*x2*z2 + 70.0*x4*z4 - 28.0*x2*z2*z4 + z4 * z4)*k1*k2;      

		trap = min(trap, float4(abs(w), m));

		m = dot(w, w);
		if (m > 256.0)
			break;
	}

	return 0.25*log(m)*sqrt(m) / dz;
}
//------------------------------------------------------------------

#define ZERO (min(iFrame,0))

//------------------------------------------------------------------

float2 map(in float3 inpos)
{
	float3 pos = inpos;

	float2 res = float2(1e10, 0.0);

	res = opU(res, float2(DE1(pos - float3(-2,-1,-7), 2, 15), 5));

	res = opU(res, float2(Mandelbulb(pos - float3(2,-1, -7)), 40));

	return res;
}

// http://iquilezles.org/www/articles/boxfunctions/boxfunctions.htm
float2 iBox(in float3 ro, in float3 rd, in float3 rad)
{
	float3 m = 1.0 / rd;
	float3 n = m * ro;
	float3 k = abs(m)*rad;
	float3 t1 = -n - k;
	float3 t2 = -n + k;
	return float2(max(max(t1.x, t1.y), t1.z),
		min(min(t2.x, t2.y), t2.z));
}

const float maxHei = 0.8;

float2 castRay(in float3 ro, in float3 rd)
{
	float2 res = float2(-1.0, -1.0);

	float tmin = 1.0;
	float tmax = 200.0;

	// raytrace floor plane
	//float tp1 = (0.0 - ro.y) / rd.y;
	//if (tp1 > 0.0)
	//{
	//	tmax = min(tmax, tp1);
	//	res = float2(tp1, 1.0);
	//}
	//else return res;

	// raymarch primitives   
	float2 tb = iBox(ro, rd, float3(100, 100, 100));
	if (tb.x<tb.y && tb.y>0.0 && tb.x < tmax)
	{
		tmin = max(tb.x, tmin);
		tmax = min(tb.y, tmax);

		float t = tmin;
		for (int i = 0; i < 170 && t < tmax; i++)
		{

			float2 h = map(ro + rd * t);
			if (abs(h.x) < (0.001*t))
			{
				res = float2(t, h.y);
				break;
			}
			t += h.x;
		}

	}

	return res;
}


// http://iquilezles.org/www/articles/rmshadows/rmshadows.htm
float calcSoftshadow(in float3 ro, in float3 rd, in float mint, in float tmax)
{
	// bounding volume
	float tp = (maxHei - ro.y) / rd.y; if (tp > 0.0) tmax = min(tmax, tp);

	float res = 1.0;
	float t = mint;
	for (int i = 0; i < 16; i++)
	{
		float h = map(ro + rd * t).x;
		res = min(res, 8.0*h / t);
		t += clamp(h, 0.02, 0.10);
		//if (res<0.005 || t>tmax) break;
	}
	return clamp(res, 0.0, 1.0);
}

// http://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
float3 calcNormal(in float3 pos)
{
#if 1
	float2 e = float2(1.0, -1.0)*0.5773*0.0005;
	return normalize(e.xyy*map(pos + e.xyy).x +
		e.yyx*map(pos + e.yyx).x +
		e.yxy*map(pos + e.yxy).x +
		e.xxx*map(pos + e.xxx).x);
#else
	// inspired by klems - a way to prevent the compiler from inlining map() 4 times
	float3 n = float3(0.0);
	for (int i = ZERO; i < 4; i++)
	{
		float3 e = 0.5773*(2.0*float3((((i + 3) >> 1) & 1), ((i >> 1) & 1), (i & 1)) - 1.0);
		n += e * map(pos + 0.0005*e).x;
	}
	return normalize(n);
#endif    
}

float calcAO(in float3 pos, in float3 nor)
{
	float occ = 0.0;
	float sca = 1.0;
	for (int i = 0; i < 5; i++)
	{
		float hr = 0.01 + 0.12*float(i) / 4.0;
		float3 aopos = nor * hr + pos;
		float dd = map(aopos).x;
		occ += -(dd - hr)*sca;
		sca *= 0.95;
	}
	return clamp(1.0 - 3.0*occ, 0.0, 1.0) * (0.5 + 0.5*nor.y);
}

// http://iquilezles.org/www/articles/checkerfiltering/checkerfiltering.htm
float checkersGradBox(in float2 p)
{
	// filter kernel
	float2 w = fwidth(p) + 0.001;
	// analytical integral (box filter)
	float2 i = 2.0*(abs(frac((p - 0.5*w)*0.5) - 0.5) - abs(frac((p + 0.5*w)*0.5) - 0.5)) / w;
	// xor pattern
	return 0.5 - 0.5*i.x*i.y;
}

float3 render(in float3 ro, in float3 rd)
{
	float3 col = float3(0.7, 0.9, 1.0) + rd.y*0.8;
	float2 res = castRay(ro, rd);
	float t = res.x;
	float m = res.y;
	if (m > -0.5)
	{
		float3 pos = ro + t * rd;
		float4 depthPos = mul(mul(float4(pos, 1), view), projection);
		output.depth = depthPos.z / depthPos.w;
		float3 nor = (m < 1.5) ? float3(0.0, 1.0, 0.0) : calcNormal(pos);
		float3 ref = reflect(rd, nor);

		// material        
		col = 0.45 + 0.35*sin(float3(0.05, 0.08, 0.10)*(m - 1.0));
		if (m < 1.5)
		{

			float f = checkersGradBox(5.0*pos.xz);
			col = 0.3 + f * float3(0.1, 0.1, 0.1);
		}

		// lighting
		float occ = calcAO(pos, nor);
		float3  lig = normalize(float3(-0.4, 0.7, -0.6));
		float3  hal = normalize(lig - rd);
		float amb = clamp(0.5 + 0.5*nor.y, 0.0, 1.0);
		float dif = clamp(dot(nor, lig), 0.0, 1.0);
		float bac = clamp(dot(nor, normalize(float3(-lig.x, 0.0, -lig.z))), 0.0, 1.0)*clamp(1.0 - pos.y, 0.0, 1.0);
		float dom = smoothstep(-0.2, 0.2, ref.y);
		float fre = pow(clamp(1.0 + dot(nor, rd), 0.0, 1.0), 2.0);

		dif *= calcSoftshadow(pos, lig, 0.02, 2.5);
		dom *= calcSoftshadow(pos, ref, 0.02, 2.5);

		float spe = pow(clamp(dot(nor, hal), 0.0, 1.0), 16.0)*
			dif *
			(0.04 + 0.96*pow(clamp(1.0 + dot(hal, rd), 0.0, 1.0), 5.0));

		float3 lin = float3(0, 0, 0);
		lin += 1.30*dif*float3(1.00, 0.80, 0.55);
		lin += 0.30*amb*float3(0.40, 0.60, 1.00)*occ;
		lin += 0.40*dom*float3(0.40, 0.60, 1.00)*occ;
		lin += 0.50*bac*float3(0.25, 0.25, 0.25)*occ;
		lin += 0.25*fre*float3(1.00, 1.00, 1.00)*occ;
		col = col * lin;
		col += 9.00*spe*float3(1.00, 0.90, 0.70);

		col = lerp(col, float3(0.8, 0.9, 1.0), 1.0 - exp(-0.0002*t*t));
	}
	else
	{
		discard;
	}

	return float3(clamp(col, 0.0, 1.0));
}

float3x3 setCamera(in float3 ro, in float3 ta, float cr)
{
	float3 cw = normalize(ta - ro);
	float3 cp = float3(sin(cr), cos(cr), 0.0);
	float3 cu = normalize(cross(cw, cp));
	float3 cv = (cross(cu, cw));
	return transpose(float3x3(cu, cv, cw));
}

PixelShaderOutput main(VS_QUAD input)
{
	float zoom = 10;

	float2 xy = zoom * input.canvasXY;
	float distEye2Canvas = nearPlane;
	float3 PixelPos = float3(xy, -distEye2Canvas);

	Ray eyeRay;
	eyeRay.o = eyePos.xyz;
	eyeRay.d = normalize(PixelPos - eyePos.xyz);

	output.colour = float4(render(eyeRay.o, eyeRay.d), 1);
	return output;
}
