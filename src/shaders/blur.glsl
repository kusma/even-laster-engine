vec4 fastBlur5x5(sampler2D samp, vec2 uv, int lod)
{
	const float centerWeight = 0.16210282163712664;
	const vec2 diagonalOffsets = vec2(0.3842896354828526, 1.2048616327242379);
	const vec4 offsets = vec4(-diagonalOffsets.xy, +diagonalOffsets.xy) / textureSize(samp, lod).xyxy;
	const float diagonalWeight = 0.2085034734347498;

	return textureLod(samp, uv, lod) * centerWeight +
	       textureLod(samp, uv + offsets.xy, lod) * diagonalWeight +
	       textureLod(samp, uv + offsets.wx, lod) * diagonalWeight +
	       textureLod(samp, uv + offsets.zw, lod) * diagonalWeight +
	       textureLod(samp, uv + offsets.yz, lod) * diagonalWeight;
}
