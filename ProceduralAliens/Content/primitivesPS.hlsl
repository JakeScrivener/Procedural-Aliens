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

float Hash(float2 grid) {
	float h = dot(grid, float2 (127.1, 311.7));
	return frac(sin(h)*43758.5453123);
}

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

float sdPlane(float3 p)
{
	return p.y;
}

float sdSphere(float3 p, float s)
{
	return length(p) - s;
}

float sdBox(float3 p, float3 b)
{
	float3 d = abs(p) - b;
	return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

float sdEllipsoid(in float3 p, in float3 r) // approximated
{
	float k0 = length(p / r);
	float k1 = length(p / (r*r));
	return k0 * (k0 - 1.0) / k1;

}

float sdRoundBox(in float3 p, in float3 b, in float r)
{
	float3 q = abs(p) - b;
	return min(max(q.x, max(q.y, q.z)), 0.0) + length(max(q, 0.0)) - r;
}

float sdTorus(float3 p, float2 t)
{
	return length(float2(length(p.xz) - t.x, p.y)) - t.y;
}

float sdHexPrism(float3 p, float2 h)
{
	float3 q = abs(p);

	const float3 k = float3(-0.8660254, 0.5, 0.57735);
	p = abs(p);
	p.xy -= 2.0*min(dot(k.xy, p.xy), 0.0)*k.xy;
	float2 d = float2(
		length(p.xy - float2(clamp(p.x, -k.z*h.x, k.z*h.x), h.x))*sign(p.y - h.x),
		p.z - h.y);
	return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float sdCapsule(float3 p, float3 a, float3 b, float r)
{
	float3 pa = p - a, ba = b - a;
	float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
	return length(pa - ba * h) - r;
}

float sdRoundCone(in float3 p, in float r1, float r2, float h)
{
	float2 q = float2(length(p.xz), p.y);

	float b = (r1 - r2) / h;
	float a = sqrt(1.0 - b * b);
	float k = dot(q, float2(-b, a));

	if (k < 0.0) return length(q) - r1;
	if (k > a*h) return length(q - float2(0.0, h)) - r2;

	return dot(q, float2(a, b)) - r1;
}

float dot2(in float3 v) { return dot(v, v); }
float sdRoundCone(float3 p, float3 a, float3 b, float r1, float r2)
{
	// sampling independent computations (only depend on shape)
	float3  ba = b - a;
	float l2 = dot(ba, ba);
	float rr = r1 - r2;
	float a2 = l2 - rr * rr;
	float il2 = 1.0 / l2;

	// sampling dependant computations
	float3 pa = p - a;
	float y = dot(pa, ba);
	float z = y - l2;
	float x2 = dot2(pa*l2 - ba * y);
	float y2 = y * y*l2;
	float z2 = z * z*l2;

	// single square root!
	float k = sign(rr)*rr*rr*x2;
	if (sign(z)*a2*z2 > k) return  sqrt(x2 + z2)        *il2 - r2;
	if (sign(y)*a2*y2 < k) return  sqrt(x2 + y2)        *il2 - r1;
	return (sqrt(x2*a2*il2) + y * rr)*il2 - r1;
}

float sdEquilateralTriangle(in float2 p)
{
	const float k = 1.73205;//sqrt(3.0);
	p.x = abs(p.x) - 1.0;
	p.y = p.y + 1.0 / k;
	if (p.x + k * p.y > 0.0) p = float2(p.x - k * p.y, -k * p.x - p.y) / 2.0;
	p.x += 2.0 - 2.0*clamp((p.x + 2.0) / 2.0, 0.0, 1.0);
	return -length(p)*sign(p.y);
}

float sdTriPrism(float3 p, float2 h)
{
	float3 q = abs(p);
	float d1 = q.z - h.y;
	h.x *= 0.866025;
	float d2 = sdEquilateralTriangle(p.xy / h.x)*h.x;
	return length(max(float2(d1, d2), 0.0)) + min(max(d1, d2), 0.);
}

// vertical
float sdCylinder(float3 p, float2 h)
{
	float2 d = abs(float2(length(p.xz), p.y)) - h;
	return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

// arbitrary orientation
float sdCylinder(float3 p, float3 a, float3 b, float r)
{
	float3 pa = p - a;
	float3 ba = b - a;
	float baba = dot(ba, ba);
	float paba = dot(pa, ba);

	float x = length(pa*baba - ba * paba) - r * baba;
	float y = abs(paba - baba * 0.5) - baba * 0.5;
	float x2 = x * x;
	float y2 = y * y*baba;
	float d = (max(x, y) < 0.0) ? -min(x2, y2) : (((x > 0.0) ? x2 : 0.0) + ((y > 0.0) ? y2 : 0.0));
	return sign(d)*sqrt(abs(d)) / baba;
}

float sdCone(in float3 p, in float3 c)
{
	float2 q = float2(length(p.xz), p.y);
	float d1 = -q.y - c.z;
	float d2 = max(dot(q, c.xy), q.y);
	return length(max(float2(d1, d2), 0.0)) + min(max(d1, d2), 0.);
}

float dot2(in float2 v) { return dot(v, v); }
float sdCappedCone(in float3 p, in float h, in float r1, in float r2)
{
	float2 q = float2(length(p.xz), p.y);

	float2 k1 = float2(r2, h);
	float2 k2 = float2(r2 - r1, 2.0*h);
	float2 ca = float2(q.x - min(q.x, (q.y < 0.0) ? r1 : r2), abs(q.y) - h);
	float2 cb = q - k1 + k2 * clamp(dot(k1 - q, k2) / dot2(k2), 0.0, 1.0);
	float s = (cb.x < 0.0 && ca.y < 0.0) ? -1.0 : 1.0;
	return s * sqrt(min(dot2(ca), dot2(cb)));
}

float sdOctahedron(float3 p, float s)
{
	p = abs(p);
	return (p.x + p.y + p.z - s)*0.57735027;
}



float length2(float2 p)
{
	return sqrt(p.x*p.x + p.y*p.y);
}

float length6(float2 p)
{
	p = p * p*p; p = p * p;
	return pow(p.x + p.y, 1.0 / 6.0);
}

float length8(float2 p)
{
	p = p * p; p = p * p; p = p * p;
	return pow(p.x + p.y, 1.0 / 8.0);
}

float sdTorus82(float3 p, float2 t)
{
	float2 q = float2(length2(p.xz) - t.x, p.y);
	return length8(q) - t.y;
}

float sdTorus88(float3 p, float2 t)
{
	float2 q = float2(length8(p.xz) - t.x, p.y);
	return length8(q) - t.y;
}

float sdCylinder6(float3 p, float2 h)
{
	return max(length6(p.xz) - h.x, abs(p.y) - h.y);
}

//------------------------------------------------------------------

float opS(float d1, float d2)
{
	return max(-d2, d1);
}

float2 opU(float2 d1, float2 d2)
{
	return (d1.x < d2.x) ? d1 : d2;
}

float3 opRep(float3 p, float3 c)
{
	return fmod(p, c) - 0.5*c;
}

float3 opTwist(float3 p)
{
	float  c = cos(10.0*p.y + 10.0);
	float  s = sin(10.0*p.y + 10.0);
	float2x2   m = transpose(float2x2(c, -s, s, c));
	float2 xz = mul(m, p.xz);
	return float3(xz.x, xz.y, p.y);
}

float softAbs2(float x, float a)
{
	float xx = 2.0*x / a; float abs2 = abs(xx);
	if (abs2 < 2.0)
		abs2 = 0.5*xx*xx*(1.0 - abs2 / 6) + 2.0 / 3.0;
	return abs2 * a / 2.0;
}

float softMax2(float x, float y, float a)
{
	return 0.5*(x + y + softAbs2(x - y, a));
}

float softMin2(float x, float y, float a)
{
	return   -0.5*(-x - y + softAbs2(x - y, a));
}

//------------------------------------------------------------------

#define ZERO (min(iFrame,0))

//------------------------------------------------------------------

float2 map(in float3 inpos)
{
	float3 pos = inpos;

	float2 res = float2(1e10, 0.0);

	//simple primitive examples
	res = opU(res, float2(sdOctahedron(pos - float3(-4, -3, 0), 0.5f), 50));
	res = opU(res, float2(sdEllipsoid(pos - float3 (-3, -3, 0), float3(0.5, 0.2, 0.7)), 50));
	res = opU(res, float2(sdBox(pos - float3(-2, -3, 0), float3(0.3, 0.7, 0.5)), 50));
	res = opU(res, float2(sdHexPrism(pos - float3(-1, -3, 0), float2(0.5, 0.7)), 50));
	res = opU(res, float2(sdCappedCone(pos - float3(0, -3, 0), 0.5, 0.4, 0.1), 50));

	res = opU(res, float2(softMax2(sdEllipsoid(pos - float3 (2, -3, 0), float3(0.5, 0.2, 0.7)),
		sdCappedCone(pos - float3(2, -3, 0), 0.5, 0.4, 0.1),
		-abs(sin(time))), 50));

	res = opU(res, float2(softMax2(sdOctahedron(pos - float3(4, -3, 0), 0.2f),
		sdBox(pos - float3(4, -3, 0), float3(0.5, 0.5, 0.5)),
		-abs(sin(time))), 50));

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
		for (int i = 0; i < 250 && t < tmax; i++)
		{

			float2 h = map(ro + rd * t);
			if (abs(h.x) < (0.00001*t))
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

		col = lerp(col, float3(0.8, 0.8, 0.8), 1.0 - exp(-0.0002*t*t));
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
