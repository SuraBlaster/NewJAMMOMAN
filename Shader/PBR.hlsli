
//============================================================================
// íËêî
//----------------------------------------------------------------------------
static const float PI = 3.1415926f;
static const float INV_PI = 1.0f / PI;
static const float DIELECTRIC_F0 = 0.04f;
static const float4 COLOR_SPACE_DIELECTRIC_SPEC = float4(0.04f, 0.04f, 0.04f, 1.0f - 0.04f);

//============================================================================
// ä÷êî
//----------------------------------------------------------------------------
inline float Pow5(float x)
{
	return x * x * x * x * x;
}

inline float Pow4(float x)
{
	return x * x * x * x;
}

inline float SmoothnessToPerceptualRoughness(float smoothness)
{
	return (1 - smoothness);
}

inline float PerceptualRoughnessToRoughness(float perceptualRoughness)
{
	return perceptualRoughness * perceptualRoughness;
}

inline float DisneyDiffuse(float nv, float nl, float lh, float perceptualRoughness)
{
	float fd90 = 0.5f + 2 * lh * lh * perceptualRoughness;
	float lightScatter = (1 + (fd90 - 1) * Pow5(1 - nl));
	float viewScatter = (1 + (fd90 - 1) * Pow5(1 - nv));

	float diffuse = lightScatter * viewScatter;
	//diffuse /= PI;
	//diffuse *= InvPI;
	return diffuse;
}

inline float SmithJointGGXVisibilityTerm(float nl, float nv, float roughness)
{
	float a = roughness;
	float lambdaV = nl * (nv * (1 - a) + a);
	float lambdaL = nv * (nl * (1 - a) + a);

	return 0.5f / (lambdaV + lambdaL + 0.0001f);
}

inline float SmithVisibilityTerm(float ndotL, float ndotV, float k)
{
	float gL = ndotL * (1 - k) + k;
	float gV = ndotV * (1 - k) + k;

	return 1.0 / (gL * gV + 1e-5f); // This function is not intended to be running on Mobile,
									// therefore epsilon is smaller than can be represented by half
}

inline float SmithBeckmannVisibilityTerm(float ndotL, float ndotV, float roughness)
{
	float c = 0.797884560802865f; // c = sqrt(2 / Pi)
	float k = roughness * c;
	return SmithVisibilityTerm(ndotL, ndotV, k) * 0.25f; // * 0.25 is the 1/4 of the visibility term
}

inline float NDFBlinnPhongNormalizedTerm(float ndotH, float n)
{
	// norm = (n+2)/(2*pi)
	float normTerm = (n + 2.0f) * (0.5f / PI);
	float specTerm = pow(ndotH, n);
	return specTerm * normTerm;
}

inline float PerceptualRoughnessToSpecPower(float perceptualRoughness)
{
    float m = PerceptualRoughnessToRoughness(perceptualRoughness); // m is the true academic roughness.
	float sq = max(1e-4f, m * m);
	float n = (2.0 / sq) - 2.0;                          // https://dl.dropboxusercontent.com/u/55891920/papers/mm_brdf.pdf
	n = max(n, 1e-4f);                                  // prevent possible cases of pow(0,0), which could happen when roughness is 1.0 and NdotH is zero
	return n;
}

inline float GGXTerm(float nh, float roughness)
{
	float a2 = roughness * roughness;
	float d = (nh * a2 - nh) * nh + 1.0f;

	return INV_PI * a2 / (d * d + 0.0001f);
}

inline float3 FresnelTerm(float3 f0, float cosA)
{
	float t = Pow5(1 - cosA);
	return f0 + (1 - f0) * t;
}

inline float3 FresnelLerp(float3 f0, float3 f90, float cosA)
{
	float t = Pow5(1 - cosA);   // ala Schlick interpoliation
	return lerp(f0, f90, t);
}

inline float3 FresnelLerpFast(float3 f0, float3 f90, float cosA)
{
	float t = Pow4(1 - cosA);
	return lerp(f0, f90, t);
}

inline float OneMinusReflectivityFromMetallic(float metallic)
{
	float oneMinusDielectricSpec = COLOR_SPACE_DIELECTRIC_SPEC.a;
	return oneMinusDielectricSpec - metallic * oneMinusDielectricSpec;
}

inline float3 DiffuseAndSpecularFromMetallic(float3 albedo, float metallic, out float3 specColor, out float oneMinusReflectivity)
{
    specColor = lerp(COLOR_SPACE_DIELECTRIC_SPEC.rgb, albedo, metallic);
    oneMinusReflectivity = OneMinusReflectivityFromMetallic(metallic);
    return albedo * oneMinusReflectivity;
}

inline float3 PreMultiplyAlpha(float3 diffColor, float alpha, float oneMinusReflectivity, out float outModifiedAlpha)
{
#if 1
    diffColor *= alpha;
    outModifiedAlpha = 1 - oneMinusReflectivity + alpha * oneMinusReflectivity;
#else
	outModifiedAlpha = alpha;
#endif
    return diffColor;
}


