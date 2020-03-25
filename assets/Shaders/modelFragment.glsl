#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUv;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 camPos;

layout(location = 0) out vec4 outColor;

layout(set=1, binding=0) uniform sampler2D baseColorTexture;
layout(set=1, binding=1) uniform sampler2D metallicRoughnessTexture;
layout(set=1, binding=2) uniform sampler2D normalTexture;
layout(set=1, binding=3) uniform sampler2D occlusionTexture;
layout(set=1, binding=4) uniform sampler2D emissiveTexture;

layout(push_constant) uniform PushConstantsFrag
{
    layout(offset = 64)  vec4 baseColorFactor;
	layout(offset = 80)  vec4 emissiveFactor;
	layout(offset = 96)  float metallicFactor;
	layout(offset = 100) float roughnessFactor;
	layout(offset = 104) int baseColorTextureCoord;
	layout(offset = 108) int metallicRoughnessTextureCoord;
	layout(offset = 112) int normalTextureCoord;
	layout(offset = 116) int occlusionTextureCoord;
	layout(offset = 120) int emissiveTextureCoord;
    layout(offset = 124) int padding;
};

const float MIN_ROUGHNESS = 0.04;
const float ALPHA_CUTOFF = 0.1;
const float PI = 3.14159265359;
const float SCREEN_GAMMA = 2.2; // Assume the monitor is calibrated to the sRGB color space.
const float SCREEN_EXPOSURE = 1.0;

// Not sure if this should be used.
#define MANUAL_SRGB 1
vec4 SRGBtoLINEAR(vec4 srgbIn)
{
	#ifdef MANUAL_SRGB
	#ifdef SRGB_FAST_APPROXIMATION
	vec3 linOut = pow(srgbIn.xyz,vec3(2.2));
	#else //SRGB_FAST_APPROXIMATION
	vec3 bLess = step(vec3(0.04045),srgbIn.xyz);
	vec3 linOut = mix( srgbIn.xyz/vec3(12.92), pow((srgbIn.xyz+vec3(0.055))/vec3(1.055),vec3(2.4)), bLess );
	#endif //SRGB_FAST_APPROXIMATION
	return vec4(linOut,srgbIn.w);;
	#else //MANUAL_SRGB
	return srgbIn;
	#endif //MANUAL_SRGB
}

struct PBRInfo
{
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float LdotH;                  // cos angle between light direction and half vector
	float VdotH;                  // cos angle between view direction and half vector
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	float metalness;              // metallic value at the surface
	vec3 reflectance0;            // full reflectance color (normal incidence angle)
	vec3 reflectance90;           // reflectance color at grazing angle
	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	vec3 diffuseColor;            // color contribution from diffuse lighting
	vec3 specularColor;           // color contribution from specular lighting
};

