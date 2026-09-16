
//============================================================================
// íËêî
//----------------------------------------------------------------------------
static const float PI = 3.1415926f;
static const float InvPI = 1.0f / PI;
static const float Dielectricf0 = 0.04f;
static const float4 ColorSpaceDielectricSpec = float4(0.04f, 0.04f, 0.04f, 1.0f - 0.04f);

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

inline float PerceptualRoughnessToRoughness(float perceptual_roughness)
{
	return perceptual_roughness * perceptual_roughness;
}

inline float DisneyDiffuse(float nv, float nl, float lh, float perceptual_roughness)
{
	float fd90 = 0.5f + 2 * lh * lh * perceptual_roughness;
	float light_scatter = (1 + (fd90 - 1) * Pow5(1 - nl));
	float view_scatter = (1 + (fd90 - 1) * Pow5(1 - nv));

	float diffuse = light_scatter * view_scatter;
	//diffuse /= PI;
	//diffuse *= InvPI;
	return diffuse;
}

inline float SmithJointGGXVisibilityTerm(float nl, float nv, float roughness)
{
	float a = roughness;
	float lambda_v = nl * (nv * (1 - a) + a);
	float lambda_l = nv * (nl * (1 - a) + a);

	return 0.5f / (lambda_v + lambda_l + 0.0001f);
}

inline float SmithVisibilityTerm(float NdotL, float NdotV, float k)
{
	float gL = NdotL * (1 - k) + k;
	float gV = NdotV * (1 - k) + k;

	return 1.0 / (gL * gV + 1e-5f); // This function is not intended to be running on Mobile,
									// therefore epsilon is smaller than can be represented by half
}

inline float SmithBeckmannVisibilityTerm(float NdotL, float NdotV, float roughness)
{
	float c = 0.797884560802865f; // c = sqrt(2 / Pi)
	float k = roughness * c;
	return SmithVisibilityTerm(NdotL, NdotV, k) * 0.25f; // * 0.25 is the 1/4 of the visibility term
}

inline float NDFBlinnPhongNormalizedTerm(float NdotH, float n)
{
	// norm = (n+2)/(2*pi)
	float normTerm = (n + 2.0f) * (0.5f / PI);
	float specTerm = pow(NdotH, n);
	return specTerm * normTerm;
}

inline float PerceptualRoughnessToSpecPower(float perceptual_roughness)
{
    float m = PerceptualRoughnessToRoughness(perceptual_roughness); // m is the true academic roughness.
	float sq = max(1e-4f, m * m);
	float n = (2.0 / sq) - 2.0;                          // https://dl.dropboxusercontent.com/u/55891920/papers/mm_brdf.pdf
	n = max(n, 1e-4f);                                  // prevent possible cases of pow(0,0), which could happen when roughness is 1.0 and NdotH is zero
	return n;
}

inline float GGXTerm(float nh, float roughness)
{
	float a2 = roughness * roughness;
	float d = (nh * a2 - nh) * nh + 1.0f;

	return InvPI * a2 / (d * d + 0.0001f);
}

inline float3 FresnelTerm(float3 f0, float cos_a)
{
	float t = Pow5(1 - cos_a);
	return f0 + (1 - f0) * t;
}

inline float3 FresnelLerp(float3 F0, float3 F90, float cosA)
{
	float t = Pow5(1 - cosA);   // ala Schlick interpoliation
	return lerp(F0, F90, t);
}

inline float3 FresnelLerpFast(float3 F0, float3 F90, float cosA)
{
	float t = Pow4(1 - cosA);
	return lerp(F0, F90, t);
}

inline float OneMinusReflectivityFromMetallic(float metallic)
{
	float oneMinusDielectricSpec = ColorSpaceDielectricSpec.a;
	return oneMinusDielectricSpec - metallic * oneMinusDielectricSpec;
}

inline float3 DiffuseAndSpecularFromMetallic(float3 albedo, float metallic, out float3 spec_color, out float one_minus_reflectivity)
{
    spec_color = lerp(ColorSpaceDielectricSpec.rgb, albedo, metallic);
    one_minus_reflectivity = OneMinusReflectivityFromMetallic(metallic);
    return albedo * one_minus_reflectivity;
}

inline float3 PreMultiplyAlpha(float3 diff_color, float alpha, float one_minus_reflectivity, out float out_modified_alpha)
{
#if 1
    diff_color *= alpha;
    out_modified_alpha = 1 - one_minus_reflectivity + alpha * one_minus_reflectivity;
#else
	out_modified_alpha = alpha;
#endif
    return diff_color;
}