float3 BRDF1(
	float3 diffuse,
	float3 specular,
	float oneMinusReflectivity,
	float smoothness,
	float3 normal,
	float3 viewDirection,
	float3 lightDirection,
	float3 lightColor,
	float3 giDiffuse,
	float3 giSpecular)
{
	float perceptualRoughness = SmoothnessToPerceptualRoughness(smoothness);
	float3 halfDirection = normalize(lightDirection + viewDirection);
	float nv = abs(dot(normal, viewDirection));
	float nl = saturate(dot(normal, lightDirection));
	float nh = saturate(dot(normal, halfDirection));
	//float lv = saturate(dot(light_direction, view_direction));
	float lh = saturate(dot(lightDirection, halfDirection));

	float diffuseTerm = DisneyDiffuse(nv, nl, lh, perceptualRoughness) * nl;

	float roughness = PerceptualRoughnessToRoughness(perceptualRoughness);
#if 1
	roughness = max(roughness, 0.002f);
	float v = SmithJointGGXVisibilityTerm(nl, nv, roughness);
	float d = GGXTerm(nh, PerceptualRoughnessToSpecPower(roughness));
#else
	float v = SmithBeckmannVisibilityTerm(nl, nv, roughness);
	float d = NDFBlinnPhongNormalizedTerm(nh, PerceptualRoughnessToSpecPower(roughness));
#endif

	float specularTerm = v * d * PI;

	specularTerm = max(0, specularTerm * nl);
#if 0
	specularTerm = 0;
#endif

	float surfaceReduction = 1.0f / (roughness * roughness + 1.0f);

	specularTerm *= any(specular) ? 1.0f : 0.0f;

	float grazingTerm = saturate(smoothness + (1 - oneMinusReflectivity));

	return diffuse * (giDiffuse + lightColor * diffuseTerm)
		+ lightColor * specularTerm * FresnelTerm(specular, lh)
		+ surfaceReduction * giSpecular * FresnelLerp(specular, grazingTerm, nv);
}

float3 BRDF2(
	float3 diffuse,
	float3 specular,
	float oneMinusReflectivity,
	float smoothness,
	float3 normal,
	float3 viewDirection,
	float3 lightDirection,
	float3 lightColor,
	float3 giDiffuse,
	float3 giSpecular)
{
	float3 halfDirection = normalize(lightDirection + viewDirection);

	float nl = saturate(dot(normal, lightDirection));
	float nh = saturate(dot(normal, halfDirection));
	float nv = dot(normal, viewDirection);
	float lh = saturate(dot(lightDirection, halfDirection));

	float perceptualRoughness = SmoothnessToPerceptualRoughness(smoothness);
	float roughness = PerceptualRoughnessToRoughness(perceptualRoughness);

#if 1
	float a = roughness;
	float a2 = a * a;
	float d = nh * nh * (a2 - 1.f) + 1.00001f;
	float specularTerm = a2 / (max(0.1f, lh * lh) * (roughness + 0.5f) * (d * d) * 4);
#else
	float specularPower = PerceptualRoughnessToSpecPower(perceptualRoughness);
	float invV = lh * lh * smoothness + perceptualRoughness * perceptualRoughness;
	float invF = lh;
	float specularTerm = ((specularPower + 1) * pow(nh, specularPower)) / (8.0f * invV * invF + 1e-4f);
#endif


	float surfaceReduction = (0.6f - 0.08f * perceptualRoughness);
	surfaceReduction = 1.0 - roughness * perceptualRoughness * surfaceReduction;
	float grazingTerm = saturate(smoothness + (1 - oneMinusReflectivity));

	return (diffuse + specularTerm * specular) * lightColor * nl
		+ giDiffuse * diffuse
		+ surfaceReduction * giSpecular * FresnelLerpFast(specular, grazingTerm, nv)
		;
}

float3 BRDF(
	float3 albedo,
	float metallic,
	float smoothness,
	float3 normal,
	float3 viewDirection,
	float3 lightDirection,
	float3 lightColor,
	float3 ambient)
{
    float oneMinusReflectivity;

#if 0
    float3 giSpecular;
    float3 giDiffuse = DiffuseAndSpecularFromMetallic(ambient, metallic, giSpecular, oneMinusReflectivity);
#elif 1
    const float3 giDiffuse = ambient;
    const float3 giSpecular = float3(0.1f, 0.1f, 0.1f);
#else
    const float3 giDiffuse = float3(0.2f, 0.2f, 0.2f);
	const float3 giSpecular = float3(0.1f, 0.1f, 0.1f);
#endif

	float3 specular;
	float3 diffuse = DiffuseAndSpecularFromMetallic(albedo, metallic, specular, oneMinusReflectivity);

	return BRDF2(
		diffuse,
		specular,
		oneMinusReflectivity,
		smoothness,
		normal,
		viewDirection,
		lightDirection,
		lightColor,
		giDiffuse,
		giSpecular);
}
