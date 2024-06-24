#version 330 core

in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D noisyImage;
uniform sampler2D albedoBuffer;
uniform sampler2D depthBuffer;
uniform sampler2D normalBuffer;
uniform sampler2D motionVector;
uniform sampler2D prevDenoisedImage;

float rand(vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec4 jointBilateralFilter(vec2 uv) {
    vec2 resolution = vec2(1280, 720);
    vec2 motion = texture(motionVector, texCoord).xy;
    float temporalWeight = 0.8;
    float k = 0.03;

    int sigma_p = 15;
    float sigma_c = 0.9;
    float sigma_a = 0.3;
    float sigma_n = 0.3;
    float sigma_d = 0.3;

    vec3 sum = vec3(0.0);
    float weightSum = 0.0;
    vec3 centerColor = texture(noisyImage, uv).rgb;
    vec3 centerAlbedo = texture(albedoBuffer, uv).rgb;
    float centerDepth = texture(depthBuffer, uv).r;
    vec3 centerNormal = texture(normalBuffer, uv).rgb;

    // Calculate mean and variance over a 7x7 block
    vec3 mean = vec3(0.0);
    vec3 squaredMean = vec3(0.0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec3 pixel = texture(noisyImage, texCoord + vec2(x, y) / resolution).rgb;
            mean += pixel;
            squaredMean += pixel * pixel;
        }
    }
    mean /= 9.0;
    squaredMean /= 9.0;
    vec3 variance = squaredMean - mean * mean;

    centerColor = clamp(centerColor, mean - 2.0 * variance, mean + 2.0 * variance);

    for (int x = -sigma_p; x <= sigma_p; x += 1) {
        vec2 offset = vec2(x,20*rand(vec2(x) + vec2(0.1, 0.4))) / resolution;
        vec3 sampleColor = texture(noisyImage, uv + offset).rgb;
        vec3 sampleAlbedo = texture(albedoBuffer, uv + offset).rgb;
        float sampleDepth = texture(depthBuffer, uv + offset).r;
        vec3 sampleNormal = texture(normalBuffer, uv + offset).rgb;

        float spatialDist = length(offset);
        float colorDist = length(centerColor - sampleColor);
        float albedoDist = length(centerAlbedo - sampleAlbedo);
        float depthDist = abs(centerDepth - sampleDepth);
        float normalDist = length(centerNormal - sampleNormal);

        float weight =    exp(-(spatialDist * spatialDist) / (2.0 * sigma_p * sigma_p)
                        -(colorDist * colorDist)     / (2.0 * sigma_c * sigma_c)
                        -(albedoDist * albedoDist)   / (2.0 * sigma_a * sigma_a)
                        -(depthDist * depthDist)     / (2.0 * sigma_d * sigma_d)
                        -(normalDist * normalDist)   / (2.0 * sigma_n * sigma_n));

        sum += sampleColor * weight;
        weightSum += weight;
    }

    for (int y = -sigma_p; y <= sigma_p; y += 1) {
        vec2 offset = vec2(10*rand(vec2(y) + vec2(0.7, 0.3)),y) / resolution;
        vec3 sampleColor = texture(noisyImage, uv + offset).rgb;
        vec3 sampleAlbedo = texture(albedoBuffer, uv + offset).rgb;
        float sampleDepth = texture(depthBuffer, uv + offset).r;
        vec3 sampleNormal = texture(normalBuffer, uv + offset).rgb;

        float spatialDist = length(offset);
        float colorDist = length(centerColor - sampleColor);
        float albedoDist = length(centerAlbedo - sampleAlbedo);
        float depthDist = abs(centerDepth - sampleDepth);
        float normalDist = length(centerNormal - sampleNormal);

        float weight =    exp(-(spatialDist * spatialDist) / (2.0 * sigma_p * sigma_p)
                        -(colorDist * colorDist)     / (2.0 * sigma_c * sigma_c)
                        -(albedoDist * albedoDist)   / (2.0 * sigma_a * sigma_a)
                        -(depthDist * depthDist)     / (2.0 * sigma_d * sigma_d)
                        -(normalDist * normalDist)   / (2.0 * sigma_n * sigma_n));

        sum += sampleColor * weight;
        weightSum += weight;
    }

    vec3 spatialFiltered = sum / weightSum;
    vec2 prevCoord = texCoord - motion / resolution;
    vec3 prevFiltered = texture(prevDenoisedImage, vec2(prevCoord.x, -prevCoord.y)).rgb;
    vec3 clampedPrevFiltered = clamp(prevFiltered, mean - 2.0 * variance, mean + 2.0 * variance);
    fragColor = vec4(mix(spatialFiltered, clampedPrevFiltered, temporalWeight), 1.0);
    return fragColor;
}

void main() {
    fragColor = jointBilateralFilter(texCoord);
}
