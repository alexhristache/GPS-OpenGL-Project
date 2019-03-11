#version 410 core

in vec3 normal;
in vec4 fragPosEye;
in vec4 fragPosEyePointLight;
in vec4 fragPosLightSpace;
in vec2 fragTexCoords;

out vec4 fColor;

//lighting
uniform	mat3 normalMatrix;
uniform mat3 lightDirMatrix;
uniform	vec3 lightColor;
uniform	vec3 lightDir;
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

// For directional light
vec3 ambient;
vec3 diffuse;
vec3 specular;
float ambientStrength = 0.2f;
float specularStrength = 0.5f;
float shininess = 32.0f;

// For point light
uniform vec3 positionP;
float constant = 1.0f;
float linear = 0.09f;
float quadratic = 0.032f; 
vec3 ambientP;
vec3 diffuseP;

// For choosing light
uniform int chooseLight = 0;

// Toggle Fog
uniform int fog = 1;

vec3 o;

float computeShadow()
{	
	// perform perspective divide
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    if(normalizedCoords.z > 1.0f)
        return 0.0f;
    // Transform to [0,1] range
    normalizedCoords = normalizedCoords * 0.5f + 0.5f;
    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;    
    // Get depth of current fragment from light's perspective
    float currentDepth = normalizedCoords.z;
    // Check whether current frag pos is in shadow
    float bias = max(0.05f * (1.0f - dot(normal, lightDir)), 0.005f);
    float shadow = currentDepth - bias > closestDepth  ? 1.0f : 0.0f;

	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for(int x = -1; x <= 1; ++x) {
		for(int y = -1; y <= 1; ++y) {
			float pcfDepth = texture(shadowMap, normalizedCoords.xy + vec2(x, y) * texelSize).r; 
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0;

    return shadow;	
}

vec3 calcDirLight()
{
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	//transform normal
	vec3 normalEye = normalize(normalMatrix * normal);	
	
	//compute light direction
	vec3 lightDirN = normalize(lightDirMatrix * lightDir);	

	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fragPosEye.xyz);
	
	//compute half vector
	vec3 halfVector = normalize(lightDirN + viewDirN);
		
	//compute ambient light
	ambient = ambientStrength * lightColor;
	
	//compute diffuse light
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	
	//compute specular light
	float specCoeff = pow(max(dot(halfVector, normalEye), 0.0f), shininess);
	specular = specularStrength * specCoeff * lightColor;
	
	float shadow = computeShadow();
	
	//modulate with diffuse map
	ambient *= vec3(texture(diffuseTexture, fragTexCoords));
	diffuse *= vec3(texture(diffuseTexture, fragTexCoords));
	specular *= vec3(texture(specularTexture, fragTexCoords));
	//modulate with shadow
	vec3 color = min((ambient + (1.0f - shadow) * diffuse) + (1.0f - shadow) * specular, 1.0f);
	
	return color;
}

float computeFog()
{
 float fogDensity = 0.01f;
 float fragmentDistance = length(fragPosEye);
 float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

 return clamp(fogFactor, 0.0f, 1.0f);
}

vec3 calcPointLight()
{
	ambientP = ambientStrength * vec3(1.0f, 0.0f, 0.0f);
	
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	//compute view direction 
	vec3 viewDir = normalize(cameraPosEye - fragPosEyePointLight.xyz);
	//compute normalized light direction
	vec3 lightDir = normalize(positionP - fragPosEyePointLight.xyz);
	//transform normal
	vec3 normalEye = normalize(normalMatrix * normal);	
	//compute half vector
	vec3 halfVector = normalize(lightDir + viewDir);
	
    // diffuse shading
    float diff = max(dot(normalEye, lightDir), 0.0);
    // specular shading
    float spec = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
    // attenuation
    float distance    = length(positionP - fragPosEyePointLight.xyz);
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));    
    // combine results
    vec3 ambient  = lightColor * ambientP * vec3(texture(diffuseTexture, fragTexCoords));
    vec3 diffuse  = lightColor * diff * vec3(texture(diffuseTexture, fragTexCoords));
    vec3 specular = lightColor * specularStrength * spec * vec3(texture(specularTexture, fragTexCoords));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
	
	return ambient + diffuse + specular;
}
void main() 
{
	vec3 color;
	if (chooseLight == 1)
	{
		color = vec3(0.0f, 0.0f, 0.0f);
	}
	else if (chooseLight == 2)
	{
		color = calcPointLight();
	}
	else color = calcPointLight() + calcDirLight();
	
	if (fog == 0)
	{
		fColor = vec4(color, 1.0f);
	} 
	else 
	{
		float fogFactor = computeFog();
		vec4 fogColor = vec4(0.3f, 0.3f, 0.3f, 1.0f);
		fColor = fogColor*(1-fogColor) + vec4(color*fogFactor, 0.0f);
	}
}