float3 BRDF1(
	float3 diffuse,
	float3 specular,
	float one_minus_reflectivity,
	float smoothness,
	float3 normal,
	float3 view_direction,
	float3 light_direction,
	float3 light_color,
	float3 gi_diffuse,
	float3 gi_specular)
{
	float perceptual_roughness = SmoothnessToPerceptualRoughness(smoothness);
	float3 half_direction = normalize(light_direction + view_direction);
	float nv = abs(dot(normal, view_direction));
	float nl = saturate(dot(normal, light_direction));
	float nh = saturate(dot(normal, half_direction));
	//float lv = saturate(dot(light_direction, view_direction));
	float lh = saturate(dot(light_direction, half_direction));

	float diffuse_term = DisneyDiffuse(nv, nl, lh, perceptual_roughness) * nl;

	float roughness = PerceptualRoughnessToRoughness(perceptual_roughness);
#if 1
	roughness = max(roughness, 0.002f);
	float v = SmithJointGGXVisibilityTerm(nl, nv, roughness);
	float d = GGXTerm(nh, PerceptualRoughnessToSpecPower(roughness));
#else
	float v = SmithBeckmannVisibilityTerm(nl, nv, roughness);
	float d = NDFBlinnPhongNormalizedTerm(nh, PerceptualRoughnessToSpecPower(roughness));
#endif

	float specular_term = v * d * PI;

	specular_term = max(0, specular_term * nl);
#if 0
	specular_term = 0;
#endif

	float surface_reduction = 1.0f / (roughness * roughness + 1.0f);

	specular_term *= any(specular) ? 1.0f : 0.0f;

	float grazing_term = saturate(smoothness + (1 - one_minus_reflectivity));

	return diffuse * (gi_diffuse + light_color * diffuse_term)
		+ light_color * specular_term * FresnelTerm(specular, lh)
		+ surface_reduction * gi_specular * FresnelLerp(specular, grazing_term, nv);
}

float3 BRDF2(
	float3 diffuse,
	float3 specular,
	float one_minus_reflectivity,
	float smoothness,
	float3 normal,
	float3 view_direction,
	float3 light_direction,
	float3 light_color,
	float3 gi_diffuse,
	float3 gi_specular)
{
	float3 half_direction = normalize(light_direction + view_direction);

	float nl = saturate(dot(normal, light_direction));
	float nh = saturate(dot(normal, half_direction));
	float nv = dot(normal, view_direction);
	float lh = saturate(dot(light_direction, half_direction));

	float perceptual_roughness = SmoothnessToPerceptualRoughness(smoothness);
	float roughness = PerceptualRoughnessToRoughness(perceptual_roughness);

#if 1
	float a = roughness;
	float a2 = a * a;
	float d = nh * nh * (a2 - 1.f) + 1.00001f;
	float specular_term = a2 / (max(0.1f, lh * lh) * (roughness + 0.5f) * (d * d) * 4);	
#else
	float specular_power = PerceptualRoughnessToSpecPower(perceptual_roughness);
	float inv_v = lh * lh * smoothness + perceptual_roughness * perceptual_roughness;
	float inv_f = lh;
	float specular_term = ((specular_power + 1) * pow(nh, specular_power)) / (8.0f * inv_v * inv_f + 1e-4f);
#endif


	float surface_reduction = (0.6f - 0.08f * perceptual_roughness);
	surface_reduction = 1.0 - roughness * perceptual_roughness * surface_reduction;
	float grazing_term = saturate(smoothness + (1 - one_minus_reflectivity));

	return (diffuse + specular_term * specular) * light_color * nl
		+ gi_diffuse * diffuse
		+ surface_reduction * gi_specular * FresnelLerpFast(specular, grazing_term, nv)
		;
}

float3 BRDF(
	float3 albedo,
	float metallic,
	float smoothness,
	float3 normal,
	float3 view_direction,
	float3 light_direction,
	float3 light_color,
	float3 ambient)
{
    float one_minus_reflectivity;

#if 0
    float3 gi_specular;
    float3 gi_diffuse = DiffuseAndSpecularFromMetallic(ambient, metallic, gi_specular, one_minus_reflectivity);
#elif 1
    const float3 gi_diffuse = ambient;
    const float3 gi_specular = float3(0.1f, 0.1f, 0.1f);
#else
    const float3 gi_diffuse = float3(0.2f, 0.2f, 0.2f);
	const float3 gi_specular = float3(0.1f, 0.1f, 0.1f);
#endif

	float3 specular;
	float3 diffuse = DiffuseAndSpecularFromMetallic(albedo, metallic, specular, one_minus_reflectivity);

	return BRDF2(
		diffuse,
		specular,
		one_minus_reflectivity,
		smoothness,
		normal,
		view_direction,
		light_direction,
		light_color,
		gi_diffuse,
		gi_specular);
}