vec3 Uncharted2Tonemap(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

vec4 tonemap(vec4 color)
{
	vec3 outcol = Uncharted2Tonemap(color.rgb * SCREEN_EXPOSURE);
	outcol = outcol * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
	return vec4(pow(outcol, vec3(1.0f / SCREEN_GAMMA)), color.a);
}

vec3 getNormal()
{
	// Perturb normal, see http://www.thetenthplanet.de/archives/1180
	vec3 tangentNormal = texture(normalTexture, fragUv).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(fragPos);
	vec3 q2 = dFdy(fragPos);
	vec2 st1 = dFdx(fragUv);
	vec2 st2 = dFdy(fragUv);

	vec3 N = normalize(fragNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

// Basic Lambertian diffuse
// Implementation from Lambert's Photometria https://archive.org/details/lambertsphotome00lambgoog
// See also [1], Equation 1
vec3 diffuse(PBRInfo pbrInputs)
{
	return pbrInputs.diffuseColor / PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(PBRInfo pbrInputs)
{
	return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alphaRoughness as input as originally proposed in [2].
float geometricOcclusion(PBRInfo pbrInputs)
{
	float NdotL = pbrInputs.NdotL;
	float NdotV = pbrInputs.NdotV;
	float r = pbrInputs.alphaRoughness;

	float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
	float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
	return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(PBRInfo pbrInputs)
{
	float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
	float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
	return roughnessSq / (PI * f * f);
}

void main() {
    /*vec3 baseColor = texture(baseColorTexture, fragUv).rgb;

    float diffuse = max(dot(normalize(fragNormal), -lightDir), 0.0);
    
    outColor = vec4(baseColor*diffuse, 1.0);*/
    vec3 lightDir = normalize(vec3(0.5, 2.0, 0.5)); // Vector from surface point to light

    // Get material color
	vec4 baseColor; // Should be raised to the power of screenGamma
	if(baseColorTextureCoord > -1)
		baseColor = SRGBtoLINEAR(texture(baseColorTexture, fragUv))*baseColorFactor; 
	else
		baseColor = baseColorFactor;
	if(baseColor.a < ALPHA_CUTOFF)
		discard;

	// Get metallic and grouhness factors
    float metallic = metallicFactor;
    float perceptualRoughness  = roughnessFactor;
    if(metallicRoughnessTextureCoord > -1)
	{
		perceptualRoughness  = texture(metallicRoughnessTexture, fragUv).g*perceptualRoughness;
		metallic = texture(metallicRoughnessTexture, fragUv).b*metallic;
	}
	else
	{
		perceptualRoughness  = clamp(perceptualRoughness, MIN_ROUGHNESS, 1.0);
		metallic = clamp(metallic, 0.0, 1.0);
	}

	vec3 f0 = vec3(0.04);
	vec3 diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
	diffuseColor *= 1.0 - metallic;

	float alphaRoughness = perceptualRoughness * perceptualRoughness;
	vec3 specularColor = mix(f0, baseColor.rgb, metallic);

	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
    float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	vec3 specularEnvironmentR0 = specularColor.rgb;
	vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

	vec3 n = (normalTextureCoord > -1) ? getNormal() : normalize(fragNormal);
	vec3 v = normalize(camPos - fragPos);    		// Vector from surface point to camera
	vec3 l = normalize(lightDir);     				// Vector from surface point to light
	vec3 h = normalize(l+v);                        // Half vector between both l and v
	vec3 reflection = -normalize(reflect(v, n));
	reflection.y *= -1.0f;

	float NdotL = clamp(dot(n, l), 0.001, 1.0);
	float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
	float NdotH = clamp(dot(n, h), 0.0, 1.0);
	float LdotH = clamp(dot(l, h), 0.0, 1.0);
	float VdotH = clamp(dot(v, h), 0.0, 1.0);

	PBRInfo pbrInputs = PBRInfo(
		NdotL,
		NdotV,
		NdotH,
		LdotH,
		VdotH,
		perceptualRoughness,
		metallic,
		specularEnvironmentR0,
		specularEnvironmentR90,
		alphaRoughness,
		diffuseColor,
		specularColor
	);

	// Calculate the shading terms for the microfacet specular shading model
	vec3 F = specularReflection(pbrInputs);
	float G = geometricOcclusion(pbrInputs);
	float D = microfacetDistribution(pbrInputs);

	const vec3 u_LightColor = vec3(1.0);

	// Calculation of analytical lighting contribution
	vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);
	vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
	// Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
	vec3 color = NdotL * u_LightColor * (diffuseContrib + specContrib);

	// Calculate lighting contribution from image based lighting source (IBL)
	//color += getIBLContribution(pbrInputs, n, reflection);

	const float u_OcclusionStrength = 1.0f;
	// Apply optional PBR terms for additional (optional) shading
	if (occlusionTextureCoord > -1) {
		float ao = texture(occlusionTexture, fragUv).r;
		color = mix(color, color * ao, u_OcclusionStrength);
	}

	if (emissiveTextureCoord > -1) {
		vec3 emissive = SRGBtoLINEAR(texture(emissiveTexture, fragUv)).rgb * emissiveFactor.rgb;
		color += emissive;
	}
	
	outColor = vec4(color, baseColor.a);
}