#version 410 core
in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;
in vec4 fPosSpotEye;
out vec4 fColor;


//lighting
uniform	vec3 lightDir;
uniform	vec3 lightColor;
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform int objectShadow;
//texture
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

vec3 ambient;
float ambientStrength = 0.1f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;

void computeLightComponents()
{		
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	//transform normal
	vec3 normalEye = normalize(fNormal);	
	
	//compute light direction
	vec3 lightDirN = normalize(lightDir);
	
	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
		
	//compute ambient light
	ambient = ambientStrength * lightColor;
	
	//compute diffuse light
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	
	//compute specular light
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
	specular = specularStrength * specCoeff * lightColor;
	
}

float computeShadow()
{
	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	normalizedCoords = normalizedCoords * 0.5 + 0.5;
	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
	float currentDepth = normalizedCoords.z;
	float bias = max(0.05f * (1.0f - dot(normalizedCoords, lightDir)), 0.005f);
	float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;
	if (normalizedCoords.z > 1.0f)
		return 0.0f;
	return shadow;
}

float computeFog()
{
 float fogDensity = 0.03f;
 float fragmentDistance = length(fPosEye);
 float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

 return clamp(fogFactor, 0.0f, 1.0f);
}


void main() 
{
	computeLightComponents();

	float theta = dot(normalize(spotLightPos - fPosSpotEye.xyz), normalize(-(spotLightDir - spotLightPos)));
	if (theta >= cos(radians(25.0f))) {
		ambient +=vec3(0.9f, 0.35f, 0.0f);
		diffuse += vec3(0.9f, 0.35f, 0.0f);
		specular += vec3(0.9f, 0.35f, 0.0f);

	}
	
	ambient *= texture(diffuseTexture, fTexCoords).rgb;
	diffuse *= texture(diffuseTexture, fTexCoords).rgb;
	specular *= texture(specularTexture, fTexCoords).rgb;
	if (objectShadow == 1) {
		float shadow = computeShadow();
		vec3 color = min((ambient + (1.0f - shadow)*diffuse) + (1.0f - shadow)*specular, 1.0f);
		float fogFactor = computeFog();
		vec4 fogColor = vec4(0.1f, 0.1f, 0.1f, 1.0f);
		
	fColor = fogColor * (1 - fogFactor) + vec4(color, 1.0f) * fogFactor;
	}else if (objectShadow == 0) {
		vec3 color = min((ambient + diffuse) * vec3(1.0f, 1.0f, 1.0f) + specular, 1.0f);
		float fogFactor = computeFog();
		vec4 fogColor = vec4(0.01f, 0.01f, 0.01f, 1.0f);
	
		fColor = fogColor * (1 - fogFactor) + vec4(color, 1.0f) * fogFactor;	
	}

			
}
